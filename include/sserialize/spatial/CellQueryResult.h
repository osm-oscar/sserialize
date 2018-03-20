#ifndef SSERIALIZE_CELL_QUERY_RESULT_H
#define SSERIALIZE_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/containers/RLEStream.h>

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

class CellQueryResultIterator: public std::iterator<std::input_iterator_tag, sserialize::ItemIndex, int64_t> {
public:
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
	inline const sserialize::ItemIndex & idx() const { return this->operator*(); }
	bool fetched() const;
	///raw data in the format (cellId|fetched|fullMatch) least-significant bit to the right
	uint32_t rawDesc() const;
	inline uint32_t pos() const { return m_pos; }
	const sserialize::ItemIndex & operator*() const;
	inline bool operator!=(const CellQueryResultIterator & other) const { return m_pos != other.m_pos; }
	inline bool operator==(const CellQueryResultIterator & other) const { return m_pos == other.m_pos; }
	inline difference_type operator-(const CellQueryResultIterator & other) const { return m_pos - other.m_pos; }
	CellQueryResultIterator operator++(int);
	CellQueryResultIterator & operator++();
	CellQueryResultIterator operator+(difference_type v) const;
};

}

class TreedCellQueryResult;

class CellQueryResult {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	typedef detail::CellQueryResultIterator const_iterator;
	enum {
		FF_NONE=0x0,
		FF_EMPTY=0x1,
		FF_CELL_LOCAL_ITEM_IDS=0x2,
		FF_CELL_GLOBAL_ITEM_IDS=0x4,
		FF_MASK_CELL_ITEM_IDS=FF_CELL_LOCAL_ITEM_IDS|FF_CELL_GLOBAL_ITEM_IDS,
		FF_DEFAULTS=FF_CELL_GLOBAL_ITEM_IDS
	} FeatureFlags;
public:
	CellQueryResult();
	CellQueryResult(const GeoHierarchy & gh, const ItemIndexStore & idxStore, int flags = FF_DEFAULTS);
	CellQueryResult(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore, int flags);
	///cellIdxId is ignored if fullMatch is set
	CellQueryResult(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId, int flags);
	CellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::CompactUintArray::const_iterator & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore,
					int flags);
	CellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					const sserialize::RLEStream & partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore,
					int flags);
	CellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					std::vector<uint32_t>::const_iterator partialMatchesItemsPtrBegin,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore,
					int flags);
	CellQueryResult(const ItemIndex & fullMatches,
					const ItemIndex & partialMatches,
					std::vector<sserialize::ItemIndex>::const_iterator partialMatchesIdx,
					const GeoHierarchy & gh, const ItemIndexStore & idxStore,
					int flags);
	virtual ~CellQueryResult();
	CellQueryResult(const CellQueryResult & other);
	CellQueryResult & operator=(const CellQueryResult & other);
	
	const GeoHierarchy & geoHierarchy() const;
	const ItemIndexStore & idxStore() const;
	int flags() const;
	
	uint32_t cellCount() const;
	bool hasHits() const;
	///complexity: O(cellCount())
	uint32_t maxItems() const;
	int defaultIndexTypes() const;
	uint32_t idxSize(uint32_t pos) const;
	uint32_t cellId(uint32_t pos) const;
    //not that this may return cell local ids, items() instead to retrieve global item ids
	sserialize::ItemIndex idx(uint32_t pos) const;
    sserialize::ItemIndex items(uint32_t pos) const;
	///This is only correct for ( ((features&FF_CELL_LOCAL_ITEM_IDS)== 0 && fullMatch) || !fetched())
	uint32_t idxId(uint32_t pos) const;
	bool fetched(uint32_t pos) const;
	bool fullMatch(uint32_t pos) const;
	
	CellQueryResult operator/(const CellQueryResult & other) const;
	CellQueryResult operator+(const CellQueryResult & other) const;
	CellQueryResult operator-(const CellQueryResult & other) const;
	CellQueryResult operator^(const CellQueryResult & other) const;
	CellQueryResult allToFull() const;
	CellQueryResult removeEmpty() const;
	
	bool operator!=(const CellQueryResult & other) const;
	bool operator==(const CellQueryResult & other) const;
	
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator end() const;
	const_iterator cend() const;
	ItemIndex flaten(uint32_t threadCount = 1) const;
	ItemIndex topK(uint32_t numItems) const;
	CellQueryResult toGlobalItemIds(uint32_t threadCount = 1) const;
	CellQueryResult toCellLocalItemIds(uint32_t threadCount = 1) const;
	void dump(std::ostream & out) const;
	void dump() const;
	sserialize::ItemIndex cells() const;
private:
	friend class sserialize::TreedCellQueryResult;
private:
	CellQueryResult(detail::CellQueryResult * priv);
	bool selfCheck();
private:
	RCPtrWrapper<detail::CellQueryResult> m_priv;
};

std::ostream & operator<<(std::ostream & out, const CellQueryResult & src);

}


#endif
