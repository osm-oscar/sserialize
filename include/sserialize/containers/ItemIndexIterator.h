#ifndef SSERAILIZE_ITEM_INDEX_ITERATOR_H
#define SSERAILIZE_ITEM_INDEX_ITERATOR_H
#include <sserialize/utility/refcounting.h>
#include "ItemIndex.h"

namespace sserialize {

class ItemIndexIteratorPrivate: public RefCountObject {
public:
	ItemIndexIteratorPrivate();
	virtual ~ItemIndexIteratorPrivate();
	virtual uint32_t maxSize() const = 0;
	virtual uint32_t operator*() const = 0;
	virtual bool valid() const = 0;
	virtual void next() = 0;
	virtual void reset() = 0;
	
	virtual ItemIndexIteratorPrivate * copy() const = 0;
};

class ItemIndexIteratorPrivateEmpty: public ItemIndexIteratorPrivate {
public:
    ItemIndexIteratorPrivateEmpty() {}
	virtual ~ItemIndexIteratorPrivateEmpty() {}
    virtual uint32_t maxSize() const { return 0; }
	virtual uint32_t operator*() const { return 0;}
	virtual bool valid() const { return false; }
	virtual void next() {}
	virtual void reset() {}
	
	virtual ItemIndexIteratorPrivate * copy() const { return new ItemIndexIteratorPrivateEmpty();}
};

class ItemIndexIteratorPrivateFixedRange: public ItemIndexIteratorPrivate {
	uint32_t m_begin;
	uint32_t m_end;
	uint32_t m_pos;
	uint32_t m_cur;
public:
	//end is one beyond the last
    ItemIndexIteratorPrivateFixedRange(uint32_t begin, uint32_t end) : m_begin(begin), m_end(end), m_pos(m_begin) {
		next();
    }
	virtual ~ItemIndexIteratorPrivateFixedRange() {}
    virtual uint32_t maxSize() const { return m_end-m_begin; }
	virtual uint32_t operator*() const { return m_cur;}
	virtual bool valid() const { return m_pos <= m_end; }
	virtual void next() { m_cur = m_pos; m_pos++; }
	virtual void reset() { m_pos = m_begin; next();}
	
	virtual ItemIndexIteratorPrivate * copy() const {
		ItemIndexIteratorPrivateFixedRange * priv = new ItemIndexIteratorPrivateFixedRange(m_begin, m_end);
		priv->m_pos = m_pos;
		priv->m_cur = m_cur;
		return priv;
	}
};


class ItemIndexIteratorPrivateItemIndex: public ItemIndexIteratorPrivate {
	ItemIndex m_index;
	uint32_t m_pos;
	uint32_t m_cur;
public:
	ItemIndexIteratorPrivateItemIndex();
	ItemIndexIteratorPrivateItemIndex(const ItemIndex & idx);
	virtual ~ItemIndexIteratorPrivateItemIndex();
    virtual uint32_t maxSize() const;
	virtual uint32_t operator*() const;
	virtual bool valid() const;
	virtual void next();
	virtual void reset();
	
	virtual ItemIndexIteratorPrivate * copy() const;
};

class ItemIndexIterator;

typedef ItemIndexIterator (*ItemIndexIteratorOperatorFP)(const sserialize::ItemIndexIterator &, const sserialize::ItemIndexIterator &);

class ItemIndexIterator: RCWrapper<ItemIndexIteratorPrivate> {
public:
	ItemIndexIterator();
	ItemIndexIterator(const ItemIndexIterator & other);
	ItemIndexIterator(ItemIndexIteratorPrivate * priv);
    ItemIndexIterator(const ItemIndex & idx);
	ItemIndexIterator(const std::vector< sserialize::ItemIndexIterator >& intersectIterators);
// 	ItemIndexIterator(const std::vector<ItemIndexIterator> & intersectIterators);
	virtual ~ItemIndexIterator();
	uint32_t maxSize() const;
	ItemIndexIterator& operator=(const ItemIndexIterator & other);
	uint32_t operator*() const;
	bool valid() const;
	ItemIndexIterator& operator++();
	ItemIndexIterator& reset();
	ItemIndex toItemIndex() const;

	template<class RandomAccessIterator>
	static ItemIndexIterator createTree(const RandomAccessIterator & begin, const RandomAccessIterator & end, ItemIndexIteratorOperatorFP fp) {
		if (end-begin == 2) {
			return (*fp)(*begin, *end);
		}
		else if (end-begin == 1) {
			return ItemIndexIterator(*begin);
		}
		else {
			return (*fp)( createTree(begin, begin + ((end-begin)/2), fp), createTree(begin + ((end-begin)/2) + 1, end, fp) );
		}
	}

};


sserialize::ItemIndexIterator createMergeItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator createDiffItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator createIntersectItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator createSymDiffItemIndexIterator(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);

}//end namespace

sserialize::ItemIndexIterator operator+(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator operator-(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator operator/(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);
sserialize::ItemIndexIterator operator^(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second);

#endif