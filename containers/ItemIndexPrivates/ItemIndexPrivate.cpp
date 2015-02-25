#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/AtStlInputIterator.h>
#include <sserialize/utility/statfuncs.h>
#include "ItemIndexSetFunctions.h"

namespace sserialize {

ItemIndexPrivate::ItemIndexPrivate() {}
ItemIndexPrivate::~ItemIndexPrivate() {}

void ItemIndexPrivate::loadIntoMemory() {}

UByteArrayAdapter ItemIndexPrivate::data() const {
	throw sserialize::UnimplementedFunctionException("sserialize::ItemIndexPrivate::data");
	return UByteArrayAdapter();
}

ItemIndexPrivate::const_iterator ItemIndexPrivate::cbegin() const {
	typedef ReadOnlyAtStlIterator<const ItemIndexPrivate*, uint32_t, uint32_t> MyIt;
	return new detail::AbstractArrayIteratorDefaultImp<MyIt , uint32_t >(MyIt(0, this));
}

ItemIndexPrivate::const_iterator ItemIndexPrivate::cend() const {
	typedef ReadOnlyAtStlIterator<const ItemIndexPrivate*, uint32_t, uint32_t> MyIt;
	return new detail::AbstractArrayIteratorDefaultImp<MyIt , uint32_t >(MyIt(size(), this));
}

uint32_t ItemIndexPrivate::uncheckedAt(uint32_t pos) const {
	return at(pos);
}

ItemIndexPrivate * ItemIndexPrivate::doUnite(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	uint32_t lowestId = std::min<uint32_t>(first(), other->first());
	uint32_t highestId = std::max<uint32_t>(last(), other->last());
	uint32_t summedItemIdCount = size() + other->size();
	if (highestId - lowestId + 1 < summedItemIdCount) {
		summedItemIdCount = highestId - lowestId + 1;
	}
	UByteArrayAdapter indexFileAdapter( ItemIndexPrivateSimpleCreator::createCache(lowestId, highestId, summedItemIdCount, false) );
	ItemIndexPrivateSimpleCreator creator(lowestId, highestId, summedItemIdCount, indexFileAdapter);

	uniteWithMerge2(this, other, creator);

	if (creator.size() > 0) {
		creator.flush();
		return creator.privateIndex();
	}
	else {
		return new ItemIndexPrivateEmpty();
	}
}

ItemIndexPrivate * ItemIndexPrivate::doIntersect(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	uint32_t lowestId = std::min<uint32_t>(first(), other->first());
	uint32_t highestId = std::max<uint32_t>(last(), other->last());
	uint32_t smallestIndexSize;
	uint32_t largestIndexSize;
	if (size() < other->size()) {
		smallestIndexSize = size();
		largestIndexSize = other->size();
	}
	else {
		smallestIndexSize = other->size();
		largestIndexSize = size();
	}

	UByteArrayAdapter indexFileAdapter( ItemIndexPrivateSimpleCreator::createCache(lowestId, highestId, smallestIndexSize, false) );
	ItemIndexPrivateSimpleCreator creator(lowestId, highestId, smallestIndexSize, indexFileAdapter);

	if (this->is_random_access() && other->is_random_access() && (double)smallestIndexSize < (double)largestIndexSize/logTo2((double)largestIndexSize)) {
		intersectWithBinarySearch(this, other, creator);
	}
	else {
		intersectWithMerge2(this, other, creator);
	}

	if (creator.size()) {
		creator.flush();
		return creator.privateIndex();
	}
	else {
		return new ItemIndexPrivateEmpty();
	}
}

ItemIndexPrivate * ItemIndexPrivate::doDifference(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	uint32_t highestId = last();
	uint32_t lowestId = first();
	uint32_t maxStorageSize = std::min<uint32_t>(highestId-lowestId+1, size());
	UByteArrayAdapter indexFileAdapter( ItemIndexPrivateSimpleCreator::createCache(lowestId, highestId, maxStorageSize, false) );
	ItemIndexPrivateSimpleCreator creator(lowestId, highestId, maxStorageSize, indexFileAdapter);

	if (size() < other->size() && (double)size() < (double)other->size()/logTo2((double)other->size())) {
		differenceWithBinarySearch(this, other, creator);
	}
	else { //do the merge
		differenceWithMerge(this, other, creator);
	}

	if (creator.size()) {
		creator.flush();
		return creator.privateIndex();
	}
	else {
		return new ItemIndexPrivateEmpty();
	}
}

ItemIndexPrivate * ItemIndexPrivate::doSymmetricDifference(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	uint32_t lowestId = std::min<uint32_t>(first(), other->first());
	uint32_t highestId = std::max<uint32_t>(last(), other->last());
	uint32_t maxStorageSize = std::min<uint32_t>(highestId-lowestId+1, size()+other->size());
	UByteArrayAdapter indexFileAdapter( ItemIndexPrivateSimpleCreator::createCache(lowestId, highestId, maxStorageSize, false) );
	ItemIndexPrivateSimpleCreator creator(lowestId, highestId, maxStorageSize, indexFileAdapter);

	symmetricDifferenceWithMerge(this, other, creator);

	if (creator.size()) {
		creator.flush();
		return creator.privateIndex();
	}
	else {
		return new ItemIndexPrivateEmpty();
	}
}


int ItemIndexPrivate::find(uint32_t id) const {
	uint32_t len = size();
	if (len == 0)
		return -1;
	int left = 0;
	int right = len-1;
	int mid = (right-left)/2+left;
	uint32_t curId;
	while( left < right ) {
		curId = at(mid);
		if (curId == id) return mid;
		if (curId < id) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	curId = at(mid);
	return ((curId == id) ? mid : -1);
}

void ItemIndexPrivate::doPutInto(DynamicBitSet & bitSet) const {
	uint32_t mySize = size();
	for(uint32_t i = 0; i < mySize; ++i)
		bitSet.set(at(i));
}

void ItemIndexPrivate::doPutInto(uint32_t * dest) const {
	for(uint32_t i = 0, s = size(); i < s; ++i, ++dest) {
		*dest = at(i);
	}
}

bool ItemIndexPrivate::is_random_access() const {
	return true; ///BUG: set default to false
}

void ItemIndexPrivate::putInto(DynamicBitSet & bitSet) const {
	doPutInto(bitSet);
}

void ItemIndexPrivate::putInto(uint32_t * dest) const {
	doPutInto(dest);
}

ItemIndexPrivate* ItemIndexPrivate::uniteK(const sserialize::ItemIndexPrivate* other, uint32_t numItems) const {
	return unite(other);
}


ItemIndexPrivate * ItemIndexPrivate::unite(const ItemIndexPrivate * other) const {
	return doUnite(other);
}

ItemIndexPrivate * ItemIndexPrivate::intersect(const ItemIndexPrivate * other) const {
	return doIntersect(other);
}

ItemIndexPrivate * ItemIndexPrivate::difference(const ItemIndexPrivate * other) const {
	return doDifference(other);
}

ItemIndexPrivate * ItemIndexPrivate::symmetricDifference(const ItemIndexPrivate * other) const {
	return doSymmetricDifference(other);
}


ItemIndexPrivateEmpty::ItemIndexPrivateEmpty() {}
ItemIndexPrivateEmpty::~ItemIndexPrivateEmpty() {}

ItemIndex::Types ItemIndexPrivateEmpty::type() const {
	return ItemIndex::T_EMPTY;
}

int ItemIndexPrivateEmpty::find(uint32_t /*id*/) const {
    return -1;
}



uint32_t ItemIndexPrivateEmpty::first() const {
	return 0;
}

uint32_t ItemIndexPrivateEmpty::last() const {
	return 0;
}

uint32_t ItemIndexPrivateEmpty::at(uint32_t /*pos*/) const {
	return 0;
}

uint32_t ItemIndexPrivateEmpty::size() const {
	return 0;
}


uint8_t ItemIndexPrivateEmpty::bpn() const {
	return 0;
}

uint32_t ItemIndexPrivateEmpty::getSizeInBytes() const {
	return 0;
}

double ItemIndexPrivateEmpty::entropy() const {
	return 0;
}

}//end namespace