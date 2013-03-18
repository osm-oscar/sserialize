#ifndef SSERIALIZE_DIRECT_FIFO_CACHE_H
#define SSERIALIZE_DIRECT_FIFO_CACHE_H
#include <limits>
#include <vector>
#include <list>
#include <stdlib.h>

template<typename TValue>
class DirectFIFOCache {
public:
	typedef typename std::vector<TValue>::iterator CacheIterator;
	typedef typename std::vector<TValue>::const_iterator ConstCacheIterator;
	typedef std::list<uint32_t>::const_iterator OccupyIterator;
private:
	std::vector<TValue> m_cache;
	std::list<uint32_t> m_occupied;
	TValue m_defaultValue;
public:
	DirectFIFOCache() {}
	DirectFIFOCache(uint32_t cacheLineCount, const TValue & defaultValue) : m_cache(cacheLineCount, defaultValue), m_defaultValue(defaultValue) {}
	/// It's your task to take of correct deletion of the values! */
	virtual ~DirectFIFOCache() {}
	
	uint32_t occupyCount() const { return m_occupied.size(); }
	
	/** Tries to find a victim cache line
	  * @return the victim cache line, std::numeric_limits<uint32_t>::max() on failure
	  */
	uint32_t findVictim() {
		if (occupyCount() == 0)
			return std::numeric_limits<uint32_t>::max();
		return m_occupied.front();
	}

	///evcit, evicts the FRONT!
	void evict(uint32_t pos) {
		m_occupied.pop_front();
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
		std::list<uint32_t>::const_iterator end(m_occupied.end());
		for(std::list<uint32_t>::const_iterator it(m_occupied.begin()); it != end; ++it) {
			if (*it == cacheLine)
				return true;
		}
		return false;
	}
	
	/** Inserts @param value into @param cacheLine, inital usage=1 */
	void insert(const uint32_t cacheLine, const TValue & value) {
		m_cache[cacheLine] = value;
		m_occupied.push_back(cacheLine);
	}
	///Clears the cache
	void clear() {
		m_cache.assign(m_cache.size(), m_defaultValue);
		m_occupied.clear();
	}
};

#endif