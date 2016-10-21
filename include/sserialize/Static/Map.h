#ifndef SSERIALIZE_STATIC_MAP_H
#define SSERIALIZE_STATIC_MAP_H
#include <sserialize/Static/Array.h>
#include <sserialize/Static/Pair.h>

/** FileFormat
 *
 *-------------
 *Static::Array
 *-------------
 *
 * Use Static::Array as basis, remove version information (array is on version 4)
 *
 *
 */

namespace sserialize {
namespace Static {
 
template<typename TKey, typename TValue>
class Map {
public:
	typedef Map<TKey, TValue> type;
	typedef TKey key_type;
	typedef TValue mapped_type;
	typedef sserialize::Static::Pair<TKey, TValue> value_type;
	typedef typename sserialize::Static::Array<value_type>::iterator iterator;
	typedef typename sserialize::Static::Array<value_type>::const_iterator const_iterator; 
	typedef sserialize::SizeType size_type;
	static constexpr size_type npos = std::numeric_limits<size_type>::max();
private:
	sserialize::Static::Array<value_type> m_data;
public:
	Map();
	Map(const UByteArrayAdapter & data);
	~Map() {}
	iterator begin() { return m_data.begin(); }
	const_iterator begin() const { return m_data.begin(); }
	const_iterator cbegin() const { return m_data.cbegin(); }
	iterator end() { return m_data.end(); }
	const_iterator end() const { return m_data.end(); }
	const_iterator cend() const { return m_data.cend(); }
	
	inline sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const {return m_data.getSizeInBytes();}
	inline size_type size() const { return m_data.size();}
	bool contains(const key_type & key) const;
	mapped_type at(const key_type & key) const;
	value_type atPosition(const size_type pos) const;
	value_type find(const key_type & key) const;
	size_type
	findPosition(const key_type & key) const;
};

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::size_type
Map<TKey, TValue>::findPosition(const TKey & key) const {
	if (! size())
		return npos;
	int64_t len = m_data.size()-1;
	int64_t left = 0;
	int64_t right = len;
	int64_t mid = (right-left)/2+left;
	while( left < right ) {
		Pair<TKey, TValue> pair(atPosition(mid));
		int8_t cmp = pair.compareWithFirst(key);
		if (cmp == 0)
			return mid;
		if (cmp < 0) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	Pair<TKey, TValue> pair(atPosition(mid));
	return (pair.compareWithFirst(key) == 0 ? mid : npos);
}

template<typename TKey, typename TValue>
Map<TKey, TValue>::Map() {}

template<typename TKey, typename TValue>
Map<TKey, TValue>::Map(const UByteArrayAdapter & data) :
m_data(data)
{}

template<typename TKey, typename TValue>
bool
Map<TKey, TValue>::contains(const TKey & key) const {
	return (findPosition(key) >= 0);
}

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::mapped_type
Map<TKey, TValue>::at(const TKey & key) const {
	size_type pos = findPosition(key);
	if (pos == npos) {
		return TValue();
	}
	else {
		return atPosition(pos).second();
	}
}


template<typename TKey, typename TValue>
typename Map<TKey, TValue>::value_type
Map<TKey, TValue>::atPosition(const size_type pos) const {
	return m_data.at(pos);
}

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::value_type
Map<TKey, TValue>::find(const TKey & key) const {
	size_type pos = findPosition(key);
	if (pos == npos) {
		return value_type();
	}
	else {
		return atPosition(pos);
	}
}

template<typename TKey, typename TValue>
bool operator==(const sserialize::Static::Map<TKey, TValue> & mapA, const sserialize::Static::Map<TKey, TValue> & mapB) {
	if (mapA.size() != mapB.size())
		return false;
	uint32_t size = mapA.size();
	for(uint32_t i = 0; i < size; i++) {
		if (mapA.atPosition(i) != mapB.atPosition(i))
			return false;
	}
	return true;
}

template<typename TKey, typename TValue>
bool operator==(const sserialize::Static::Map<TKey, TValue> & mapA, const std::map<TKey, TValue> & mapB) {
	if (mapA.size() != mapB.size())
		return false;
	uint32_t count = 0;
	for(typename std::map<TKey, TValue>::const_iterator it = mapB.begin(); it != mapB.end(); it++) {
		const sserialize::Static::Pair<TKey, TValue> & sv = mapA.atPosition(count);
		const std::pair<TKey, TValue> & mv = *it;
		if (sv != mv)
			return false;
		count++;
	}
	return true;
}

template<typename TKey, typename TValue>
bool operator==(const std::map<TKey, TValue> & mapA, const sserialize::Static::Map<TKey, TValue> & mapB) {
	return mapB == mapA;
}

template<typename TKey, typename TValue>
bool operator<(const sserialize::Static::Map<TKey, TValue> & mapA, const sserialize::Static::Map<TKey, TValue> & mapB) {
	uint32_t itA = 0;
	uint32_t itB = 0;
	while (itA<mapA.size()) {
		if (mapB.size() <= itB || mapB.at(itB) < mapA.at(itA)) {
			return false;
		}
		TValue vA(mapA.at(itA));
		TValue vB(mapB.at(itB));
		if (vB < vA) {
			return false;
		}
		else if (vA < vB)
			return true;
		itA++;
		itB++;
	}
	return (itB < mapB.size());
}

template<typename TKey, typename TValue>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::Static::Map<TKey, TValue> & destination) {
	sserialize::UByteArrayAdapter d(source);
	d.shrinkToGetPtr();
	destination = sserialize::Static::Map<TKey, TValue>(d);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}


}//end namespace Static

template<typename TKey, typename TValue>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::map<TKey, TValue> & map) {
	sserialize::Static::ArrayCreator< std::pair<TKey, TValue>  > ac(destination);
	for(const std::pair<TKey, TValue> & x : map) {
		ac.put(x);
	}
	ac.flush();
	return destination;
}

}//end namespace sserialize

#endif