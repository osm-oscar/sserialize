#ifndef SSERIALIZE_STATIC_SET_H
#define SSERIALIZE_STATIC_SET_H
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <string>
#include <set>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/AtStlInputIterator.h>
#define SSERIALIZE_STATIC_SET_VERSION 1

/** File format
 *
 *----------------------------------------------------------------------
 *VERSION|SIZE|SortedOffsetIndex|Data+
 *----------------------------------------------------------------------
 *   1   | 4  |  *              |  *
 *
 *
 * If type is int32_t, uint32_t, uint16_t, uint8_t, itemindex is not present
 * Otherwise: SortedOffsetIndex encodes offsets
 * size is length in bytes of data
 * 
 * TODO:base this on Static::Array since this is essentialy a sorted array
 * 
 */

namespace sserialize {
namespace Static {
 
template<typename TValue>
class Set {
public:
	typedef TValue value_type;
	typedef sserialize::ReadOnlyAtStlIterator< sserialize::Static::Set<TValue>*, TValue > iterator;
	typedef sserialize::ReadOnlyAtStlIterator< const sserialize::Static::Set<TValue>*, TValue > const_iterator; 
private:
	SortedOffsetIndex m_index;
	UByteArrayAdapter m_data;
public:
	Set();
	Set(const UByteArrayAdapter & data);
	~Set() {}
	iterator begin() { return iterator(0, this); }
	const_iterator cbegin() const { return const_iterator(0, this); }
	iterator end() { return iterator(size(), this); }
	const_iterator cend() const { return const_iterator(size(), this); }
	
	inline UByteArrayAdapter::OffsetType getSizeInBytes() const {  return 5+m_index.getSizeInBytes()+m_data.size();}
	uint32_t size() const;
	int32_t find(const TValue & value) const;
	bool contains(const TValue & value) const;
	TValue at(uint32_t pos) const;
};

template<typename TValue>
int32_t
Set<TValue>::find(const TValue & value) const {
	if (! size() )
		return -1;
	int32_t len = size()-1;
	int32_t left = 0;
	int32_t right = len;
	int32_t mid = (right-left)/2+left;
	while( left < right ) {
		TValue tmpV(at(mid));
		if (tmpV == value)
			return mid;
		if (tmpV < value) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	return (at(mid) == value ? mid : -1);
}

template<typename TValue>
Set<TValue>::Set() {}

template<typename TValue>
Set<TValue>::Set(const UByteArrayAdapter & data) :
m_index(data+5)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SET_VERSION, data.at(0), "Static::Set");
	m_data = UByteArrayAdapter(data, 5+m_index.getSizeInBytes(), data.getUint32(1));
	SSERIALIZE_LENGTH_CHECK(m_index.size(), m_data.size(), "Static::Set::Set::Insufficient data");
}

template<>
Set<int32_t>::Set(const UByteArrayAdapter & data);

template<>
Set<uint32_t>::Set(const UByteArrayAdapter & data);

template<>
Set<uint16_t>::Set(const UByteArrayAdapter & data);

template<>
Set<uint8_t>::Set(const UByteArrayAdapter & data);

template<typename TValue>
uint32_t
Set<TValue>::size() const {
	return m_index.size();
}

template<>
uint32_t
Set<int32_t>::size() const;

template<>
uint32_t
Set<uint32_t>::size() const;

template<>
uint32_t
Set<uint16_t>::size() const;

template<>
uint32_t
Set<uint8_t>::size() const;

template<>
UByteArrayAdapter::OffsetType
Set<uint8_t>::getSizeInBytes() const;

template<>
UByteArrayAdapter::OffsetType
Set<int32_t>::getSizeInBytes() const;

template<>
UByteArrayAdapter::OffsetType
Set<uint32_t>::getSizeInBytes() const;

template<>
UByteArrayAdapter::OffsetType
Set<uint16_t>::getSizeInBytes() const;

template<typename TValue>
bool
Set<TValue>::contains(const TValue & value) const {
	return (find(value) >= 0);
}

template<typename TValue>
TValue
Set<TValue>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return TValue();
	}
	return TValue(m_data + m_index.at(pos));
}

template<>
int32_t
Set<int32_t>::at(uint32_t pos) const;

template<>
uint32_t
Set<uint32_t>::at(uint32_t pos) const;


template<>
uint16_t
Set<uint16_t>::at(uint32_t pos) const;

template<>
uint8_t
Set<uint8_t>::at(uint32_t pos) const;

template<>
std::string
Set<std::string>::at(uint32_t pos) const;

}}//end namespace

template<typename TValue>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<TValue> & source) {
	std::vector<uint8_t> tmpStore;
	sserialize::UByteArrayAdapter tmpValueStore(&tmpStore);
	std::set<uint32_t> offSets;
	for(typename std::set<TValue>::const_iterator it = source.begin(); it != source.end(); it++) {
		offSets.insert(tmpValueStore.tellPutPtr());
		tmpValueStore << *it;
	}
	
	sserialize::UByteArrayAdapter indexData(new std::vector<uint8_t>, true);
	sserialize::Static::SortedOffsetIndexPrivate::create(offSets, indexData);
	
	//Add everything to our adapter
	destination.putUint8(SSERIALIZE_STATIC_SET_VERSION); //version
	destination.putUint32(tmpStore.size());
	destination.put(indexData);
	destination.put(tmpStore);
	return destination;
}

template<typename TNumberValue>
sserialize::UByteArrayAdapter& streamNumberSetInUByteArrayAdapter(sserialize::UByteArrayAdapter & destination, const std::set<TNumberValue> & source) {
	destination.putUint8(0); //version
	uint32_t sizePutPtr = destination.tellPutPtr(); //save put ptr
	destination.putUint32(0); //put dummy size
	for(typename std::set<TNumberValue>::const_iterator it = source.begin(); it != source.end(); it++) {
		destination << *it;
	}
	//Put correct size, the 4 is the size of the SIZE-Field
	destination.putUint32(sizePutPtr, destination.tellPutPtr()-(4+sizePutPtr));
	return destination;
}

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<int32_t> & source);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint32_t> & source);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint16_t> & source);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint8_t> & source);

template<typename TValue>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::Static::Set<TValue> & destination) {
	sserialize::UByteArrayAdapter tmpAdap(source);
	tmpAdap.shrinkToGetPtr();
	destination = sserialize::Static::Set<TValue>(tmpAdap);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}

template<typename TValue>
bool operator==(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	if (setA.size() != setB.size())
		return false;
	uint32_t size = setA.size();
	for(uint32_t i = 0; i < size; i++) {
		if (setA.at(i) != setB.at(i))
			return false;
	}
	return true;
}

template<typename TValue>
bool operator<(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	uint32_t setItA = 0;
	uint32_t setItB = 0;
	while (setItA<setA.size()) {
		if (setB.size() <= setItB || setB.at(setItB) < setA.at(setItA)) {
			return false;
		}
		TValue vA(setA.at(setItA));
		TValue vB(setB.at(setItB));
		if (vB < vA) {
			return false;
		}
		else if (vA < vB)
			return true;
		setItA++;
		setItB++;
	}
	return (setItB < setB.size());
}

template<typename TValue>
bool operator!=(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return !(setA == setB);
}

template<typename TValue>
bool operator>(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return setB < setA;
}

template<typename TValue>
bool operator<=(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return ! (setA > setB);
}

template<typename TValue>
bool operator>=(const sserialize::Static::Set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return ! (setA < setB);
}

template<typename TValue>
bool operator==(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	if (setA.size() != setB.size())
		return false;
	uint32_t size = setA.size();
	typename std::set<TValue>::const_iterator setItB = setB.begin();
	for(uint32_t i = 0; i < size; i++) {
		if (setA.at(i) != *setItB)
			return false;
		setItB++;
	}
	return true;
}

template<typename TValue>
bool operator==(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return setB == setA;
}

template<typename TValue>
bool operator<(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	uint32_t setItA = 0;
	typename std::set<TValue>::const_iterator setItB = setB.begin();
	while (setItA<setA.size()) {
		if (setItB == setB.end() || *setItB < setA.at(setItA)) {
			return false;
		}
		TValue vA(setA.at(setItA));
		if (*setItB < vA) {
			return false;
		}
		else if (vA < *setItB)
			return true;
		setItA++;
		setItB++;
	}
	return (setItB != setB.end());
}

template<typename TValue>
bool operator<(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return (setB > setA);
}

template<typename TValue>
bool operator!=(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	return !(setA == setB);
}

template<typename TValue>
bool operator!=(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return !(setB == setA);
}

template<typename TValue>
bool operator>(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	return setB < setA;
}

template<typename TValue>
bool operator>(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return setB < setA;
}

template<typename TValue>
bool operator<=(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	return ! (setA > setB);
}

template<typename TValue>
bool operator<=(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return ! (setA > setB);
}

template<typename TValue>
bool operator>=(const sserialize::Static::Set<TValue> & setA, const std::set<TValue> & setB) {
	return ! (setA < setB);
}

template<typename TValue>
bool operator>=(const std::set<TValue> & setA, const sserialize::Static::Set<TValue> & setB) {
	return ! (setA < setB);
}

#endif