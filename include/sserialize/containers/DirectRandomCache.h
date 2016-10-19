#ifndef SSERIALIZE_DIRECT_RANDOM_CACHE_H
#define SSERIALIZE_DIRECT_RANDOM_CACHE_H
#include <sserialize/utility/assert.h>
#include <limits>
#include <vector>
#include <set>
#include <stdlib.h>

template<typename TValue>
class DirectRandomCache {
public:
	typedef typename std::vector<TValue>::iterator CacheIterator;
	typedef typename std::vector<TValue>::const_iterator ConstCacheIterator;
	typedef std::set<uint32_t>::const_iterator OccupyIterator;
private:
	std::vector<TValue> m_cache;
	std::set<uint32_t> m_occupied;
	TValue m_defaultValue;
public:
	DirectRandomCache() {}
	DirectRandomCache(uint32_t cacheLineCount, const TValue & defaultValue) : m_cache(cacheLineCount, defaultValue), m_defaultValue(defaultValue) {}
	/// It's your task to take of correct deletion of the values! */
	virtual ~DirectRandomCache() {}
	
	uint32_t occupyCount() const { return (uint32_t) m_occupied.size(); }
	
	/** Tries to find a victim cache line
	  * @return the victim cache line, std::numeric_limits<uint32_t>::max() on failure
	  */
	uint32_t findVictim() {
		if (occupyCount() == 0)
			return std::numeric_limits<uint32_t>::max();
		uint32_t pos = rand() % occupyCount();
		std::set<uint32_t>::const_iterator sIt(m_occupied.cbegin());
		while (pos) {
			--pos;
			++sIt;
		}
		return *sIt;
	}

	///evcit
	void evict(uint32_t pos) {
		m_occupied.erase(pos);
		m_cache[pos] = m_defaultValue;
	}
	
	///Does not check if cache line is valid!
	TValue & operator[](const uint32_t cacheLine) {
		return m_cache[cacheLine];
	}
	
	///Does not check if cache line is valid and does not change usage
	TValue & directAccess(const uint32_t cacheLine) {
		return m_cache[cacheLine];
	}
	
	bool occupied(const uint32_t cacheLine) const {
		return m_occupied.count(cacheLine);
	}
	
	/** Inserts @param value into @param cacheLine, inital usage=1 */
	void insert(const uint32_t cacheLine, const TValue & value) {
		m_cache[cacheLine] = value;
		m_occupied.insert(cacheLine);
		SSERIALIZE_CHEAP_ASSERT_SMALLER(m_occupied.size(), (std::size_t) std::numeric_limits<uint32_t>::max());
	}
	///Clears the cache
	void clear() {
		m_cache.assign(m_cache.size(), m_defaultValue);
		m_occupied.clear();
	}
};

#endif