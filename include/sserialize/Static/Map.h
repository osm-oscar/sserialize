#ifndef SSERIALIZE_STATIC_MAP_H
#define SSERIALIZE_STATIC_MAP_H
#include <map>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/Pair.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/AtStlInputIterator.h>
#define SSERIALIZE_STATIC_MAP_VERSION 2

/** FileFormat
 *
 *------------------------------------------------------------
 *VERSION|SIZE|SortedOffsetIndex|Data*
 *------------------------------------------------------------
 * 1byte | 5  | *               | *
 *
 * SIZE=size of Data
 *
 */

namespace sserialize {
namespace Static {
 
template<typename TKey, typename TValue>
class Map {
private:
	struct IteratorDerefer {
		Static::Pair<TKey, TValue> operator()(const Map<TKey, TValue> * map, uint32_t pos);
	};
public:
	typedef Map<TKey, TValue> type;
	typedef TKey key_type;
	typedef TValue mapped_type;
	typedef sserialize::Static::Pair<TKey, TValue> value_type;
	typedef sserialize::ReadOnlyAtStlIterator< type *, value_type > iterator;
	typedef sserialize::ReadOnlyAtStlIterator< const type *, value_type > const_iterator; 
	typedef uint32_t size_type;
	static const size_type npos = std::numeric_limits<size_type>::max();
private:
	UByteArrayAdapter m_data;
	SortedOffsetIndex m_index;
public:
	Map();
	Map(const UByteArrayAdapter & data);
	~Map() {}
	iterator begin() { return iterator(0, this); }
	const_iterator cbegin() const { return const_iterator(0, this); }
	iterator end() { return iterator(size(), this); }
	const_iterator cend() const { return const_iterator(size(), this); }
	
	inline sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const {  return 1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_index.getSizeInBytes()+m_data.size();}
	inline size_type size() const { return m_index.size();}
	bool contains(const key_type & key) const;
	mapped_type at(const key_type & key) const;
	value_type atPosition(const size_type pos) const;
	value_type find(const key_type & key) const;
	int32_t findPosition(const key_type & key) const;
};

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::value_type
Map<TKey, TValue>::IteratorDerefer::operator()(const Map<TKey, TValue> * map, uint32_t pos) {
	return map->atPosition(pos);
}

template<typename TKey, typename TValue>
int32_t
Map<TKey, TValue>::findPosition(const TKey & key) const {
	if (! size())
		return -1;
	int32_t len = m_index.size()-1;
	int32_t left = 0;
	int32_t right = len;
	int32_t mid = (right-left)/2+left;
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
	return (pair.compareWithFirst(key) == 0 ? mid : -1);
}

template<typename TKey, typename TValue>
Map<TKey, TValue>::Map() {}

template<typename TKey, typename TValue>
Map<TKey, TValue>::Map(const UByteArrayAdapter & data) :
m_index(data+(1+UByteArrayAdapter::OffsetTypeSerializedLength()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_MAP_VERSION, data.at(0), "Static::Map");
	m_data = UByteArrayAdapter(data,(1 + UByteArrayAdapter::OffsetTypeSerializedLength() + m_index.getSizeInBytes()), data.getOffset(1));
	SSERIALIZE_LENGTH_CHECK(2*m_index.size(), m_data.size(), "Static::Map::Map::Insufficient data");
}

template<typename TKey, typename TValue>
bool
Map<TKey, TValue>::contains(const TKey & key) const {
	return (findPosition(key) >= 0);
}

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::mapped_type
Map<TKey, TValue>::at(const TKey & key) const {
	int32_t pos = findPosition(key);
	if (pos < 0) {
		return TValue();
	}
	else {
		return atPosition(pos).second();
	}
}


template<typename TKey, typename TValue>
typename Map<TKey, TValue>::value_type
Map<TKey, TValue>::atPosition(const uint32_t pos) const {
	if (pos >= m_index.size()) {
		return Pair<TKey, TValue>();
	}
	else {
		return Pair<TKey, TValue>(m_data + m_index.at(pos));
	}
}

template<typename TKey, typename TValue>
typename Map<TKey, TValue>::value_type
Map<TKey, TValue>::find(const TKey & key) const {
	int32_t pos = findPosition(key);
	if (pos < 0) {
		return Pair<TKey, TValue>();
	}
	else {
		return atPosition(pos);
	}
}

}}//end namespace

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
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::map<TKey, TValue> & map) {
	std::vector<uint8_t> tmpStore;
	sserialize::UByteArrayAdapter tmpValueStore(&tmpStore);
	std::set<sserialize::OffsetType> offSets;
	for(typename std::map<TKey, TValue>::const_iterator it = map.begin(); it != map.end(); it++) {
		offSets.insert(tmpValueStore.tellPutPtr());
		tmpValueStore << *it;
	}
	sserialize::UByteArrayAdapter indexData(new std::vector<uint8_t>, true);
	sserialize::Static::SortedOffsetIndexPrivate::create(offSets, indexData);
	
	//Add everything to our adapter
	destination.putUint8(SSERIALIZE_STATIC_MAP_VERSION); //version
	destination.putOffset(tmpStore.size());
	destination.put(indexData);
	destination.put(tmpStore);
	return destination;
}

template<typename TKey, typename TValue>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::Static::Map<TKey, TValue> & destination) {
	sserialize::UByteArrayAdapter tmpAdap(source);
	tmpAdap.shrinkToGetPtr();
	destination = sserialize::Static::Map<TKey, TValue>(tmpAdap);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}

#endif