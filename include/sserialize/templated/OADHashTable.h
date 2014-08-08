#ifndef SSERIALIZE_OAD_HASH_TABLE_H
#define SSERIALIZE_OAD_HASH_TABLE_H
#include <vector>
#include <stdexcept>
#include <limits>
#include <algorithm>

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

template<typename TOADSizeType>
struct OADSizeTypeLimits {};

template<>
struct OADSizeTypeLimits<uint32_t> {
	static constexpr uint32_t max = 0xFFFFFFFF;
};

template<>
struct OADSizeTypeLimits<uint64_t> {
	static constexpr uint64_t max = 0xFFFFFFFFFFFFFFFF;
};

}}

///An Open addressing double hashed hash table with up to ValueStorageType::value_type::max-1 elements
///element order using the iterators is based on the insertion order
template<	typename TKey,
			typename TValue,
			typename THash1 = detail::OADHashTable::DefaultHash<TKey, false>,
			typename THash2 = detail::OADHashTable::DefaultHash<TKey, true>,
			typename TValueStorageType = std::vector< std::pair<TKey, TValue> >,
			typename TTableStorageType = std::vector<uint32_t> >
class OADHashTable {
public:
	typedef TKey key_type;
	typedef TValue mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef TValueStorageType ValueStorageType;
	typedef TTableStorageType TableStorageType;
	typedef typename ValueStorageType::iterator iterator;
	typedef typename ValueStorageType::const_iterator const_iterator;
	typedef typename TableStorageType::value_type SizeType;
private:
	static constexpr SizeType findend = detail::OADHashTable::OADSizeTypeLimits<SizeType>::max;
private:
	ValueStorageType m_valueStorage;
	TableStorageType m_d;
	double m_maxLoad;
	double m_rehashMult;
	THash1 m_hash1;
	THash2 m_hash2;
private:
	SizeType find(const key_type & key) const;
	void rehash(SizeType count);
	
	//pos == 0 is the null-value => real values start with > 0
	inline value_type & value(SizeType ptr) {
		return m_valueStorage[ptr-1];
	}
	
	inline const value_type & value(SizeType ptr) const {
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
	OADHashTable(THash1 hash1, THash2 hash2, double maxLoad, const ValueStorageType & valueStorage, const TableStorageType & tableStorage) :
	m_valueStorage(valueStorage),
	m_d(tableStorage),
	m_maxLoad(maxLoad),
	m_rehashMult(2.0),
	m_hash1(hash1),
	m_hash2(hash2)
	{
		m_valueStorage.clear();
		m_d.clear();
		m_d.resize(16, 0);
	}
	OADHashTable(const ValueStorageType & valueStorage, const TableStorageType & tableStorage) :
	m_valueStorage(valueStorage),
	m_d(tableStorage),
	m_maxLoad(0.8),
	m_rehashMult(2.0)
	{
		m_valueStorage.clear();
		m_d.clear();
		m_d.resize(16, 0);
	}
	OADHashTable(const OADHashTable & other) :
	m_valueStorage(other.m_valueStorage),
	m_d(other.m_d),
	m_maxLoad(other.m_maxLoad),
	m_rehashMult(other.m_rehashMult),
	m_hash1(other.m_hash1),
	m_hash2(other.m_hash2)
	{}
	virtual ~OADHashTable() {}
	inline SizeType size() const { return m_valueStorage.size(); }
	inline SizeType storageCapacity()  const { return m_valueStorage.capacity();}
	inline double rehashMultiplier() const { return m_rehashMult;}
	inline void rehashMultiplier(double v) { m_rehashMult = v; }
	inline SizeType capacity() const { return m_d.size();}
	inline double load_factor() const { return (double)size()/m_d.size();}
	inline double max_load_factor() const { return m_maxLoad;}
	void max_load_factor(double f);
	void reserve(SizeType count);
	mapped_type & operator[](const key_type & key);
	mapped_type & at(const key_type & key);
	const mapped_type & at(const key_type & key) const;
	inline bool count(const key_type & key) const {
		SizeType pos = find(key);
		return (pos == findend ? false : bool(m_d[pos]));
	}
	inline const_iterator cbegin() const { return m_valueStorage.cbegin(); }
	inline const_iterator cend() const { return m_valueStorage.cend(); }
	
	///After changing a key you have to call rehash to make the hashtable consistent
	inline iterator begin() { return m_valueStorage.begin(); }
	///After changing a key you have to call rehash to make the hashtable consistent
	inline iterator end() { return m_valueStorage.end(); }
	///Sort the elments in the value storage and do a rehash afterwards.
	///If you do this before using cbegin/cend, then the elements in between cbegin-cend will be sorted according to T_SORT_OP
	template<typename T_SORT_OP>
	void sort(T_SORT_OP op);
};

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
typename OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::SizeType
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::find(const key_type & key) const {
	uint64_t h1 = (m_hash1(key)) & ~0x1;
	SizeType s = m_d.size();
	SizeType cpos = h1%s;
	SizeType cp = m_d[cpos];
	if (!cp || value(cp).first == key) {
		return cpos;
	}
	uint64_t h2 = (m_hash2(key) & ~0x1);
	for(uint64_t i = 1; i < m_d.size(); ++i) {
		cpos = (h1 + i*h2) % s;
		SizeType cp = m_d[cpos];
		if (!cp || value(cp).first == key) {
			return cpos;
		}
	}
	return findend;
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::rehash(SizeType count) {
	count = count | 0x1;
	m_d.clear();
	m_d.resize(count, 0);
	for(SizeType i = 1, s = size(); i <= s; ++i) {
		SizeType pos = find(value(i).first);
		if (pos != findend) {
			m_d[pos] = i;
		}
		else {
			reserve(count*m_rehashMult);
		}
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::max_load_factor(double f) {
	m_maxLoad = f;
	if (load_factor() > m_maxLoad) {
		SizeType targetSize = m_d.size()*m_rehashMult;
		while ( (double)size()/targetSize > m_maxLoad) {
			targetSize = targetSize*m_rehashMult;
		}
		rehash(targetSize);
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::reserve(SizeType count) {
	double newTableSize = count/m_maxLoad;
	if ((double)size()/newTableSize < m_maxLoad) {
		rehash(newTableSize);
	}
	m_valueStorage.reserve(count);
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::operator[](const key_type & key) {
	SizeType pos = find(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
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

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::at(const key_type & key) {
	SizeType pos = find(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range();
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
const TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::at(const key_type & key) const {
	SizeType pos = find(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range();
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType>
template<typename T_SORT_OP>
void OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType>::sort(T_SORT_OP op) {
	std::sort(m_valueStorage.begin(), m_valueStorage.end(), op);
	rehash(m_d.size());
}

}//end namespace

#endif