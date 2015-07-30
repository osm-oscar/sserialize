#include "DataSetFactory.h"
#include <sserialize/algorithm/hashspecializations.h>

namespace sserialize {

DataSetFactory::DataSetFactory(bool memoryBased) :
m_data(UByteArrayAdapter::createCache(1, (memoryBased ? sserialize::MM_PROGRAM_MEMORY : sserialize::MM_SHARED_MEMORY))),
m_ac(&m_data),
m_hitCount(0)
{
}

DataSetFactory::DataSetFactory(DataSetFactory && other) :
m_ac(std::move(other.m_ac)),
m_hitCount(other.m_hitCount.load())
{
	using std::swap;
	swap(m_hash, other.m_hash);
	//default init read-write-lock
}

DataSetFactory::~DataSetFactory() {}

DataSetFactory & DataSetFactory::operator=(DataSetFactory && other) {
	using std::swap;
	m_ac = std::move(other.m_ac);

	m_hitCount.store(other.m_hitCount.load());
	swap(m_hash, other.m_hash);
	//default init read-write-lock
	return *this;
}


void DataSetFactory::setDataStoreFile(sserialize::UByteArrayAdapter data) {
	if (size()) { //clear everything
		m_ac.clear();
		m_hitCount = 0;
		m_hash.clear();
	}
	m_data = data;
}

uint64_t DataSetFactory::hashFunc(const UByteArrayAdapter & v) {
	UByteArrayAdapter::MemoryView mv(v.asMemView());
	uint64_t h = 0;
	for(UByteArrayAdapter::MemoryView::const_iterator it(mv.cbegin()), end(mv.cend()); it != end; ++it) {
		hash_combine(h, *it);
	}
	return h;
}

int64_t DataSetFactory::getStoreId(const UByteArrayAdapter & v, uint64_t & hv) {
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

bool DataSetFactory::dataInStore(const UByteArrayAdapter & v, uint32_t id) {
	UByteArrayAdapter tmp = m_ac.dataAt(id);
	UByteArrayAdapter::MemoryView mvo(v.asMemView());
	m_dataLock.acquireReadLock();
	UByteArrayAdapter::MemoryView mv( tmp.asMemView() );
	bool eq = memcmp(mv.get(), mvo.get(), v.size()) == 0;
	m_dataLock.releaseReadLock();
	return eq;
}

uint32_t DataSetFactory::insert(const sserialize::UByteArrayAdapter & data) {
	uint64_t hv;
	int64_t id = getStoreId(data, hv);
	if (id < 0) {
		m_dataLock.acquireWriteLock();
		id = m_ac.size();
		m_ac.put(data);
		m_dataLock.releaseWriteLock();
		
		m_mapLock.acquireWriteLock();
		m_hash[hv].push_front(id);
		m_mapLock.releaseWriteLock();
	}
	else {
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
	return m_ac.dataAt(id);
}

OffsetType DataSetFactory::flush() {
	return m_ac.flush().size();
}

}//end namespace
