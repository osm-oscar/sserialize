#include <sserialize/containers/DataSetFactory.h>
#include <sserialize/utility/hashspecializations.h>

namespace sserialize {

DataSetFactory::DataSetFactory(bool memoryBased) :
m_hitCount(0)
{
	if (memoryBased)
		 setDataStoreFile( UByteArrayAdapter(new std::vector<uint8_t>(), true) );
	else
		setDataStoreFile( UByteArrayAdapter::createCache(8*1024*1024, sserialize::MM_FILEBASED) );
}

DataSetFactory::DataSetFactory(DataSetFactory && other) :
m_hitCount(other.m_hitCount.load())
{
	using std::swap;
	swap(m_header, other.m_header);
	swap(m_dataStore, other.m_dataStore);
	swap(m_hash, other.m_hash);
	swap(m_offsetsToId, other.m_offsetsToId);
	swap(other.m_idToOffsets, other.m_idToOffsets);
	//default init read-write-lock
}

DataSetFactory::~DataSetFactory() {}

DataSetFactory & DataSetFactory::operator=(DataSetFactory && other) {
	using std::swap;

	m_hitCount.store(other.m_hitCount.load());
	swap(m_header, other.m_header);
	swap(m_dataStore, other.m_dataStore);
	swap(m_hash, other.m_hash);
	swap(m_offsetsToId, other.m_offsetsToId);
	swap(other.m_idToOffsets, other.m_idToOffsets);
	//default init read-write-lock
	return *this;
}


void DataSetFactory::setDataStoreFile(sserialize::UByteArrayAdapter data) {
	if (size()) { //clear eversthing
		m_hitCount = 0;
		m_hash.clear();
		m_offsetsToId.clear();
		m_idToOffsets.clear();
	}

	m_header = data;
	m_header.putUint8(0); // dummy version
	m_header.putOffset(0); //dummy offset
	m_dataStore = m_header;
	m_dataStore.shrinkToPutPtr();
	m_header.resetPtrs();
}

uint64_t DataSetFactory::hashFunc(const UByteArrayAdapter & v) {
	UByteArrayAdapter::MemoryView mv(v.asMemView());
	uint64_t h = 0;
	for(UByteArrayAdapter::MemoryView::const_iterator it(mv.cbegin()), end(mv.cend()); it != end; ++it) {
		hash_combine(h, *it);
	}
	return h;
}


int64_t DataSetFactory::getStoreOffset(const UByteArrayAdapter & v, uint64_t & hv) {
	if (v.size() == 0)
		return -1;
	hv = hashFunc(v);
	m_mapLock.acquireReadLock();
	if (m_hash.count(hv) == 0) {
		m_mapLock.releaseReadLock();
		return -1;
	}
	else {
		DataOffsetContainer::const_iterator it(m_hash[hv].begin()), end(m_hash[hv].end());
		m_mapLock.releaseReadLock(); //write threads will not change something between it and end
		for(; it != end; ++it) {
			if (dataInStore(v, *it)) {
				return *it;
			}
		}
	}
	return -1;
}

bool DataSetFactory::dataInStore(const UByteArrayAdapter & v, uint64_t offset) {
	if (v.size() > (m_dataStore.tellPutPtr()-offset))
		return false;
	UByteArrayAdapter::MemoryView mvo(v.asMemView());
	m_dataLock.acquireReadLock();
	UByteArrayAdapter::MemoryView mv( m_dataStore.getMemView(offset, v.size()) );
	bool eq = memcmp(mv.get(), mvo.get(), v.size()) == 0;
	m_dataLock.releaseReadLock();
	return eq;
}

uint32_t DataSetFactory::insert(const sserialize::UByteArrayAdapter & data) {
	uint64_t hv;
	int64_t indexPos = getStoreOffset(data, hv);
	uint32_t id = std::numeric_limits<uint32_t>::max();
	if (indexPos < 0) {
		m_dataLock.acquireWriteLock();
		indexPos = m_dataStore.tellPutPtr();
		m_dataStore.put(data);
		m_dataLock.releaseWriteLock();
		
		m_mapLock.acquireWriteLock();
		id = size();
		m_offsetsToId[indexPos] = id;
		m_idToOffsets.push_back(indexPos);
		m_hash[hv].push_front(indexPos);
		m_mapLock.releaseWriteLock();
	}
	else {
		m_mapLock.acquireReadLock();
		id = m_offsetsToId[indexPos];
		m_mapLock.releaseReadLock();
		++m_hitCount;
	}
	return id;
}

uint32_t DataSetFactory::insert(const std::vector<uint8_t> & data) {
	sserialize::UByteArrayAdapter tmp(const_cast<std::vector<uint8_t>*>(&data), false);
	return insert(tmp);
}

UByteArrayAdapter DataSetFactory::at(uint32_t id) const {
	if (id >= size()) {
		throw sserialize::OutOfBoundsException("sserialize::DataSetFactory::dataAtToRename");
	}
	return m_dataStore+m_idToOffsets.at(id);
}

UByteArrayAdapter DataSetFactory::getFlushedData() {
	UByteArrayAdapter fd = m_header;
	fd.growStorage(m_dataStore.tellPutPtr());
	return fd;
}

OffsetType DataSetFactory::flush() {
	m_header.resetPtrs();
	m_header << static_cast<uint8_t>(3); //Version
	m_header.putOffset(m_dataStore.tellPutPtr());
	
	std::cout << "Gathering offsets...";
	//Create the offsets
	std::vector<uint64_t> os(m_offsetsToId.size(), 0);
	for(OffsetToIdHashType::const_iterator it (m_offsetsToId.begin()), end(m_offsetsToId.end()); it != end; ++it) {
			os[it->second] = it->first;
	}
	std::cout << os.size() << " gathered" << std::endl;
	std::cout << "Serializing offsets...";
	uint64_t oIBegin = m_dataStore.tellPutPtr();
	if (! Static::SortedOffsetIndexPrivate::create(os, m_dataStore) ) {
		std::cout << "ItemIndexFactory::serialize: failed to create Offsetindex." << std::endl;
		return 0;
	}
	else {
		UByteArrayAdapter oIData(m_dataStore, oIBegin);
		sserialize::Static::SortedOffsetIndex oIndex(oIData);
		if (os != oIndex) {
			std::cout << "OffsetIndex creation FAILED!" << std::endl;
		}
	}
	std::cout << "done." << std::endl;

	return 3+UByteArrayAdapter::OffsetTypeSerializedLength()+m_dataStore.tellPutPtr();
}

}//end namespace
