#ifndef SSERIALIZE_CELL_QUERY_RESULT_H
#define SSERIALIZE_CELL_QUERY_RESULT_H
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <memory>
#include <unordered_map>
#include <string.h>

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
	CellQueryResult(const CellQueryResult & other) :
	m_fullMatches(other.m_fullMatches),
	m_partialMatches(other.m_partialMatches),
	m_partialMatchesItems(other.m_partialMatchesItems),
	m_indexType(other.m_indexType)
	{}
	CellQueryResult(CellQueryResult && other) :
	m_fullMatches(other.m_fullMatches),
	m_partialMatches(other.m_partialMatches),
	m_indexType(other.m_indexType)
	{
		m_partialMatchesItems.swap(other.m_partialMatchesItems);
	}
	virtual ~CellQueryResult() {}
	CellQueryResult & operator=(const CellQueryResult & other);
	CellQueryResult & operator=(CellQueryResult && other);
	
	CellQueryResult operator/(const CellQueryResult & other) const;
	CellQueryResult operator+(const CellQueryResult & other) const;
	CellQueryResult operator-(const CellQueryResult & other) const;
	CellQueryResult operator^(const CellQueryResult & other) const;
	
	inline const ItemIndex & fullMatches() const { return m_fullMatches; }
	inline ItemIndex & fullMatches() { return m_fullMatches; }
	
	inline const ItemIndex & partialMatches() const { return m_partialMatches; }
	inline ItemIndex & partialMatches() { return m_partialMatches; }
	
	inline const PartialMatchesMap & partialMatchesItems() const { return m_partialMatchesItems; }
	inline PartialMatchesMap & partialMatchesItems() { return m_partialMatchesItems; }

	inline const ItemIndex::Types & indexType() const { return m_indexType; }
	inline ItemIndex::Types & indexType() { return m_indexType; }
	
	template<typename T_INDEX_DEREFER>
	ItemIndex flaten(T_INDEX_DEREFER derefer) const;
};

template<typename T_CELL_ID_TO_ITEMS_DEREFER>
ItemIndex CellQueryResult::flaten(T_CELL_ID_TO_ITEMS_DEREFER cellIdToItemsDerefer) const {
	std::vector<sserialize::ItemIndex> idcs;
	idcs.reserve(fullMatches().size() + partialMatches().size());
	for(ItemIndex::const_iterator it(fullMatches().cbegin()), end(fullMatches().cend()); it != end; ++it) {
		idcs.push_back( cellIdToItemsDerefer(*it) );
	}
	for(PartialMatchesMap::const_iterator it(partialMatchesItems().cbegin()), end(partialMatchesItems().cend()); it != end; ++it) {
		idcs.push_back( it->second );
	}
	return ItemIndex::unite(idcs);
}

}


#endif