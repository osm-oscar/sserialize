#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Array.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/containers/CellQueryResult.h>

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

class GeoHierarchy {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef sserialize::MultiVarBitArray RegionDescriptionType;
	typedef sserialize::BoundedCompactUintArray RegionPtrListType;
	
	typedef sserialize::MultiVarBitArray CellDescriptionType;
	typedef sserialize::BoundedCompactUintArray CellPtrListType;

	class Cell {
	public:
		typedef enum {CD_ITEM_PTR=0, CD_ITEM_COUNT=1, CD_PARENTS_BEGIN=2, CD__ENTRY_SIZE=3} CellDescriptionAccessors;
	private:
		uint32_t m_pos;
		const GeoHierarchy * m_db;;
	public:
		Cell();
		Cell(uint32_t pos, const GeoHierarchy * db);
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
		const GeoHierarchy * m_db;
	public:
		Region();
		///Only to be used data with getPtr = 0
		Region(uint32_t pos, const GeoHierarchy * db);
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
	friend class Region;
	friend class Cell;

	class SubSet {
	public:
		class Node {
		public:
			typedef enum {T_CELL, T_REGION} Type;
			typedef std::vector<Node*> ChildrenStorageContainer;
		private:
			Type m_type;
			uint32_t m_id;
			std::vector<Node*> m_children;
			uint32_t m_itemSize;
		public:
			Node(Type t, uint32_t id, uint32_t itemSize = 0) : m_type(t), m_id(id), m_itemSize(itemSize) {}
			virtual ~Node();
			inline uint32_t id() const { return m_id; }
			inline uint32_t size() const { return m_children.size(); }
			inline Node * operator[](uint32_t pos) { return m_children[pos]; }
			inline Node * at(uint32_t pos) { return m_children.at(pos);}
			inline void push_back(Node * child) { m_children.push_back(child);}
			inline uint32_t itemSize() const { return m_itemSize; }
			inline uint32_t & itemSize() { return m_itemSize; }
		};
	private:
		std::shared_ptr<Node> m_root;
	public:
		SubSet() {}
		SubSet(Node * root) : m_root(root) {}
		virtual ~SubSet()  {}
		inline Node* root() const { return m_root.get();}
	};

private:
	RegionDescriptionType m_regions;
	RegionPtrListType m_regionPtrs;
	sserialize::Static::Array<sserialize::spatial::GeoRect> m_regionBoundaries;
	CellDescriptionType m_cells;
	CellPtrListType m_cellPtrs;
protected:
	const RegionDescriptionType & regions() const { return m_regions; }
	const CellDescriptionType & cells() const { return m_cells; }
	const RegionPtrListType & regionPtrs() const { return m_regionPtrs; }
public:
	GeoHierarchy();
	GeoHierarchy(const UByteArrayAdapter & data);
	virtual ~GeoHierarchy();
	OffsetType getSizeInBytes() const;
	
	uint32_t cellSize() const;
	Cell cell(uint32_t id) const;
	
	uint32_t cellParentsBegin(uint32_t id) const;
	uint32_t cellParentsEnd(uint32_t id) const;
	uint32_t cellPtrSize() const;
	uint32_t cellPtr(uint32_t pos) const;
	const CellPtrListType & cellPtrs() const { return m_cellPtrs; }
	uint32_t cellItemsPtr(uint32_t pos) const;
	uint32_t cellItemsCount(uint32_t pos) const;
	
	uint32_t regionSize() const;
	Region region(uint32_t id) const;
	Region rootRegion() const;
	
	uint32_t regionItemsPtr(uint32_t pos) const;
	uint32_t regionItemsCount(uint32_t pos) const;
	
	uint32_t regionParentsBegin(uint32_t id) const;
	uint32_t regionParentsEnd(uint32_t id) const;
	uint32_t regionPtrSize() const;
	uint32_t regionPtr(uint32_t pos) const;
	
	sserialize::spatial::GeoRect boundary(uint32_t id) const;
	
	bool consistencyCheck(const sserialize::Static::ItemIndexStore& store) const;
	
	std::ostream & printStats(std::ostream & out) const;

	SubSet subSet(const sserialize::CellQueryResult & cqr) const;
};

}}} //end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Cell & r);
std::ostream & operator<<(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::Region & r);

#endif