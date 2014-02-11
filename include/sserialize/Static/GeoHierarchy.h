#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Deque.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>

#define SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION 2

namespace sserialize {
namespace Static {
namespace spatial {

/**
  *Version 1: initial draft
  *Version 2: use offset array for child/parent ptrs, use MVBitArray for node description
  *
  *--------------------------------------------------------
  *VERSION|Cells |RegionDesc||RegionPtrs   
  *--------------------------------------------------------
  *  1Byte|Vector|MVBitArray|BoundedCompactUintArray
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
  *-----------------------------------------------------
  *ItemPtrs|Parents
  *-----------------------------------------------------
  *   vu32 |ItemIndexPrivateRegLine
  *
  */

  ///TODO:Boundingboxc wie in aktueller version, also alles mit allem verwuschrteln
  ///so viel auf zellebene wie möglich
  ///Schnitte et
  
class GeoHierarchy {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef sserialize::MultiVarBitArray RegionDescriptionType;
	typedef sserialize::BoundedCompactUintArray RegionPtrListType;
	

	class Cell {
		uint32_t m_itemPtr;
		ItemIndex m_parents;
	public:
		Cell();
		Cell(const UByteArrayAdapter & data);
		virtual ~Cell();
		inline uint32_t itemPtr() const { return m_itemPtr; }
		const ItemIndex & parents() const { return m_parents; }
	};
	typedef sserialize::Static::Deque<Cell> CellListType;

	
	class Region {
	public:
		typedef enum {RD_CELL_LIST_PTR=0, RD_TYPE=1, RD_ID=2, RD_CHILDREN_BEGIN=3, RD_PARENTS_OFFSET=4, RD__ENTRY_SIZE=5} RegionDescriptionAccessors;
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
	};
	friend class Region;
	friend class Cell;
private:
	CellListType m_cells;
	RegionDescriptionType m_regions;
	RegionPtrListType m_regionPtrs;
	
public:
	GeoHierarchy();
	GeoHierarchy(const UByteArrayAdapter & data);
	virtual ~GeoHierarchy();
	OffsetType getSizeInBytes() const;
	
	inline uint32_t cellSize() const { return m_cells.size(); }
	inline Cell cell(uint32_t id) const { return m_cells.at(id); }
	const CellListType & cells() const { return m_cells; }
	
	uint32_t regionSize() const;
	Region region(uint32_t id) const;
	const RegionDescriptionType & regions() const { return m_regions; }
	
	uint32_t regionPtrSize() const;
	uint32_t regionPtr(uint32_t pos) const;
	const RegionPtrListType regionPtrs() const { return m_regionPtrs; }
	
	std::ostream & printStats(std::ostream & out) const;
};

}}} //end namespace

#endif