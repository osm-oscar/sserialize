#ifndef SSERIALIZE_ITEM_INDEX_ITERATOR_INTERSECTING_H
#define SSERIALIZE_ITEM_INDEX_ITERATOR_INTERSECTING_H
#include <deque>
#include "ItemIndexIterator.h"

namespace sserialize {

class ItemIndexIteratorIntersecting: public ItemIndexIteratorPrivate {
	std::deque<ItemIndexIterator> m_idx;
	uint32_t m_val;
	uint32_t m_valid;
private:
	/** sets the next element, sets m_valid to false if necessary */
	void moveToNext();
public:
	ItemIndexIteratorIntersecting();
	ItemIndexIteratorIntersecting(const std::deque<ItemIndexIterator> & intersect); 
	virtual ~ItemIndexIteratorIntersecting();
	virtual uint32_t maxSize() const;
	virtual uint32_t operator*() const;
	virtual bool valid() const;
	virtual void next();
	virtual void reset();
	virtual ItemIndexIteratorPrivate * copy() const;
};




}//end namespace

#endif