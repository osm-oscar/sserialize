#ifndef SSERIALIZE_DIRECT_LRU_CACHE_H
#define SSERIALIZE_DIRECT_LRU_CACHE_H
#include <limits>
#include <vector>
#include <set>

/** This is a DirectMapped LFU cache with up std::size_t entries (underlying storage is a simple std::vector<TKey>
  * It is up to you to do the eviction if necessary. There are helper functions to do this
  * Furthermore its up to you to make sure that multiple copies of a cache don't interfere with each other! (i.e. if you store pointers)
  */
template<typename TValue>
class DirectLRUCache {
public:
	typedef typename std::vector<TValue>::iterator CacheIterator;
	typedef typename std::vector<TValue>::const_iterator ConstCacheIterator;
	typedef std::set<uint32_t>::const_iterator OccupyIterator;
public:
	struct Info {
		Info(uint32_t at, const TValue & v) : accessTime(at), value(v) {}
		uint32_t accessTime;
		TValue value;
	};
private:
	uint32_t m_time;
	std::vector<Info> m_cache;
	std::set<uint32_t> m_occupied;
	TValue m_defaultValue;
private:
	void normalizeTime() {
		if (!occupyCount())
			return;
		m_time = 1;
		uint32_t minAT = std::numeric_limits<uint32_t>::max();
		uint32_t maxAT = std::numeric_limits<uint32_t>::min();
		OccupyIterator ocEnd( m_occupied.end() );
		for(OccupyIterator it( m_occupied.begin() ); it != ocEnd; ++it) {
			Info & info = m_cache[*it];
			minAT = std::min<uint32_t>(minAT, info.accessTime);
			maxAT = std::min<uint32_t>(maxAT, info.accessTime);
		}
		uint32_t diffAT = maxAT-minAT;
		uint32_t destDiffAt = 2*m_occupied.size() - 2;
		
		//now map the accessTime range to 2 - 2*m_occupied.size()
		for(OccupyIterator it( m_occupied.begin() ); it != ocEnd; ++it) {
			Info & info = m_cache[*it];
			info.accessTime = ((double)(info.accessTime-minAT))/diffAT * destDiffAt + 2;
		}
	}
public:
	DirectLRUCache() : m_time(1) {}
	DirectLRUCache(uint32_t cacheLineCount, const TValue & defaultValue) : m_time(1), m_defaultValue(defaultValue) {
		m_cache.resize(cacheLineCount, Info(0, m_defaultValue));
	}
	/// It's your task to take of correct deletion of the values! */
	virtual ~DirectLRUCache() {}
	
	uint32_t occupyCount() const { return m_occupied.size();}
	
	/** Tries to find a victim cache line
	  * @return the victim cache line, std::numeric_limits<uint32_t>::max() on failure
	  */
	uint32_t findVictim() {
		uint32_t cacheLine = std::numeric_limits<uint32_t>::max();
		uint32_t minTime = std::numeric_limits<uint32_t>::max();
		OccupyIterator ocEnd( m_occupied.end() );
		for(OccupyIterator it( m_occupied.begin() ); it != ocEnd; ++it) {
			Info & info = m_cache[*it];
			if (info.accessTime <= minTime) {
				cacheLine = *it;
				minTime = info.accessTime;
			}
		}
		return cacheLine;
	}

	///evict the given cache line
	void evict(uint32_t cacheLine) {
		m_occupied.erase(cacheLine);
		Info & info = m_cache[cacheLine];
		info.accessTime = 0;
		info.value = m_defaultValue;
	}
	
	///Does not check if cache line is valid!
	TValue & operator[](const uint32_t cacheLine) {
		++m_time;
		Info & info = m_cache[cacheLine];
		info.accessTime = m_time;
		if (m_time) { //this should happen almost eveyt time
			return info.value;
		}
		normalizeTime();
		return info.value;
	}
	
	///Does not check if cache line is valid and does not change usage
	TValue & directAccess(const uint32_t cacheLine) {
		return m_cache[cacheLine].value;
	}
	
	bool occupied(const uint32_t cacheLine) const { return m_occupied.count(cacheLine); }
	
	/** Inserts @param value into @param cacheLine, inital usage=1 */
	void insert(const uint32_t cacheLine, const TValue & value) {
		Info & info = m_cache[cacheLine];
		info.value = value;
		info.accessTime = m_time;
		m_occupied.insert(cacheLine);
	}
	
	///Clears the cache
	void clear() {
		uint32_t size = m_cache.size();
		for(uint32_t i = 0; i < size; ++i) {
			Info & info = m_cache[i];
			info.accessTime = 0;
			info.value = m_defaultValue;
		}
	}
};

#endif