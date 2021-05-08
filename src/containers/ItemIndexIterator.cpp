#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/containers/ItemIndexIteratorIntersecting.h>
#include <sserialize/containers/ItemIndexIteratorSetOp.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>

namespace sserialize {


ItemIndexIteratorPrivate::ItemIndexIteratorPrivate() {}
ItemIndexIteratorPrivate::~ItemIndexIteratorPrivate() {}

ItemIndexIteratorPrivateItemIndex::ItemIndexIteratorPrivateItemIndex() :
ItemIndexIteratorPrivate(),
m_pos(0),
m_cur(0)
{}

ItemIndexIteratorPrivateItemIndex::ItemIndexIteratorPrivateItemIndex(const ItemIndex& idx) :
ItemIndexIteratorPrivate(),
m_index(idx),
m_pos(0),
m_cur(0)
{
	if (idx.size()) {
		m_cur = m_index.at(0);
	}
	else {
		m_pos = 1; //move one beyond
	}
}

ItemIndexIteratorPrivateItemIndex::~ItemIndexIteratorPrivateItemIndex() {}

uint32_t ItemIndexIteratorPrivateItemIndex::maxSize() const {
	return m_index.size();
}

uint32_t ItemIndexIteratorPrivateItemIndex::operator*() const {
	return m_cur;
}

void ItemIndexIteratorPrivateItemIndex::next() {
	uint32_t idxSize = m_index.size();
	if (m_pos+1 < idxSize) {
		++m_pos;
		m_cur = m_index.at(m_pos);
	}
	else {
		m_pos = idxSize;
		m_cur = 0;
	}
}

void ItemIndexIteratorPrivateItemIndex::reset() {
	m_pos = 0;
	m_cur = 0;
	if (m_index.size()) {
		m_cur = m_index.at(0);
	}
}


bool ItemIndexIteratorPrivateItemIndex::valid() const {
	return m_pos < m_index.size();
}

ItemIndexIteratorPrivate* ItemIndexIteratorPrivateItemIndex::copy() const {
	ItemIndexIteratorPrivateItemIndex * newIdx = new ItemIndexIteratorPrivateItemIndex();
	newIdx->m_index = m_index;
	newIdx->m_pos = m_pos;
	newIdx->m_cur = m_cur;
	return newIdx;
}

ItemIndexIterator::ItemIndexIterator() :
RCWrapper<ItemIndexIteratorPrivate>(new ItemIndexIteratorPrivateEmpty())
{}

ItemIndexIterator::ItemIndexIterator(const ItemIndexIterator& other) :
RCWrapper<ItemIndexIteratorPrivate>(other)
{}

ItemIndexIterator::ItemIndexIterator(ItemIndexIteratorPrivate* priv) :
RCWrapper< sserialize::ItemIndexIteratorPrivate >(priv)
{}

ItemIndexIterator::ItemIndexIterator(const ItemIndex& idx) : 
RCWrapper< sserialize::ItemIndexIteratorPrivate >(new ItemIndexIteratorPrivateItemIndex(idx))
{}

ItemIndexIterator::ItemIndexIterator(const std::vector< ItemIndexIterator >& intersectIterators) :
RCWrapper< sserialize::ItemIndexIteratorPrivate >(new ItemIndexIteratorIntersecting(intersectIterators))
{}
/*
ItemIndexIterator::ItemIndexIterator(const std::vector< ItemIndexIterator >& intersectIterators) :
RCWrapper< sserialize::ItemIndexIteratorPrivate >(new ItemIndexIteratorIntersecting(intersectIterators))
{}*/

ItemIndexIterator::~ItemIndexIterator() {}

ItemIndexIterator& ItemIndexIterator::operator=(const ItemIndexIterator& other) {
	RCWrapper<sserialize::ItemIndexIteratorPrivate>::operator=(other);
	return *this;
}

uint32_t ItemIndexIterator::maxSize() const {
	return priv()->maxSize();
}


uint32_t ItemIndexIterator::operator*() const {
	return priv()->operator*();
}

bool ItemIndexIterator::valid() const {
	return priv()->valid();
}

ItemIndexIterator& ItemIndexIterator::operator++() {
	if (priv()->rc() > 1) {
		ItemIndexIteratorPrivate * newPriv = priv()->copy();
		SSERIALIZE_CHEAP_ASSERT_EQUAL(uint32_t(0), newPriv->rc());
		setPrivate(newPriv);
	}
	priv()->next();
	return *this;
}

ItemIndexIterator& ItemIndexIterator::reset() {
	if (priv()->rc() > 1) {
		ItemIndexIteratorPrivate * newPriv = priv()->copy();
		SSERIALIZE_CHEAP_ASSERT_EQUAL(uint32_t(0), newPriv->rc());
		setPrivate(newPriv);
	}
	priv()->reset();
	return *this;
}


ItemIndex ItemIndexIterator::toItemIndex() const {
	uint32_t tempStorageSize = ItemIndexPrivateSimple::storageSize(maxSize(), 4);
	UByteArrayAdapter tempStorage( UByteArrayAdapter::createCache(tempStorageSize, sserialize::MM_PROGRAM_MEMORY) );
	if (tempStorage.size() == 0)
		return ItemIndex();
	
	CompactUintArray carr( ItemIndexPrivateSimple::initStorage(tempStorage, 4) );
	
	RCPtrWrapper<ItemIndexIteratorPrivate> tempPriv( priv()->copy() );
	
	uint32_t pos = 0;
	while (tempPriv->valid()) {
		carr.set(pos, tempPriv->operator*());
		tempPriv->next();
		pos++;
	}
	ItemIndexPrivateSimple::addHeader(pos, 4, 0, tempStorage);
	return ItemIndex(tempStorage, sserialize::ItemIndex::T_SIMPLE);
}

}//end namespace

sserialize::ItemIndexIterator operator+(const sserialize::ItemIndexIterator& first, const sserialize::ItemIndexIterator& second) {
	return sserialize::ItemIndexIterator( new  sserialize::ItemIndexIteratorSetOp(first, second, sserialize::ItemIndexIteratorSetOp::OPT_UNITE) );
}


sserialize::ItemIndexIterator operator-(const sserialize::ItemIndexIterator& first, const sserialize::ItemIndexIterator& second) {
	return sserialize::ItemIndexIterator( new sserialize::ItemIndexIteratorSetOp(first, second, sserialize::ItemIndexIteratorSetOp::OPT_DIFF) );
}


sserialize::ItemIndexIterator operator/(const sserialize::ItemIndexIterator& first, const sserialize::ItemIndexIterator& second) {
	return sserialize::ItemIndexIterator( new sserialize::ItemIndexIteratorSetOp(first, second, sserialize::ItemIndexIteratorSetOp::OPT_INTERSECT) );
}


sserialize::ItemIndexIterator operator^(const sserialize::ItemIndexIterator& first, const sserialize::ItemIndexIterator& second) {
	return sserialize::ItemIndexIterator( new sserialize::ItemIndexIteratorSetOp(first, second, sserialize::ItemIndexIteratorSetOp::OPT_XOR) );
}

namespace sserialize {


sserialize::ItemIndexIterator createMergeItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second) {
	return first + second;
}

sserialize::ItemIndexIterator createDiffItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second) {
	return first - second;
}

sserialize::ItemIndexIterator createIntersectItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second) {
	return first / second;
}

sserialize::ItemIndexIterator createSymDiffItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second) {
	return first ^ second;
}


}//end namespace
