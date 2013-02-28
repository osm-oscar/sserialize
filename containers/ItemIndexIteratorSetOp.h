#ifndef SSERIALAIZE_ITEM_INDEX_ITERATOR_H
#define SSERIALAIZE_ITEM_INDEX_ITERATOR_H
#include "ItemIndexIterator.h"


namespace sserialize {

class ItemIndexIteratorSetOp: public ItemIndexIteratorPrivate {
public:
	typedef enum {OPT_UNITE, OPT_INTERSECT, OPT_DIFF, OPT_XOR} OpTypes;
private:
	OpTypes m_type;
	ItemIndexIterator m_first;
	ItemIndexIterator m_second;
	uint32_t m_val;
	bool m_valid;
private:
	void uniteNext();
	void intersectNext();
	void diffNext();
	void xorNext();
public:
	ItemIndexIteratorSetOp();
	ItemIndexIteratorSetOp(const ItemIndexIterator & first, const ItemIndexIterator & second, ItemIndexIteratorSetOp::OpTypes type);
	virtual ~ItemIndexIteratorSetOp();
    virtual uint32_t maxSize() const;
	virtual uint32_t operator*() const;
	virtual bool valid() const;
	virtual void next();
	virtual void reset();
	virtual ItemIndexIteratorPrivate * copy() const;
};


}//end namespace

#endif