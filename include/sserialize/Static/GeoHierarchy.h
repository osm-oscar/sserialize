#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Deque.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>

#define SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION 3

namespace sserialize {
namespace Static {
namespace spatial {

/**
  *Version 1: initial draft
  *Version 2: use offset array for child/parent ptrs, use MVBitArray for node description
  *Version 3: use offset array for cell parents, use MVBitArray for cell description
  *Version 4: add boundary to regions
  *Version 5: add items pointer to regions
  *-------------------------------------------------------------------------------
  *VERSION|RegionDesc|RegionPtrs |RegionBoundaries       |CellDesc  |CellPtrs
  *-------------------------------------------------------------------------------
  *  1Byte|MVBitArray|BCUintArray|(region.size+1)*GeoRect|MVBitArray|BCUintArray
  *
  * There has to be one Region more than used. The last one defines the end for the RegionPtrs
  *
  *
  *Region
  *---------------------------------------------
  *CellListPtr|type|id|ChildrenBegin|ParentBegin
  *---------------------------------------------
  *
  *ParentBegin = offset from childrenBegin where the parent ptrs start
  *So the number of Children = ParentBegin
  *An the number of parents = Region[i+1].childrenBegin - (Region[i].childrenBegin + Region[i].ParentBegin)
  *
  *
  *Cell
  *---------------------
  *ItemPtrs|ParentsBegin
  *---------------------
  *
  *
  * The last region is the root of the dag. it has no representation in a db and is not directly accessible
  * If a Region has no parent and is not the root region, then it is a child of the root region
  * In essence, the root region is just the entrypoint to the graph
  *
  *
  */

  ///TODO:Boundingbox wie in aktueller version, also alles mit allem verwuschrteln
  ///so viel auf zellebene wie möglich
  ///Schnitte et
  
class GeoHierarchy {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef sserialize::MultiVarBitArray RegionDescriptionType;
	typedef sserialize::BoundedCompactUintArray RegionPtrListType;
	
	typedef sserialize::MultiVarBitArray CellDescriptionType;
	typedef sserialize::BoundedCompactUintArray CellPtrListType;

	class Cell {
	public:
		typedef enum {CD_ITEM_PTR=0, CD_PARENTS_BEGIN=1, CD__ENTRY_SIZE=2} CellDescriptionAccessors;
	private:
		uint32_t m_pos;
		const GeoHierarchy * m_db;;
	public:
		Cell();
		Cell(uint32_t pos, const GeoHierarchy * db);
		virtual ~Cell();
		uint32_t itemPtr() const;
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
		typedef enum {RD_CELL_LIST_PTR=0, RD_TYPE=1, RD_ID=2, RD_CHILDREN_BEGIN=3, RD_PARENTS_OFFSET=4, RD_ITEMS_PTR=5, RD__ENTRY_SIZE=6} RegionDescriptionAccessors;
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
	};
	friend class Region;
	friend class Cell;
private:
	RegionDescriptionType m_regions;
	RegionPtrListType m_regionPtrs;
	sserialize::Static::Deque<sserialize::spatial::GeoRect> m_regionBoundaries;
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
	
	uint32_t cellPtrSize() const;
	uint32_t cellPtr(uint32_t pos) const;
	const CellPtrListType & cellPtrs() const { return m_cellPtrs; }
	
	uint32_t regionSize() const;
	Region region(uint32_t id) const;
	Region rootRegion() const;
	
	uint32_t regionPtrSize() const;
	uint32_t regionPtr(uint32_t pos) const;
	
	sserialize::spatial::GeoRect boundary(uint32_t id) const;
	
	std::ostream & printStats(std::ostream & out) const;

};

}}} //end namespace

#endif