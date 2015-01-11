#ifndef SSERIALIZE_DIRECT_HUGE_HASH_MAP_H
#define SSERIALIZE_DIRECT_HUGE_HASH_MAP_H
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <unordered_map>

namespace sserialize {

///This class provides a table based hash hashing all elements smaller than size into a file based table
///and the rest into a user provides hashmap
template<typename TValue, typename THashMap = std::unordered_map<uint64_t, TValue> >
class DirectHugeHashMap {
private:
	uint64_t m_begin;
	uint64_t m_end;
	uint64_t m_count;//count only within m_begin and m_end
	MmappedMemory<TValue> m_data;
	DynamicBitSet m_bitSet;
	THashMap m_upperData;
public:
	DirectHugeHashMap() : m_begin(0), m_end(0), m_count(0) {}
	DirectHugeHashMap(uint64_t begin, uint64_t end, MmappedMemoryType mmt) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_count(0),
	m_data(m_end-m_begin, mmt),
	m_bitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true))
	{}
	template<typename... THashParams>
	DirectHugeHashMap(uint64_t begin, uint64_t end, MmappedMemoryType mmt, THashParams... params) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_count(0),
	m_data(m_end-m_begin, mmt),
	m_bitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true)),
	m_upperData(params...)
	{}
	virtual ~DirectHugeHashMap() {}
	uint64_t beginDirectRange() const { return m_begin; }
	uint64_t endDirectRange() const { return m_end; }
	
	const THashMap & hashMap() const { return m_upperData; }
	
	uint64_t size() const {
		return m_count + m_upperData.size();
	}
	
	///reserve @count entries in hash map
	void reserve(uint64_t count) {
		m_upperData.reserve(count);
	}
	
	///Memory in direct range is not initialized
	void mark(uint64_t pos) {
		if (m_begin <= pos && pos < m_end) {
			if (!m_bitSet.isSet(pos-m_begin)) {
				++m_count;
				m_bitSet.set(pos-m_begin);
			}
		}
		else {
			m_upperData[pos] = TValue();
		}
	}
	
	///memory in direct range is set to TValue()
	void erase(uint64_t pos) {
		if (m_begin <= pos && pos < m_end) {
			if (m_bitSet.isSet(pos-m_begin)) {
				--m_count;
				m_bitSet.unset(pos-m_begin);
				m_data.data()[pos-m_begin] = TValue();
			}
		}
		else {
			m_upperData.erase(pos);
		}
	}
	
	///equal to operator[](pos) = TValue();
	inline void insert(uint64_t pos) {
		operator[](pos);
	}
	
	bool count(uint64_t pos) const {
		if (m_begin <= pos && pos < m_end) {
			return m_bitSet.isSet(pos-m_begin);
		}
		return m_upperData.count(pos);
	}
	
	const TValue & at(uint64_t pos) const {
		if (count(pos)) {
			return operator[](pos);
		}
		throw sserialize::OutOfBoundsException("DirectHugheHash::at");
	}
	
	TValue & at(uint64_t pos) {
		if (count(pos)) {
			return operator[](pos);
		}
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
			if (!m_bitSet.isSet(pos-m_begin)) {
				++m_count;
				m_bitSet.set(pos-m_begin);
				m_data.data()[pos-m_begin] = TValue();
			}
			return m_data.data()[pos-m_begin]; 
		}
		return m_upperData[pos];
	}
};

}//end namespace

#endif