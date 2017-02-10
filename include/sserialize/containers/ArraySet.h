#ifndef SSERIALIZE_ARRAY_SET_H
#define SSERIALIZE_ARRAY_SET_H
#include <vector>
#include <string.h>

namespace sserialize {

template<typename T_INTEGER_TYPE>
class ArraySet {
public:
	typedef T_INTEGER_TYPE value_type;
	typedef typename std::vector<value_type>::const_iterator const_iterator;
protected:
	bool contains(const_iterator it, const const_iterator & end, value_type id) const {
		for(; it < end; ++it) {
			//early termination is faster than trying to improve branches here by reading cacheline-size many items at once
			if (*it == id) {
				return true;
			}
		}
		return false;
	}
public:
	ArraySet() {}
	bool count(value_type id) const {
		return contains(d().begin(), d().end(), id);
	}
	void insert(value_type id) {
		m_d.emplace_back(id);
	}
	void clear() {
		m_d.clear();
	}
protected:
	const std::vector<value_type> & d() const { return m_d; }
protected:
	std::vector<value_type> m_d;

};

template<typename T_INTEGER_TYPE>
class ArraySetWithStartOffset: public ArraySet<T_INTEGER_TYPE> {
public:
	typedef ArraySet<T_INTEGER_TYPE> MyBaseClass;
	typedef typename MyBaseClass::value_type value_type;
public:
	ArraySetWithStartOffset() {
		clear();
	}
	bool count(value_type id) const {
		uint8_t bf = id & 0xFF;
		return MyBaseClass::contains(MyBaseClass::d().cbegin()+m_st[bf], MyBaseClass::d().cend(), id);
	}
	void insert(int id) {
		uint8_t bf = id & 0xFF;
		MyBaseClass::insert(id);
		m_st[bf] = std::min<std::size_t>(MyBaseClass::d().size()-1, m_st[bf]);
	}
	void clear() {
		;MyBaseClass::clear();
		::memset(m_st, 0xFF, 256);
	}
protected:
	uint8_t m_st[256];
};

template<typename T_BASE, int T_SHIFT>
class ArraySetWithBloomFilter {
public:
	typedef T_BASE MyBaseClass;
	typedef typename MyBaseClass::value_type value_type;
public:
	ArraySetWithBloomFilter() {
		clear();
	}
	bool count(value_type id) const {
		uint8_t bf = calcBf(id);
		if ( (m_bf[bf/8] >> (bf % 8)) & 0x1 ){
			return m_d.count(id);
		}
		return false;
	}
	void insert(value_type id) {
		uint8_t bf = calcBf(id);
		m_bf[bf/8] |= static_cast<uint8_t>(1) << (bf % 8);
		m_d.insert(id);
	}
	void clear() {
		m_d.clear();
		::memset(m_bf, 0, 256/8);
	}
private:
	inline uint8_t calcBf(int id) const {
		return (id >> T_SHIFT) & 0xFF;
	}
private:
	MyBaseClass m_d;
	//bloom filter (a very stupid one at least)
	//using anything more complicated would be worse than the current implementation of a map
	//the idea here is that for a single node whose neighborhood gets explored
	//the ids of the neighbors is distributed uniformly at random and there shouldn't be too many (< 100)
	uint8_t m_bf[256/8];
};

template<typename T_INTEGER_TYPE>
using ArraySetSOBF = ArraySetWithBloomFilter<ArraySetWithStartOffset<T_INTEGER_TYPE>, 8>;

}//end namespace

#endif