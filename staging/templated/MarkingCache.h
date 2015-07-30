#ifndef SSERIALIZE_MARKING_CACHE_H
#define SSERIALIZE_MARKING_CACHE_H
#include <sserialize/algorithm/utilmath.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <unordered_map>
#include <vector>

namespace sserialize {

///A simple Marking-Cache. The TMarkerType specfies the number of cache lines (uintNUM_CACHE_LINES_t)
///Maximum CacheSize is 2**16-1
template<typename TKey, typename TValue, typename TMarkerType = uint64_t>
class MarkingCache {
private:
	struct CacheEntry {
		TValue value;
		uint8_t m_pos;
	};
	typedef std::unordered_set<uint8_t> FreeSet;
	typedef std::unordered_map<TKey, CacheEntry > CacheContainer;
	typedef typename CacheContainer::iterator CacheIterator;
private:
	CacheContainer m_cache;
	FreeSet m_freeSet;
	TMarkerType m_usageMarks;
	TMarkerType m_occupyMarks;
	TKey m_defaultKey;
private:
	CacheIterator findVictim() {
		if (m_cache.size() < std::numeric_limits<TMarkerType>::digits)
			return m_cache.end();
		TMarkerType candidates = ~m_usageMarks & m_occupyMarks;
		uint8_t candidateCount = popCount<TMarkerType>(candidates);
		uint8_t selectedCandidate = rand() % candidateCount;
		
		for(uint8_t skippedCandidates = 0, candidatePos = 0; skippedCandidates != selectedCandidate; ++candidatePos) {
			
		}
		
		CacheIterator evictIt = m_cache.begin();
		return evictIt;
	}
	
	void evict(CacheIterator it) {
		m_cache.erase(it);
	}
	
	inline TMarkerType cachePosBitAcc(uint8_t pos) const { return static_cast<TMarkerType>(1) << pos; }
	
	inline void updateUsage(uint8_t pos) {
		TMarkerType tmp = cachePosBitAcc(pos);
		m_usageMarks |= tmp;
		m_usageMarks = ((~(m_usageMarks | ~m_occupyMarks)) ? m_usageMarks : tmp);//reset marks if all marks are set
	}
public:
	MarkingCache() : m_freeSet(RangeGenerator(0, 16).begin(), RangeGenerator(0, 16).end()) {}
	~MarkingCache() {}
	///dummy function, maxSize is selected by TMarkerType
	void setSize(const uint8_t size) {}

	uint8_t maxSize() {
		return std::numeric_limits<TMarkerType>::digits;
	}

	uint32_t size() {
		return m_cache.size();
	}
	
	///Call this only if key is not already in the cache, otherwise an eviction is uneccasarily done
	void insert(const TKey & key, const TValue & value) {
		if (m_cache.size()+1 >= std::numeric_limits<TMarkerType>::digits) {
			m_cache.erase(evict()); //There defenitly is one there as m_maxSize > 0
		}
		uint8_t freePos = *m_freeSet.begin();
		m_freeSet.erase(m_freeSet.begin());
		TMarkerType tmp = cachePosBitAcc(freePos);
		m_occupyMarks |= tmp;
		m_usageMarks |= tmp;
		m_cache[key] = CacheEntry(value, freePos);
	}
	
	bool needsEviction() const {
		return (m_cache.size()+1 >= std::numeric_limits<TMarkerType>::digits);
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
		return it->second.value;
	}

	inline bool contains(const TKey & key) {
		return (m_cache.count(key) > 0);
	}
	
	inline size_t count(const TKey & key) {
		return m_cache.count(key);
	}

	inline void clear() {
		m_cache.clear();
		m_occupyMarks = 0;
		m_usageMarks = 0;
	}
};

}//end namespace

#endif