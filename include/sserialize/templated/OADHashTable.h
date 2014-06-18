#ifndef SSERIALIZE_OAD_HASH_TABLE_H
#define SSERIALIZE_OAD_HASH_TABLE_H
#include <vector>
#include <stdexcept>

namespace sserialize {
namespace detail {
namespace OADHashTable {

template<typename TValue, bool invert>
struct DefaultHash {
	std::hash<TValue> hasher;
	inline size_t operator()(const TValue & v) const;
};

template<typename TValue>
struct DefaultHash<TValue, true> {
	std::hash<TValue> hasher;
	inline size_t operator()(const TValue & v) const { return ~hasher(v); }
};

template<typename TValue>
struct DefaultHash<TValue, false> {
	std::hash<TValue> hasher;
	inline size_t operator()(const TValue & v) const { return hasher(v); }
};

}}

///An Open addressing double hashed hash table with up to uint32_t::max-1 elements
///element order using the iterators is based on the insertion order
template<typename TKey, typename TValue, typename THash1 = detail::OADHashTable::DefaultHash<TKey, false>, typename THash2 = detail::OADHashTable::DefaultHash<TKey, true> >
class OADHashTable {
public:
	typedef TKey key_type;
	typedef TValue mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;
	typedef std::vector<value_type> ValueStorageType;
	typedef typename ValueStorageType::const_iterator const_iterator;
private:
	typedef std::vector<uint32_t> TableStorageType;
private:
	static const uint32_t findend = 0xFFFFFFFF;
private:
	ValueStorageType m_valueStorage;
	TableStorageType m_d;
	double m_maxLoad;
	double m_rehashMult;
	THash1 m_hash1;
	THash2 m_hash2;
private:
	uint32_t find(const key_type & key) const;
	void rehash(uint32_t count);
	
	//pos == 0 is the null-value => real values start with > 0
	inline value_type & value(uint32_t ptr) {
		return m_valueStorage[ptr-1];
	}
	
	inline const value_type & value(uint32_t ptr) const {
		return m_valueStorage[ptr-1];
	}
	
public:
	OADHashTable() :
	m_d(15, 0),
	m_maxLoad(0.8),
	m_rehashMult(2.0)
	{}
	OADHashTable(THash1 hash1, THash2 hash2, double maxLoad = 0.8) :
	m_d(16, 0), //start with a small hash table, start value has to obey (m_d.size()+1)/(m_rehashMult*m_d.size()) < m_maxLoad
	m_maxLoad(maxLoad),
	m_rehashMult(2.0),
	m_hash1(hash1),
	m_hash2(hash2)
	{}
	virtual ~OADHashTable() {}
	inline uint32_t size() const { return m_valueStorage.size(); }
	inline uint32_t storageCapacity()  const { return m_valueStorage.capacity();}
	inline double rehashMultiplier() const { return m_rehashMult;}
	inline void rehashMultiplier(double v) { m_rehashMult = v; }
	inline uint32_t capacity() const { return m_d.size();}
	inline double load_factor() const { return (double)size()/m_d.size();}
	inline double max_load_factor() const { return m_maxLoad;}
	void max_load_factor(double f);
	void reserve(uint32_t count);
	inline void reserveStorage(uint32_t count) {
		m_valueStorage.reserve(count);
	}
	mapped_type & operator[](const key_type & key);
	mapped_type & at(const key_type & key);
	const mapped_type & at(const key_type & key) const;
	
	inline bool count(const key_type & key) const {
		uint32_t pos = find(key);
		return (pos == findend ? false : bool(m_d[pos]));
	}
	inline const_iterator cbegin() const { return m_valueStorage.cbegin(); }
	inline const_iterator cend() const { return m_valueStorage.cend(); }
};

template<typename TKey, typename TValue, typename THash1, typename THash2>
uint32_t OADHashTable<TKey, TValue, THash1, THash2>::find(const key_type & key) const {
	uint64_t h1 = (m_hash1(key)) & ~0x1;
	uint32_t s = m_d.size();
	uint32_t cpos = h1%s;
	uint32_t cp = m_d[cpos];
	if (!cp || value(cp).first == key) {
		return cpos;
	}
	uint64_t h2 = (m_hash2(key) & ~0x1);
	for(uint64_t i = 1; i < m_d.size(); ++i) {
		cpos = (h1 + i*h2) % s;
		uint32_t cp = m_d[cpos];
		if (!cp || value(cp).first == key) {
			return cpos;
		}
	}
	return findend;
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
void OADHashTable<TKey, TValue, THash1, THash2>::rehash(uint32_t count) {
	count = count | 0x1;
	m_d.resize(0, 0);
	m_d.resize(count, 0);
	for(uint32_t i = 1, s = size(); i <= s; ++i) {
		uint32_t pos = find(value(i).first);
		if (pos != findend) {
			m_d[pos] = i;
		}
		else {
			reserve(count*m_rehashMult);
		}
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
void OADHashTable<TKey, TValue, THash1, THash2>::max_load_factor(double f) {
	m_maxLoad = f;
	if (load_factor() > m_maxLoad) {
		uint32_t targetSize = m_d.size()*m_rehashMult;
		while ( (double)size()/targetSize > m_maxLoad) {
			targetSize = targetSize*m_rehashMult;
		}
		rehash(targetSize);
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
void OADHashTable<TKey, TValue, THash1, THash2>::reserve(uint32_t count) {
	double newTableSize = count/m_maxLoad;
	if ((double)size()/newTableSize < m_maxLoad) {
		rehash(newTableSize);
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
typename OADHashTable<TKey, TValue, THash1, THash2>::mapped_type & OADHashTable<TKey, TValue, THash1, THash2>::operator[](const key_type & key) {
	uint32_t pos = find(key);
	uint32_t & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	if (pos == findend || ((double)(size()+1)/m_d.size() > m_maxLoad)) {
		rehash(m_d.size()*m_rehashMult);
		return operator[](key);
	}
	else {
		m_valueStorage.push_back(value_type(key, mapped_type()));
		cp = size();
		return value(cp).second;
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
typename OADHashTable<TKey, TValue, THash1, THash2>::mapped_type & OADHashTable<TKey, TValue, THash1, THash2>::at(const key_type & key) {
	uint32_t pos = find(key);
	uint32_t & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range();
}

template<typename TKey, typename TValue, typename THash1, typename THash2>
const typename OADHashTable<TKey, TValue, THash1, THash2>::mapped_type & OADHashTable<TKey, TValue, THash1, THash2>::at(const key_type & key) const {
	uint32_t pos = find(key);
	uint32_t & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range();
}

}//end namespace

#endif