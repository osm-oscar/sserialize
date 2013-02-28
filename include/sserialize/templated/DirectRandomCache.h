#ifndef SSERIALIZE_DIRECT_RANDOM_CACHE_H
#define SSERIALIZE_DIRECT_RANDOM_CACHE_H
#include <limits>
#include <vector>
#include <set>
#include <stdlib.h>

/** This is a DirectMapped LFU cache with up std::size_t entries (underlying storage is a simple std::vector<TKey>
  * It is up to you to do the eviction if necessary. There are helper functions to do this
  * Furthermore its up to you to make sure that multiple copies of a cache don't interfere with each other! (i.e. if you store pointers)
  */
template<typename TValue>
class RandomCache {
public:
	typedef typename std::vector<TValue>::iterator CacheIterator;
	typedef typename std::vector<TValue>::const_iterator ConstCacheIterator;
	typedef std::set<uint32_t>::const_iterator OccupyIterator;
private:
	std::vector<TValue> m_cache;
	std::vector<uint32_t> m_cacheLine;
public:
	RandomCache() {}
	RandomCache(uint32_t reserve) {
		m_cache.reserve(reserve);
		m_cacheLine.reserve(reserve);
	}
	/// It's your task to take of correct deletion of the values! */
	virtual ~RandomCache() {}
	
	uint32_t occupyCount() const { return m_cache.size();}
	
	/** Tries to find a victim cache line
	  * @return the victim cache line, std::numeric_limits<uint32_t>::max() on failure
	  */
	uint32_t findVictim() {
		if (occupyCount() == 0)
			return std::numeric_limits<uint32_t>::max();
		return rand() % occupyCount();
	}

	///evcit
	void evict(uint32_t pos) {
		m_occupied.erase(cacheLine);
		m_cache[cacheLine] = m_defaultValue;
	}
	
	///Does not check if cache line is valid!
	TValue & operator[](const uint32_t cacheLine) {
		m_usage[cacheLine] += 1;
		return m_cache[cacheLine];
	}
	
	///Does not check if cache line is valid and does not change usage
	TValue & directAccess(const uint32_t cacheLine) {
		return m_cache[cacheLine];
	}
	
	bool occupied(const uint32_t cacheLine) const { return m_occupied.count(cacheLine); }
	
	/** Inserts @param value into @param cacheLine, inital usage=1 */
	void insert(const uint32_t cacheLine, const TValue & value) {
		m_usage[cacheLine] = 1;
		m_cache[cacheLine] = value;
		m_occupied.insert(cacheLine);
	}
	///Clears the cache
	void clear() {
		m_cache.assign(m_cache.size(), m_defaultValue);
		m_usage.assign(m_usage.size(), 0);
		m_occupied.clear();
	}
};

#endif