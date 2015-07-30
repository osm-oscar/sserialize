#ifndef SSERIALIZE_SORTED_OFFSET_INDEX_H
#define SSERIALIZE_SORTED_OFFSET_INDEX_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>

namespace sserialize {
namespace Static {

/** This class provides a a list of sorted offsets.
  * It has some limitations (due to the implementation of the static SortedOffsetIndex)
  * max(offset[i]) < max(uint36_t) for all i
  * offset[i+1]-offset[i] < max(uint32_t) for all i
  * You can store up to max(uint32_t) elements in it
  */

class SortedOffsetIndexPrivate;
  
class SortedOffsetIndex: RCWrapper<SortedOffsetIndexPrivate> {
private:
	typedef RCWrapper<SortedOffsetIndexPrivate> MyParentClass;
public:
	typedef enum {T_REGLINE} Types;
public:
	SortedOffsetIndex();
	SortedOffsetIndex(const UByteArrayAdapter & data);
	SortedOffsetIndex(const SortedOffsetIndex & other);
	virtual ~SortedOffsetIndex();
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	SortedOffsetIndex & operator=(const SortedOffsetIndex & other);
	uint32_t size() const;
	UByteArrayAdapter::OffsetType at(uint32_t pos) const;
};

template<typename T_SORTED_CONTAINER>
bool operator==(const T_SORTED_CONTAINER & other, const sserialize::Static::SortedOffsetIndex & index) {
	if (other.size() != index.size())
		return false;
	typename T_SORTED_CONTAINER::const_iterator otherIt(other.begin());
	uint32_t i = 0;
	uint32_t indexSize = index.size();
	for(; i < indexSize; ++i, ++otherIt) {
		if (index.at(i) != *otherIt)
			return false;
	}
	return true;
}

template<typename T_SORTED_CONTAINER>
bool operator==(const sserialize::Static::SortedOffsetIndex & index, const T_SORTED_CONTAINER & other) {
	return other == index;
}

template<typename T_SORTED_CONTAINER>
bool operator!=(const T_SORTED_CONTAINER & other, const sserialize::Static::SortedOffsetIndex & index) {
	return ! (other == index);
}


template<typename T_SORTED_CONTAINER>
bool operator!=(const sserialize::Static::SortedOffsetIndex & index, const T_SORTED_CONTAINER & other) {
	return ! (other == index);
}

}}//end namespace

#endif