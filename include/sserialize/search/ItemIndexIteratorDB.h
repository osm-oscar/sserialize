#ifndef SSERILIZE_ITEM_INDEX_ITERATPR_DB_H
#define SSERILIZE_ITEM_INDEX_ITERATPR_DB_H
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {

template<class DataBaseType, class StringIdStoreType>
class ItemIndexIteratorDB: public ItemIndexIteratorPrivate {
private:
	DataBaseType m_db;
	StringIdStoreType m_strIds;
	UByteArrayAdapter m_cache;
	uint32_t m_start;
	uint32_t m_end;
	uint32_t m_pos;
	bool m_valid;
	uint32_t m_val;
private:
	void moveToNext();
public:
	ItemIndexIteratorDB();
	ItemIndexIteratorDB(const DataBaseType & db, const StringIdStoreType & stringIds);
	/** @param end: one beyond the last */
	ItemIndexIteratorDB(const DataBaseType & db, const StringIdStoreType & stringIds, uint32_t start, uint32_t end);
	virtual ~ItemIndexIteratorDB() {}
	virtual uint32_t maxSize() const { return m_end-m_start; }
	virtual uint32_t operator*() const { return m_val; }
	virtual bool valid() const { return m_valid; }
	virtual void next();
	virtual void reset();
	
	virtual ItemIndexIteratorPrivate * copy() const;
};

template<class DataBaseType, class StringIdStoreType>
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::ItemIndexIteratorDB() : 
m_start(0),
m_end(0),
m_pos(0),
m_valid(false),
m_val(0)
{}

template<class DataBaseType, class StringIdStoreType>
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::ItemIndexIteratorDB(const DataBaseType & db, const StringIdStoreType & stringIds) : 
m_db(db),
m_strIds(stringIds),
m_cache( UByteArrayAdapter::createCache(4*db.size(), sserialize::MM_PROGRAM_MEMORY) ),
m_start(0),
m_end(db.size()),
m_pos(0),
m_valid(true),
m_val(0)
{
	next();
}

template<class DataBaseType, class StringIdStoreType>
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::ItemIndexIteratorDB(const DataBaseType & db, const StringIdStoreType & stringIds, uint32_t start, uint32_t end) : 
m_db(db),
m_strIds(stringIds),
m_cache( UByteArrayAdapter::createCache(4*db.size(), sserialize::MM_PROGRAM_MEMORY) ),
m_start(start),
m_end(end),
m_pos(start),
m_valid(true),
m_val(0)
{
	next();
}

template<class DataBaseType, class StringIdStoreType>
void
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::next() {
	while (m_pos <  m_end ) {
		if (m_db.match(m_pos, m_strIds)) {
			m_val = m_pos;
			m_pos++;
			m_valid = true;
			return;
		}
		else
			m_pos++;
	}
	m_val = 0;
	m_valid = 0;
}

template<class DataBaseType, class StringIdStoreType>
void
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::reset() {
	m_pos = m_start;
	next();
}

template<class DataBaseType, class StringIdStoreType>
ItemIndexIteratorPrivate *
ItemIndexIteratorDB<DataBaseType, StringIdStoreType>::copy() const {
	ItemIndexIteratorDB<DataBaseType, StringIdStoreType> * newPriv = new ItemIndexIteratorDB<DataBaseType, StringIdStoreType>();
	newPriv->m_db = m_db;
	newPriv->m_strIds = m_strIds;
	newPriv->m_cache = m_cache;//TODO:There shouldn't be a case where this is dangerous, worst case is double decoding and writing
	newPriv->m_start = m_start;
	newPriv->m_end = m_end;
	newPriv->m_pos = m_pos;
	newPriv->m_valid = m_valid;
	newPriv->m_val = m_val;
	return newPriv;
}

}//end namespace

#endif