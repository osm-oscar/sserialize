#ifndef SSERIALIZE_DIRECT_HUGE_HASH_SET_H
#define SSERIALIZE_DIRECT_HUGE_HASH_SET_H
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <unordered_set>

namespace sserialize {

///This class provides a table based hash hashing all elements smaller than size into a file based table
///and the rest into a user provides hashmap
template<typename THashSet = std::unordered_set<uint64_t> >
class DirectHugeHashSet {
private:
	uint64_t m_begin;
	uint64_t m_end;
	DynamicBitSet m_bitSet;
	THashSet m_upperData;
public:
	DirectHugeHashSet() : m_begin(0), m_end(0) {}
	DirectHugeHashSet(uint64_t begin, uint64_t end) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_bitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true))
	{}
	~DirectHugeHashSet() {}
	
	///This function is lineaer in end-begin!
	uint64_t size() const {
		return m_bitSet.size() + m_upperData.size();
	}
	
	void insert(uint64_t pos) {
		if (m_begin <= pos && pos < m_end) {
			m_bitSet.set(pos-m_begin);
		}
		else {
			m_upperData.insert(pos);
		}
	}
	
	bool count(uint64_t pos) const {
		if (m_begin <= pos && pos < m_end) {
			return m_bitSet.isSet(pos-m_begin);
		}
		return m_upperData.count(pos);
	}
};

}//end namespace

#endif