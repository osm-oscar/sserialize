#ifndef LFUCACHE_H
#define LFUCACHE_H
#include <unordered_map>
#include <utility>
#include <sserialize/utility/hashspecializations.h>

namespace sserialize {

template<typename TKey, typename TValue>
class LFUCache {
private:
	typedef typename std::unordered_map<TKey, std::pair<uint32_t, TValue> >::iterator CacheIterator;
	struct CacheEntry {
		typedef uint32_t CounterType;
		TValue value;
		CounterType usage;
		CounterType lastAccess;
	};
private:
	uint32_t m_maxSize;
	std::unordered_map<TKey, std::pair<uint32_t, TValue> > m_cache;
private:
	CacheIterator evict() {
		if (m_cache.size() == 0)
			return m_cache.end();
		uint32_t minUsage = 0xFFFFFFFF;
		CacheIterator minUsageIt = m_cache.begin();
		for(CacheIterator it = minUsageIt; it != m_cache.end(); it++) {
			if (it->second.first < minUsage) {
				minUsageIt = it;
				minUsage = it->second.first;
			}
		}
		//we defentetly have one here
		return minUsageIt;
	}
public:
	LFUCache() : m_maxSize(16) {}
	~LFUCache() {}
	void setSize(const uint32_t size) {
		m_maxSize = size;
		if (m_maxSize == 0) {
			m_maxSize = 1;
		}
		while (m_cache.size() > m_maxSize) {
			evict();
		}
	}

	uint64_t maxSize() {
		return m_maxSize;
	}

	uint32_t size() {
		return m_cache.size();
	}
	
	void insert(const TKey & key, const TValue & value) {
		if (m_cache.size()+1 >= m_maxSize) {
			m_cache.erase(evict()); //There defenitly is one there as m_maxSize > 0
		}
		m_cache.insert(std::pair<TKey, std::pair<uint32_t, TValue> >(key, std::pair<uint32_t, TValue>(0, value)) );
	}
	
	bool needsEviction() const {
		return (m_cache.size()+1 >= m_maxSize);
	}
	
	/** Tries to evict a cached object */
	void evict(TKey & key, TValue & value) {
		if (m_cache.size()) {
			CacheIterator it = evict();
			key = it->first;
			value = it->second.second;
		}
	}
	
	/** Find a key and increment its counter **/
	TValue find(const TKey & key) {
		CacheIterator it = m_cache.find(key);
		if (it == m_cache.end()) {
			return TValue();
		}
		m_cache[key].first += 1;
		return m_cache[key].second;
	}

	inline bool contains(const TKey & key) {
		return (m_cache.count(key) > 0);
	}
	
	inline size_t count(const TKey & key) {
		return m_cache.count(key);
	}

	inline void clear() {
		m_cache.clear();
	}
};

}

#endif