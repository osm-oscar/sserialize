#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/debuggerfunctions.h>
#include <sserialize/utility/ProgressInfo.h>
#include <unordered_map>
#include <iostream>
#include <sstream>

namespace sserialize {

ItemIndexFactory::ItemIndexFactory(bool memoryBased) :
m_indexIdCounter(0),
m_hitCount(0),
m_checkIndex(true),
m_bitWidth(-1),
m_useRegLine(true),
m_type(ItemIndex::T_REGLINE)
{
	if (memoryBased)
		m_indexStore = UByteArrayAdapter(new std::vector<uint8_t>(), true);
	else
		m_indexStore = UByteArrayAdapter::createCache(8*1024*1024, true);
}

ItemIndexFactory::~ItemIndexFactory() {}

UByteArrayAdapter ItemIndexFactory::at(sserialize::OffsetType offset) const {
	return m_indexStore+offset;
}

void ItemIndexFactory::setIndexFile(sserialize::UByteArrayAdapter data) {
	m_header = data;
	m_header.putUint8(0); // dummy version
	m_header.putUint8(0); //dumy index type
	m_header.putOffset(0); //dummy offset
	m_indexStore = m_header;
	m_indexStore.shrinkToPutPtr();
	m_header.resetPtrs();
	addIndex(std::set<uint32_t>(), 0);
}

uint64_t ItemIndexFactory::hashFunc(const std::vector<uint8_t> & v) {
	uint64_t h = 0;
	uint32_t count = std::min<uint32_t>(v.size(), 1024);
#if 1
	const uint8_t * vPtr = &(v[0]);
	for(size_t i = 0; i < count; ++i, ++vPtr)
		h += *vPtr;
#else
	for(size_t i = 0, k = 0; i < count; i = ((i+1) << 1), k+=8) {
		uint64_t tmp = 0;
		for(size_t j = 0; j+i < count; ++j) {
			tmp |= (static_cast<uint64_t>(v[i+j]) << j);
		}
		h += (tmp << k);
	}
#endif
	count = std::min<uint32_t>(count, 16*4);
	for(size_t i = 3;  i < count; i+=4) {
		h += (static_cast<uint64_t>(v[i]) << 48);
	}
	return h;
}

int64_t ItemIndexFactory::getIndex(const std::vector<uint8_t> & v, uint64_t & hv) {
	if (v.size() == 0)
		return -1;
	hv = hashFunc(v);
	if (m_hash.count(hv) == 0) {
		return -1;
	}
	else {
		std::list<OffsetType>::const_iterator end = m_hash.at(hv).end();
		for(std::list<OffsetType>::const_iterator it = m_hash.at(hv).begin(); it != end; ++it) {
			if (indexInStore(v, *it))
				return *it;
		}
	}
	return -1;
}

bool ItemIndexFactory::indexInStore(const std::vector< uint8_t >& v, uint64_t offset) {
	if (v.size() > (m_indexStore.tellPutPtr()-offset))
		return false;
	for(size_t i = 0; i < v.size(); i++) {
		if (v.at(i) != m_indexStore.at(offset+i))
			return false;
	}
	return true;
}

uint32_t ItemIndexFactory::addIndex(const std::vector< uint8_t >& idx, sserialize::OffsetType * indexOffset) {
	uint64_t hv;
	int64_t indexPos = getIndex(idx, hv);
	if (indexPos < 0) {
		indexPos = m_indexStore.tellPutPtr();
		m_offsetsToId[indexPos] = m_indexIdCounter;
		m_indexIdCounter++;
		m_indexStore.put(idx);
		m_hash[hv].push_back(indexPos);//deque<> takes of creating an empty list if none exists
	}
	else {
		m_hitCount++;
	}
	if (indexOffset)
		*indexOffset = indexPos;
	return m_offsetsToId[indexPos];
}

uint32_t ItemIndexFactory::addIndex(const std::unordered_set<uint32_t> & idx, bool * ok, OffsetType * indexOffset) {
	return addIndex< std::set<uint32_t> >(std::set<uint32_t>(idx.begin(), idx.end()), ok, indexOffset);
}

UByteArrayAdapter ItemIndexFactory::getFlushedData() {
	UByteArrayAdapter fd = m_header;
	fd.growStorage(m_indexStore.tellPutPtr());
	return fd;
}


OffsetType ItemIndexFactory::flush() {
	std::cout << "Serializing index with type=" << m_type << std::endl;
	m_header.resetPtrs();
	m_header << static_cast<uint8_t>(1); //Version
	m_header << static_cast<uint8_t>(m_type);//type
	m_header.putOffset(m_indexStore.tellPutPtr());
	
	std::cout << "Gathering offsets...";
	//Create the offsets
	std::vector<uint64_t> os(m_offsetsToId.size(), 0);
	for(std::unordered_map<uint64_t, uint32_t>::const_iterator it = m_offsetsToId.begin(); it != m_offsetsToId.end(); ++it) {
			os[it->second] = it->first;
	}
	std::cout << os.size() << " gathered" << std::endl;
	std::cout << "Serializing offsets...";
	uint64_t oIBegin = m_indexStore.tellPutPtr();
	if (! Static::SortedOffsetIndexPrivate::create(os, m_indexStore) ) {
		std::cout << "ItemIndexFactory::serialize: failed to create Offsetindex." << std::endl;
		return 0;
	}
	else {
		UByteArrayAdapter oIData(m_indexStore, oIBegin);
		sserialize::Static::SortedOffsetIndex oIndex(oIData);
		if (os != oIndex) {
			std::cout << "OffsetIndex creation FAILED!" << std::endl;
		}
	}
	std::cout << "done." << std::endl;

	return 2+UByteArrayAdapter::OffsetTypeSerializedLength()+m_indexStore.tellPutPtr();
}

}//end namespace
