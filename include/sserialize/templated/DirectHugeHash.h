#ifndef SSERIALIZE_DIRECT_HUGE_HASH_MAP_H
#define SSERIALIZE_DIRECT_HUGE_HASH_MAP_H
#include <sserialize/templated/MMVector.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <unordered_map>

namespace sserialize {

///This class provides a table based hash hashing all elements smaller than size into a file based table
///and the rest into a user provides hashmap
template<typename TValue, typename THashMap = std::unordered_map<uint64_t, TValue> >
class DirectHugeHashMap {
public:
	typedef THashMap BaseHashMap;
private:
	uint64_t m_begin;
	uint64_t m_end;
	uint64_t m_count;//count only within m_begin and m_end
	MMVector<TValue> m_data;
	DynamicBitSet m_bitSet;
	THashMap m_upperData;
private:
	//disable copy operaters as m_data and m_bitSet don't do any copying
	DirectHugeHashMap(const DirectHugeHashMap & other);
	DirectHugeHashMap & operator=(const DirectHugeHashMap & other);
public:
	DirectHugeHashMap() : m_begin(0), m_end(0), m_count(0), m_data(sserialize::MM_PROGRAM_MEMORY) {}
	DirectHugeHashMap(uint64_t begin, uint64_t end, MmappedMemoryType mmt) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_count(0),
	m_data(mmt, m_end-m_begin),
	m_bitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true))
	{}
	template<typename... THashParams>
	DirectHugeHashMap(uint64_t begin, uint64_t end, MmappedMemoryType mmt, THashParams... params) :
	m_begin(std::min<uint64_t>(begin, end)),
	m_end(std::max<uint64_t>(begin, end)),
	m_count(0),
	m_data(mmt, m_end-m_begin),
	m_bitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true)),
	m_upperData(params...)
	{}
	virtual ~DirectHugeHashMap() {}
	DirectHugeHashMap(DirectHugeHashMap && other) :
	m_begin(other.m_begin),
	m_end(other.m_end),
	m_count(other.m_count)
	{
		using std::swap;
		swap(m_data, other.m_data);
		swap(m_bitSet, other.m_bitSet);
		swap(m_upperData, other.m_upperData);
	}
	DirectHugeHashMap & operator=(DirectHugeHashMap && other) {
		clear();
		swap(other);
		return *this;
	}
	void swap(DirectHugeHashMap & other) {
		using std::swap;
		swap(m_begin, other.m_begin),
		swap(m_end, other.m_end);
		swap(m_count, other.m_count);
		swap(m_data, other.m_data);
		swap(m_bitSet, other.m_bitSet);
		swap(m_upperData, other.m_upperData);
	}
	void clear() {
		m_count = 0;
		//make sure to delete the old data prior allocating new memory in case of very large data
		m_data = MMVector<TValue>(m_data.mmt());
		m_upperData.clear();
		m_bitSet = DynamicBitSet();
		m_data.resize(m_end-m_begin);
		m_bitSet = DynamicBitSet(UByteArrayAdapter(new std::vector<uint8_t>((m_end-m_begin)/8+1, 0), true));
	}
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
	
	///memory in direct range is NOT reset
	void unmark(uint64_t pos) {
		if (m_begin <= pos && pos < m_end) {
			if (m_bitSet.isSet(pos-m_begin)) {
				--m_count;
				m_bitSet.unset(pos-m_begin);
			}
		}
		else {
			m_upperData.erase(pos);
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
	
	///thread safe with it self
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
				m_data[pos-m_begin] = TValue();
			}
			return m_data[pos-m_begin]; 
		}
		return m_upperData[pos];
	}
};

template<typename TValue>
void swap(DirectHugeHashMap<TValue> & a, DirectHugeHashMap<TValue> & b) {
	a.swap(b);
}

}//end namespace

#endif