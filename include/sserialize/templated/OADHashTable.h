#ifndef SSERIALIZE_OAD_HASH_TABLE_H
#define SSERIALIZE_OAD_HASH_TABLE_H
#include <vector>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <sserialize/utility/utilcontainerfuncs.h>
#include <sserialize/utility/log.h>

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
	///This is a really stupid secondary hash
	///as it completely depends on the value of the primary hash-functions
	///-> this will not resolve collisions for values where prim_hash(x) == prim_hash(y)
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
//dev WARNING:do NOT add ability of deleting functions without looking at the code for maxCollision
template<	typename TKey,
			typename TValue,
			typename THash1 = detail::OADHashTable::DefaultHash<TKey, false>,
			typename THash2 = detail::OADHashTable::DefaultHash<TKey, true>,
			typename TValueStorageType = std::vector< std::pair<TKey, TValue> >,
			typename TTableStorageType = std::vector<uint32_t>,
			typename TKeyEq = std::equal_to<TKey> >
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
	static constexpr uint64_t findend = detail::OADHashTable::OADSizeTypeLimits<uint64_t>::max;
private:
	ValueStorageType m_valueStorage;
	TableStorageType m_d;
	double m_maxLoad;
	double m_rehashMult;
	uint64_t m_maxCollisions;
	THash1 m_hash1;
	THash2 m_hash2;
	TKeyEq m_keyEq;
private:
	uint64_t findBucket(const key_type & key) const;
	void rehash(uint64_t count);
	
	//pos == 0 is the null-value => real values start with > 0
	inline value_type & value(SizeType ptr) {
		return m_valueStorage[ptr-1];
	}
	
	inline const value_type & value(SizeType ptr) const {
		return m_valueStorage[ptr-1];
	}
	
	inline iterator valueIt(SizeType ptr) {
		return m_valueStorage.begin()+(ptr-1);
	}
	
	inline const_iterator valueIt(SizeType ptr) const {
		return m_valueStorage.cbegin()+(ptr-1);
	}
	
public:
	OADHashTable() :
	m_d(16, 0),
	m_maxLoad(0.8),
	m_rehashMult(2.0),
	m_maxCollisions(100)
	{}
	OADHashTable(THash1 hash1, THash2 hash2, TKeyEq keyEq, double maxLoad = 0.8) :
	m_d(16, 0), //start with a small hash table, start value has to obey (m_d.size()+1)/(m_rehashMult*m_d.size()) < m_maxLoad
	m_maxLoad(maxLoad),
	m_rehashMult(2.0),
	m_maxCollisions(100),
	m_hash1(hash1),
	m_hash2(hash2),
	m_keyEq(keyEq)
	{}
	OADHashTable(THash1 hash1, THash2 hash2, TKeyEq keyEq, double maxLoad, const ValueStorageType & valueStorage, const TableStorageType & tableStorage) :
	m_valueStorage(valueStorage),
	m_d(tableStorage),
	m_maxLoad(maxLoad),
	m_rehashMult(2.0),
	m_maxCollisions(100),
	m_hash1(hash1),
	m_hash2(hash2),
	m_keyEq(keyEq)
	{
		m_valueStorage.clear();
		m_d.clear();
		m_d.resize(16, 0);
	}
	OADHashTable(const ValueStorageType & valueStorage, const TableStorageType & tableStorage) :
	m_valueStorage(valueStorage),
	m_d(tableStorage),
	m_maxLoad(0.8),
	m_rehashMult(2.0),
	m_maxCollisions(100)
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
	m_maxCollisions(other.m_maxCollisions),
	m_hash1(other.m_hash1),
	m_hash2(other.m_hash2),
	m_keyEq(other.m_keyEq)
	{}
	virtual ~OADHashTable() {}
	void clear() {
		m_valueStorage.clear();
		m_d.clear();
	}
	inline SizeType size() const { return m_valueStorage.size(); }
	///Capacity of the storage table
	inline SizeType storageCapacity()  const { return m_valueStorage.capacity();}
	inline double rehashMultiplier() const { return m_rehashMult;}
	inline void rehashMultiplier(double v) { m_rehashMult = v; }
	///Capacity of the hash table (not the storage table)
	inline uint64_t capacity() const { return m_d.capacity();}
	inline double load_factor() const { return (double)size()/m_d.size();}
	inline double max_load_factor() const { return m_maxLoad;}
	void max_load_factor(double f);
	///Set this to std::numeric_limits<uint64_t>::max() if you want as many collisions as the size of the table
	inline void maxCollisions(uint64_t count) { m_maxCollisions = count; rehash(std::max<uint64_t>(m_d.size(), 16)); }
	inline uint64_t maxCollisions() const { return m_maxCollisions; }
	///if you change something here, then you're on your own
	inline THash1 & hash1() { return m_hash1; }
	inline const THash1 & hash1() const { return m_hash1; }
	///if you change something here, then you're on your own
	inline THash2 & hash2() { return m_hash2; }
	inline const THash2 & hash2() const { return m_hash2; }
	
	void reserve(SizeType count);
	///Adding more elements to the hash does not invalidate iterators
	mapped_type & operator[](const key_type & key);
	void insert(const key_type & key);
	mapped_type & at(const key_type & key);
	const mapped_type & at(const key_type & key) const;
	inline bool count(const key_type & key) const {
		uint64_t pos = findBucket(key);
		return (pos == findend ? false : bool(m_d[pos]));
	}
	const_iterator find(const key_type & key) const;
	iterator find(const key_type & key);
	
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
	
	///Multi threaded sort, see @sort
	template<typename T_SORT_OP>
	void mt_sort(T_SORT_OP op, unsigned int numThreads = 0);
	
};

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
uint64_t
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::findBucket(const key_type & key) const {
	uint64_t h1 = (m_hash1(key)) & ~0x1;
	uint64_t s = m_d.size();
	uint64_t cpos = h1%s;
	SizeType cp = m_d[cpos];
	if (!cp || m_keyEq(value(cp).first, key) ) {
		return cpos;
	}
	uint64_t h2 = (m_hash2(key) & ~0x1);
	for(uint64_t i = 1, maxCollisions = std::min<uint64_t>(s, m_maxCollisions); i < maxCollisions; ++i) {
		cpos = (h1 + i*h2) % s;
		SizeType cp = m_d[cpos];
		if (!cp || m_keyEq(value(cp).first, key) ) {
			return cpos;
		}
	}
	return findend;
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::rehash(uint64_t count) {
	count = count | 0x1;
	if (!size() || count/size() > 10) {
		sserialize::info("sserialize::OADHashTable::rehash", sserialize::toString("load_factor dropped to ", (double)size()/count));
	}
	m_d.clear();
	m_d.resize(count, 0);
	for(SizeType i = 1, s = size(); i <= s; ++i) {
		uint64_t pos = findBucket(value(i).first);
		if (pos != findend) {
			m_d[pos] = i;
		}
		else {
			rehash(count*m_rehashMult);
			return;
		}
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::max_load_factor(double f) {
	m_maxLoad = f;
	if (load_factor() > m_maxLoad) {
		uint64_t targetSize = m_d.size()*m_rehashMult;
		while ( (double)size()/targetSize > m_maxLoad) {
			targetSize = targetSize*m_rehashMult;
		}
		rehash(targetSize);
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::reserve(SizeType count) {
	double newTableSize = count/m_maxLoad;
	if ((double)size()/newTableSize < m_maxLoad) {
		rehash(newTableSize);
	}
	m_valueStorage.reserve(count);
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::operator[](const key_type & key) {
	uint64_t pos = findBucket(key);
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
		auto mySize = m_valueStorage.size();
		cp = mySize;
		if ( mySize != cp ) {//get optimized our if return_type(m_valueStorage.size()) == SizeType
			throw std::out_of_range("OADHashTable: overflow in TableStorage pointer type. Too many elements in hash.");
		}
		return value(cp).second;
	}
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
void
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::insert(const key_type & key) {
	operator[](key);
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::at(const key_type & key) {
	uint64_t pos = findBucket(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range("OADHashTable::at");
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
const TValue &
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::at(const key_type & key) const {
	uint64_t pos = findBucket(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return value(cp).second;
	}
	throw std::out_of_range("OADHashTable::at");
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
template<typename T_SORT_OP>
void OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::sort(T_SORT_OP op) {
	std::sort(m_valueStorage.begin(), m_valueStorage.end(), op);
	rehash(m_d.size());
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
template<typename T_SORT_OP>
void OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::mt_sort(T_SORT_OP op, unsigned int numThreads) {
	sserialize::mt_sort(m_valueStorage.begin(), m_valueStorage.end(), op, numThreads);
	rehash(m_d.size());
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
typename OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::const_iterator
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::find(const key_type & key) const {
	uint64_t pos = findBucket(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return valueIt(cp);
	}
	return end();
}

template<typename TKey, typename TValue, typename THash1, typename THash2, typename TValueStorageType, typename TTableStorageType, typename TKeyEq>
typename OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::iterator
OADHashTable<TKey, TValue, THash1, THash2, TValueStorageType, TTableStorageType, TKeyEq>::find(const key_type & key) {
	uint64_t pos = findBucket(key);
	SizeType & cp = m_d[pos]; //saves some calls  m_d[]
	if (pos != findend && cp) {
		return valueIt(cp);
	}
	return end();
}

}//end namespace

#endif