#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Array.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/containers/CellQueryResult.h>
#include <unordered_map>

#define SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION 7

namespace sserialize {
namespace Static {
namespace spatial {

/**
  *Version 1: initial draft
  *Version 2: use offset array for child/parent ptrs, use MVBitArray for node description
  *Version 3: use offset array for cell parents, use MVBitArray for cell description
  *Version 4: add boundary to regions
  *Version 5: add items pointer to regions
  *Version 6: add items count to regions
  *Version 7: add items count to cells
  *-------------------------------------------------------------------------------
  *VERSION|RegionDesc|RegionPtrs |RegionBoundaries       |CellDesc  |CellPtrs
  *-------------------------------------------------------------------------------
  *  1Byte|MVBitArray|BCUintArray|(region.size+1)*GeoRect|MVBitArray|BCUintArray
  *
  * There has to be one Region more than used. The last one defines the end for the RegionPtrs
  *
  *
  *Region
  *-------------------------------------------------------------------
  *CellListPtr|type|id|ChildrenBegin|ParentsOffset|ItemsPtr|ItemsCount
  *-------------------------------------------------------------------
  *
  *ParentBegin = offset from childrenBegin where the parent ptrs start
  *So the number of Children = ParentBegin
  *An the number of parents = Region[i+1].childrenBegin - (Region[i].childrenBegin + Region[i].ParentBegin)
  *
  *
  *Cell
  *--------------------------------
  *ItemPtrs|ItemCount|ParentsBegin
  *--------------------------------
  *
  *
  * The last region is the root of the dag. it has no representation in a db and is not directly accessible
  * If a Region has no parent and is not the root region, then it is a child of the root region
  * In essence, the root region is just the entrypoint to the graph
  *
  *
  */

namespace detail {
  
class SubSet {
public:
	class Node: public RefCountObject {
	public:
		typedef std::vector<Node*> ChildrenStorageContainer;
		typedef RCPtrWrapper<Node> NodePtr;
	private:
		std::vector< NodePtr > m_children;
		uint32_t m_id;
		uint32_t m_itemSize;
		std::vector<uint32_t> m_cells;
	public:
		Node(uint32_t id, uint32_t itemSize = 0) : m_id(id), m_itemSize(itemSize) {}
		virtual ~Node() {}
		inline uint32_t id() const { return m_id; }
		inline uint32_t size() const { return m_children.size(); }
		inline NodePtr & operator[](uint32_t pos) { return m_children[pos]; }
		inline const NodePtr & at(uint32_t pos) { return m_children.at(pos);}
		inline const NodePtr & operator[](uint32_t pos) const { return m_children[pos]; }
		inline const NodePtr & at(uint32_t pos) const { return m_children.at(pos);}
		inline void push_back(Node * child) { m_children.push_back( RCPtrWrapper<Node>(child) );}
		inline uint32_t maxItemsSize() const { return m_itemSize; }
		inline uint32_t & maxItemsSize() { return m_itemSize; }
		inline const std::vector<uint32_t> & cells() const { return m_cells;}
		inline std::vector<uint32_t> & cells() { return m_cells;}
	};
	typedef Node::NodePtr NodePtr;
private:
	RCPtrWrapper<Node> m_root;
	CellQueryResult m_cqr;
public:
	SubSet() {}
	SubSet(Node * root, const CellQueryResult & cqr) : m_root(root), m_cqr(cqr) {}
	virtual ~SubSet()  {}
	inline const NodePtr & root() const { return m_root;}
	inline const CellQueryResult & cqr() const { return m_cqr; }
};
  
class GeoHierarchy: public RefCountObject {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef sserialize::MultiVarBitArray RegionDescriptionType;
	typedef sserialize::BoundedCompactUintArray RegionPtrListType;
	
	typedef sserialize::MultiVarBitArray CellDescriptionType;
	typedef sserialize::BoundedCompactUintArray CellPtrListType;
private:
	SubSet::Node * createSubSet(const CellQueryResult & cqr, SubSet::Node** nodes, uint32_t size) const;
	SubSet::Node * createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const;
private:
	RegionDescriptionType m_regions;
	RegionPtrListType m_regionPtrs;
	sserialize::Static::Array<sserialize::spatial::GeoRect> m_regionBoundaries;
	CellDescriptionType m_cells;
	CellPtrListType m_cellPtrs;
public:
	GeoHierarchy();
	GeoHierarchy(const UByteArrayAdapter & data);
	virtual ~GeoHierarchy();
	OffsetType getSizeInBytes() const;

	const RegionDescriptionType & regions() const { return m_regions; }
	const CellDescriptionType & cells() const { return m_cells; }
	const RegionPtrListType & regionPtrs() const { return m_regionPtrs; }
	const CellPtrListType & cellPtrs() const { return m_cellPtrs; }

	uint32_t cellSize() const;
	
	uint32_t cellParentsBegin(uint32_t id) const;
	uint32_t cellParentsEnd(uint32_t id) const;
	uint32_t cellPtrSize() const;
	uint32_t cellPtr(uint32_t pos) const;
	uint32_t cellItemsPtr(uint32_t pos) const;
	uint32_t cellItemsCount(uint32_t pos) const;
	
	uint32_t regionSize() const;
	
	uint32_t regionItemsPtr(uint32_t pos) const;
	uint32_t regionItemsCount(uint32_t pos) const;
	uint32_t regionCellSumItemsCount(uint32_t pos) const;
	
	uint32_t regionParentsBegin(uint32_t id) const;
	uint32_t regionParentsEnd(uint32_t id) const;
	uint32_t regionPtrSize() const;
	uint32_t regionPtr(uint32_t pos) const;
	
	sserialize::spatial::GeoRect boundary(uint32_t id) const;
	
	std::ostream & printStats(std::ostream & out) const;
	
	SubSet subSet(const sserialize::CellQueryResult & cqr) const;
};

class Cell {
public:
	typedef enum {CD_ITEM_PTR=0, CD_ITEM_COUNT=1, CD_PARENTS_BEGIN=2, CD__ENTRY_SIZE=3} CellDescriptionAccessors;
private:
	uint32_t m_pos;
	RCPtrWrapper<GeoHierarchy> m_db;
public:
	Cell();
	Cell(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db);
	virtual ~Cell();
	uint32_t itemPtr() const;
	uint32_t itemCount() const;
	///Offset into PtrArray
	uint32_t parentsBegin() const;
	///Offset into PtrArray
	uint32_t parentsEnd() const;
	uint32_t parentsSize() const;
	///@return pos of the parent
	uint32_t parent(uint32_t pos) const;
};

class Region {
public:
	typedef enum {RD_CELL_LIST_PTR=0, RD_TYPE=1, RD_ID=2, RD_CHILDREN_BEGIN=3, RD_PARENTS_OFFSET=4, RD_ITEMS_PTR=5, RD_ITEMS_COUNT=6,
					RD__ENTRY_SIZE=7} RegionDescriptionAccessors;
private:
	uint32_t m_pos;
	RCPtrWrapper<GeoHierarchy> m_db;
public:
	Region();
	///Only to be used data with getPtr = 0
	Region(uint32_t pos, const RCPtrWrapper<GeoHierarchy> & db);
	virtual ~Region();
	sserialize::spatial::GeoShapeType type() const;
	uint32_t id() const;
	sserialize::spatial::GeoRect boundary() const;
	uint32_t cellIndexPtr() const;
	///Offset into PtrArray
	uint32_t parentsBegin() const;
	///Offset into PtrArray
	uint32_t parentsEnd() const;
	uint32_t parentsSize() const;
	///@return pos of the parent
	uint32_t parent(uint32_t pos) const;

	uint32_t childrenSize() const;
	///Offset into PtrArray
	uint32_t childrenBegin() const;
	///Offset into PtrArray
	uint32_t childrenEnd() const;
	uint32_t child(uint32_t pos) const;
	
	///return the items in this region
	uint32_t itemsPtr() const;
	uint32_t itemsCount() const;
};

}//end namespace detail

class GeoHierarchy {
private:
	typedef RCPtrWrapper<detail::GeoHierarchy> DataPtr;
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef detail::GeoHierarchy::RegionDescriptionType RegionDescriptionType;
	typedef detail::GeoHierarchy::RegionPtrListType RegionPtrListType;
	
	typedef detail::GeoHierarchy::CellDescriptionType CellDescriptionType;
	typedef detail::GeoHierarchy::CellPtrListType CellPtrListType;

	typedef detail::Region Region;
	typedef detail::Cell Cell;
	typedef detail::SubSet SubSet;

private:
	RCPtrWrapper<detail::GeoHierarchy> m_priv;
public:
	GeoHierarchy() : m_priv(new detail::GeoHierarchy()) {}
	GeoHierarchy(const UByteArrayAdapter & data) : m_priv(new detail::GeoHierarchy(data)) {}
	GeoHierarchy(const GeoHierarchy & other) : m_priv(other.m_priv) {}
	virtual ~GeoHierarchy() {}
	inline OffsetType getSizeInBytes() const { return m_priv->getSizeInBytes(); }
	
	inline const RegionDescriptionType & regions() const { return m_priv->regions(); }
	inline const CellDescriptionType & cells() const { return m_priv->cells(); }
	inline const CellPtrListType & cellPtrs() const { return m_priv->cellPtrs(); }
	inline const RegionPtrListType & regionPtrs() const { return m_priv->regionPtrs(); }
	
	inline uint32_t cellSize() const { return m_priv->cellSize(); }
	Cell cell(uint32_t id) const;
	
	inline uint32_t cellParentsBegin(uint32_t id) const { return m_priv->cellParentsBegin(id);}
	inline uint32_t cellParentsEnd(uint32_t id) const { return m_priv->cellParentsEnd(id);}
	inline uint32_t cellPtrSize() const { return m_priv->cellPtrSize();}
	inline uint32_t cellPtr(uint32_t pos) const { return m_priv->cellPtr(pos);}
	inline uint32_t cellItemsPtr(uint32_t pos) const { return m_priv->cellItemsPtr(pos);}
	inline uint32_t cellItemsCount(uint32_t pos) const { return m_priv->cellItemsCount(pos);}
	
	inline uint32_t regionSize() const { return m_priv->regionSize();}
	Region region(uint32_t id) const;
	Region rootRegion() const;
	
	inline uint32_t regionItemsPtr(uint32_t pos) const { return m_priv->regionItemsPtr(pos);}
	inline uint32_t regionItemsCount(uint32_t pos) const { return m_priv->regionItemsCount(pos);}
	inline uint32_t regionCellSumItemsCount(uint32_t pos) const { return m_priv->regionCellSumItemsCount(pos);}
	
	inline uint32_t regionParentsBegin(uint32_t id) const { return m_priv->regionParentsBegin(id);}
	inline uint32_t regionParentsEnd(uint32_t id) const { return m_priv->regionParentsEnd(id); }
	inline uint32_t regionPtrSize() const { return m_priv->regionPtrSize();}
	inline uint32_t regionPtr(uint32_t pos) const { return m_priv->regionPtr(pos);}
	
	inline sserialize::spatial::GeoRect boundary(uint32_t id) const { return m_priv->boundary(id);}
	
	bool consistencyCheck(const sserialize::Static::ItemIndexStore& store) const;
	
	inline std::ostream & printStats(std::ostream & out) const { return m_priv->printStats(out); }

	inline SubSet subSet(const sserialize::CellQueryResult & cqr) const { return m_priv->subSet(cqr); }
};

}}} //end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Cell & r);
std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Region & r);

#endif