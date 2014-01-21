#ifndef SSERIALIZE_STATIC_GEO_HIERARCHY_H
#define SSERIALIZE_STATIC_GEO_HIERARCHY_H
#include <sserialize/Static/Deque.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/spatial/GeoShape.h>

#define SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION 1

namespace sserialize {
namespace Static {
namespace spatial {

/**
  *--------------------------------------------------------
  *VERSION|Regions |Cells
  *--------------------------------------------------------
  *  1Byte|MVBArray|Vector
  *  
  *Region
  *-------------------------------------------------------------------
  *CellListPtr|type|parent| id |ChildrenIndex
  *-------------------------------------------------------------------
  * vu32      | u8 |vu32  |vu32|ItemIndexPrivateRegLine
  *
  *
  *Cell
  *-----------------------------------------------------
  *ItemPtrs|Parents
  *-----------------------------------------------------
  *   vu32 |ItemIndexPrivateRegLine
  *
  */

class GeoHierarchy {
public:
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
	
	class Region {
		uint32_t m_cellPtr;
		sserialize::spatial::GeoShapeType m_type;
		uint32_t m_parent;
		uint32_t m_id;
		ItemIndex m_children;
	public:
		Region();
		///Only to be used data with getPtr = 0
		Region(sserialize::UByteArrayAdapter data);
		virtual ~Region();
		inline sserialize::spatial::GeoShapeType type() const { return m_type; }
		inline uint32_t id() const { return m_id; }
		inline uint32_t parent() const { return m_parent; }
		inline const ItemIndex & children() const { return m_children; }
		inline uint32_t cellIndexPtr() const { return m_cellPtr; }
	};
private:
	sserialize::Static::Deque<Region> m_regions;
	sserialize::Static::Deque<Cell> m_cells;
public:
	GeoHierarchy();
	GeoHierarchy(const UByteArrayAdapter & data);
	virtual ~GeoHierarchy();
	inline Cell cell(uint32_t id) const { return m_cells.at(id); }
	inline Region region(uint32_t id) const { return m_regions.at(id); }
};

}}} //end namespace

#endif