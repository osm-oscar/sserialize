#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/iterator/AtStlInputIterator.h>
#include <sserialize/stats/statfuncs.h>

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
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::UniteOp,
		sserialize::detail::ItemIndexPrivate::ItemIndexNativeCreator,
		uint32_t
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate * ItemIndexPrivate::doIntersect(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::IntersectOp,
		sserialize::detail::ItemIndexPrivate::ItemIndexNativeCreator,
		uint32_t
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate * ItemIndexPrivate::doDifference(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::DifferenceOp,
		sserialize::detail::ItemIndexPrivate::ItemIndexNativeCreator,
		uint32_t
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate * ItemIndexPrivate::doSymmetricDifference(const ItemIndexPrivate * other) const {
	if (!other)
		return new ItemIndexPrivateEmpty();
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::SymmetricDifferenceOp,
		sserialize::detail::ItemIndexPrivate::ItemIndexNativeCreator,
		uint32_t
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}


uint32_t ItemIndexPrivate::find(uint32_t id) const {
	uint32_t len = size();
	if (len == 0) {
		return -1;
	}
	int64_t left = 0;
	int64_t right = len-1;
	int64_t mid = (right-left)/2+left;
	uint32_t curId;
	while( left < right ) {
		curId = at((uint32_t) mid);
		if (curId == id) {
			return (uint32_t)mid;
		}
		if (curId < id) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	curId = at((uint32_t)mid);
	return ((curId == id) ? (uint32_t) mid : npos);
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
	return true; //TODO: set default to false
}

void ItemIndexPrivate::putInto(DynamicBitSet & bitSet) const {
	doPutInto(bitSet);
}

void ItemIndexPrivate::putInto(uint32_t * dest) const {
	doPutInto(dest);
}

ItemIndexPrivate* ItemIndexPrivate::uniteK(const sserialize::ItemIndexPrivate* other, uint32_t /*numItems*/) const {
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

uint32_t ItemIndexPrivateEmpty::find(uint32_t /*id*/) const {
    return npos;
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

UByteArrayAdapter::SizeType ItemIndexPrivateEmpty::getSizeInBytes() const {
	return 0;
}

namespace detail {
namespace ItemIndexImpl {

GenericSetOpExecuterAccessors<uint32_t>::PositionIterator
GenericSetOpExecuterAccessors<uint32_t>::begin(const sserialize::ItemIndexPrivate *) {
	return 0;
}

GenericSetOpExecuterAccessors<uint32_t>::PositionIterator
GenericSetOpExecuterAccessors<uint32_t>::end(const sserialize::ItemIndexPrivate * idx) {
	return idx->size();
}

void
GenericSetOpExecuterAccessors<uint32_t>::next(PositionIterator & it) {
	++it;
}

bool
GenericSetOpExecuterAccessors<uint32_t>::unequal(const PositionIterator & first, const PositionIterator & second) {
	return first != second;
}

uint32_t
GenericSetOpExecuterAccessors<uint32_t>::get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it) {
	return idx->uncheckedAt(it);
}

GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::PositionIterator
GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::begin(const sserialize::ItemIndexPrivate * idx) {
	return sserialize::ItemIndex::const_iterator(idx->cbegin());
}

GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::PositionIterator
GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::end(const sserialize::ItemIndexPrivate * idx) {
	return sserialize::ItemIndex::const_iterator(idx->cend());
}

void
GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::next(PositionIterator & it) {
	++it;
}

bool
GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::unequal(const PositionIterator & first, const PositionIterator & second) {
	return first != second;
}

uint32_t
GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator>::get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it) {
	return *it;
}


}}//end detail::ItemIndexImpl

}//end namespace