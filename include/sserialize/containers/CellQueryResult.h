#ifndef SSERIALIZE_CELL_QUERY_RESULT_H
#define SSERIALIZE_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/utility/CompactUintArray.h>

namespace sserialize {

namespace detail {
	class CellQueryResult;
}
namespace Static {
namespace spatial {
	class GeoHierarchy;
}
	class ItemIndexStore;
}

namespace detail {

class CellQueryResultIterator {
public:
	typedef int64_t differnce_type;
	typedef sserialize::ItemIndex value_type;
	typedef const sserialize::ItemIndex & reference;
	typedef enum {RD_FULL_MATCH=0x1, RD_FETCHED=0x2} RawDescAccessors;
private:
	RCPtrWrapper<detail::CellQueryResult> m_d;
	uint32_t m_pos;
public:
	CellQueryResultIterator(const RCPtrWrapper<detail::CellQueryResult> & cqr, uint32_t pos);
	CellQueryResultIterator(const CellQueryResultIterator & other);
	virtual ~CellQueryResultIterator();
	CellQueryResultIterator & operator=(const CellQueryResultIterator & other);
	uint32_t cellId() const;
	bool fullMatch() const;
	uint32_t idxSize() const;
	//This is only correct for (fullMatch() || !fetched())
	uint32_t idxId() const;
	bool fetched() const;
	///raw data in the format (cellId|fetched|fullMatch) least-significant bit to the right
	uint32_t rawDesc() const;
	inline uint32_t pos() const { return m_pos; }
	const sserialize::ItemIndex & operator*() const;
	inline bool operator!=(const CellQueryResultIterator & other) const { return m_pos != other.m_pos; }
	inline differnce_type operator-(const CellQueryResultIterator & other) const { return m_pos - other.m_pos; }
	CellQueryResultIterator operator++(int );
	CellQueryResultIterator & operator++();
	CellQueryResultIterator operator+(differnce_type v) const;
};

}

class CellQueryResult {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	typedef detail::CellQueryResultIterator const_iterator;
private:
	RCPtrWrapper<detail::CellQueryResult> m_priv;
private:
	CellQueryResult(detail::CellQueryResult * priv);
public:
	CellQueryResult();
	CellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::CompactUintArray::const_iterator & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~CellQueryResult();
	CellQueryResult(const CellQueryResult & other);
	CellQueryResult & operator=(const CellQueryResult & other);
	uint32_t cellCount() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
	
	sserialize::ItemIndex idx(uint32_t pos) const;
	///This is only correct for (fullMatch || !fetched())
	uint32_t idxId(uint32_t pos) const;
	bool fetched(uint32_t pos) const;
	bool fullMatch(uint32_t pos) const;
	
	CellQueryResult operator/(const CellQueryResult & other) const;
	CellQueryResult operator+(const CellQueryResult & other) const;
	CellQueryResult operator-(const CellQueryResult & other) const;
	CellQueryResult operator^(const CellQueryResult & other) const;
	
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator end() const;
	const_iterator cend() const;
	ItemIndex flaten() const;
};

}


#endif