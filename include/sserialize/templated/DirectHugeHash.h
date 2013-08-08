#ifndef SSERIALIZE_DIRECT_HUGHE_HASH_H
#define SSERIALIZE_DIRECT_HUGHE_HASH_H
#include <sserialize/utility/MmappedMemoy.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <unordered_map>

namespace sserialize {

///This class provides a table based hash hashing all elements smaller than size into a file based table
///and the rest into a user provides hashmap
template<typename TValue, typename THashMap = std::unordered_map<uint64_t, TValue> >
class DirectHugheHash {
private:
	uint64_t m_begin;
	uint64_t m_end;
	MmappedMemory<TValue> m_data;
	DynamicBitSet m_bitSet;
	THashMap m_upperData;
public:
	DirectHugheHash() : m_begin(0), m_end(0) {}
	DirectHugheHash(uint64_t begin, uint64_t end) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_data( static_cast<uint64_t>(m_end-m_begin)),
	m_bitSet(UByteArrayAdapter::createCache(static_cast<uint64_t>(m_end-m_begin)/8, true))
	{
		m_bitSet.data().zero();
	}
	virtual ~DirectHugheHash() {}
	
	///This function is lineaer in end-begin!
	uint64_t size() const {
		return m_bitSet.size() + m_upperData.size();
	}
	
	bool count(uint64_t pos) const {
		if (m_begin <= pos && pos < m_end) {
			return m_bitSet.isSet(pos-m_begin);
		}
		return (m_upperData.count(pos) > 0);
	}
	
	const TValue & at(uint64_t pos) const {
		if (count(pos))
			return operator[](pos);
		throw sserialize::OutOfBoundsException("DirectHugheHash::at");
	}
	
	TValue & at(uint64_t pos) {
		if (count(pos))
			return operator[](pos);
		throw sserialize::OutOfBoundsException("DirectHugheHash::at");
	}
	
	const TValue & operator[](uint64_t pos) const {
		if (m_begin <= pos && pos < m_end) {
			return m_data.data()[pos-m_begin]; 
		}
		return m_upperData.at(pos);
	}
	
	TValue & operator[](uint64_t pos) {
		if (m_begin <= pos && pos < m_end) {
			return m_data.data()[pos-m_begin]; 
		}
		return m_upperData[pos];
	}
};

}//end namespace

#endif