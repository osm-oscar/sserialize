#ifndef SSERIALIZE_CELL_QUERY_RESULT_H
#define SSERIALIZE_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <memory>
#include <unordered_map>

namespace sserialize {

class CellQueryResult {
public:
	///Class to get the items in a cell
	struct CellDerefer {
		virtual sserialize::ItemIndex operator()(uint32_t cellId) const = 0;
	};
	
	typedef std::unordered_map<uint32_t, ItemIndex> PartialMatchesMap;
	
private:
	ItemIndex m_fullMatches;
	ItemIndex m_partialMatches;
	PartialMatchesMap m_partialMatchesItems;
	ItemIndex::Types m_indexType;
public:
	CellQueryResult(ItemIndex::Types idxType = ItemIndex::Types::T_NULL) : m_indexType(idxType) {}
	CellQueryResult(const ItemIndex & fullMatches, const ItemIndex & partialMatches, ItemIndex::Types idxTypes) :
	m_fullMatches(fullMatches),
	m_partialMatches(partialMatches),
	m_indexType(idxTypes)
	{}
	virtual ~CellQueryResult() {}
	CellQueryResult operator/(const CellQueryResult & other) const;
	CellQueryResult operator+(const CellQueryResult & other) const;
	
	inline const ItemIndex & fullMatches() const { return m_fullMatches; }
	inline ItemIndex & fullMatches() { return m_fullMatches; }
	
	inline const ItemIndex & partialMatches() const { return m_partialMatches; }
	inline ItemIndex & partialMatches() { return m_partialMatches; }
	
	inline const PartialMatchesMap & partialMatchesItems() const { return m_partialMatchesItems; }
	inline PartialMatchesMap & partialMatchesItems() { return m_partialMatchesItems; }

	inline const ItemIndex::Types & indexType() const { return m_indexType; }
	inline ItemIndex::Types & indexType() { return m_indexType; }
};

}


#endif