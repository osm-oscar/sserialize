#ifndef SSERIALIZE_GEO_HIERARCHY_H
#define SSERIALIZE_GEO_HIERARCHY_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/debug.h>
#include <sserialize/containers/IFLArray.h>

namespace sserialize {
namespace spatial {

class GeoHierarchy;

namespace detail {
namespace geohierarchy {

class CellList {
public:
	class Cell {
	public:
		typedef IFLArray<uint32_t> ItemsContainer;
		typedef ItemsContainer ConstItemsContainer;
		typedef ItemsContainer ParentsContainer;
		typedef ItemsContainer ConstParentsContainer;
	private:
		friend class CellList;
		friend class sserialize::spatial::GeoHierarchy;
	private:
		uint32_t * m_parentsBegin;
		uint32_t * m_itemsBegin;
		uint32_t m_parentsSize;
		uint32_t m_itemsSize;
	public:
		Cell() : m_parentsBegin(0), m_itemsBegin(0), m_parentsSize(0), m_itemsSize(0) {}
		Cell(uint32_t * cellIdBegin, uint32_t * cellIdEnd, uint32_t * cellItemsBegin, uint32_t * cellItemsEnd) :
		m_parentsBegin(cellIdBegin), m_itemsBegin(cellItemsBegin),
		m_parentsSize(cellIdEnd-cellIdBegin), m_itemsSize(cellItemsEnd-cellItemsBegin)
		{}
		Cell(uint32_t * cellIdBegin, uint32_t cellIdSize, uint32_t * cellItemsBegin, uint32_t cellItemsSize) :
		m_parentsBegin(cellIdBegin), m_itemsBegin(cellItemsBegin),
		m_parentsSize(cellIdSize), m_itemsSize(cellItemsSize)
		{}
		ParentsContainer parents() { return (parentsBegin(), parentsSize());}
		ConstItemsContainer parents() const { return ConstItemsContainer(const_cast<Cell*>(this)->parentsBegin(), parentsSize());}
		
		ItemsContainer items() { return ItemsContainer(itemsBegin(), itemsSize());}
		ConstItemsContainer items() const { return ConstItemsContainer(const_cast<Cell*>(this)->itemsBegin(), itemsSize());}
		
		inline uint32_t * parentsBegin() { return m_parentsBegin; }
		inline const uint32_t * parentsBegin() const { return m_parentsBegin; }
		
		inline uint32_t * parentsEnd() { return m_parentsBegin+m_parentsSize; }
		inline const uint32_t * parentsEnd() const { return m_parentsBegin+m_parentsSize; }
		inline uint32_t parentsSize() const { return m_parentsSize; }
		
		inline uint32_t * itemsBegin() { return m_itemsBegin; }
		inline const uint32_t * itemsBegin() const { return m_itemsBegin; }
		inline uint32_t * itemsEnd() { return m_itemsBegin+m_itemsSize; }
		inline const uint32_t * itemsEnd() const { return m_itemsBegin+m_itemsSize; }

		inline uint32_t itemsSize() const { return m_itemsSize; }
		
		uint32_t parentAt(uint32_t pos) const { return *(parentsBegin()+pos);}
		uint32_t itemAt(uint32_t pos) const { return *(itemsBegin()+pos);}
	};
	typedef sserialize::MMVector<Cell>::const_iterator const_iterator;
	typedef sserialize::MMVector<Cell>::iterator iterator;
private:
	sserialize::MMVector<uint32_t> m_cellIdData;
	sserialize::MMVector<uint32_t> m_cellItems;
	sserialize::MMVector<Cell> m_d;
public:
	CellList();
	CellList(const sserialize::MMVector<uint32_t> & cellIdData, const sserialize::MMVector<uint32_t> & cellItems, const sserialize::MMVector<Cell> & cells);
	CellList(sserialize::MMVector<uint32_t> && cellIdData, sserialize::MMVector<uint32_t> && cellItems, sserialize::MMVector<Cell> && cells);
	CellList(const CellList & other);
	CellList(const CellList && other);
	~CellList();
	sserialize::MMVector<uint32_t> & cellItemList() { return m_cellItems;}
	const sserialize::MMVector<uint32_t> & cellItemList() const { return m_cellItems;}
	
	void swap(CellList & other);
	CellList & operator=(CellList && other);
	CellList & operator=(const CellList & other);
	
	inline uint32_t size() const { return m_d.size(); }
	
	inline Cell & operator[](uint32_t pos) { return m_d[pos]; }
	inline const Cell & operator[](uint32_t pos) const { return m_d[pos]; }
	
	inline Cell & at(uint32_t pos) { return m_d.at(pos); }
	inline const Cell & at(uint32_t pos) const { return m_d.at(pos); }
	
	inline iterator begin() { return m_d.begin(); }
	inline const_iterator begin() const { return m_d.begin(); }
	inline const_iterator cbegin() const { return m_d.cbegin(); }
	
	inline iterator end() { return m_d.end(); }
	inline const_iterator end() const { return m_d.end(); }
	inline const_iterator cend() const { return m_d.cend(); }
};

inline void swap(CellList & a, CellList & b) {
	a.swap(b);
}

/// A list for regions storing cell, parent and child ids in an contigous array
/// 
class RegionList {
private:
	friend class sserialize::spatial::GeoHierarchy;
public:
	typedef sserialize::MMVector<uint32_t> DataContainer;

	//A Region (occupies 72 Bytes of memory)
	///The order of ids in in the data storage is [children][parents][cells]
	class Region final {
	private:
		friend class sserialize::spatial::GeoHierarchy;
	public:
		typedef uint32_t* iterator;
		typedef const uint32_t* const_iterator;
		typedef sserialize::IFLArray<uint32_t> DataContainerWrapper;
		typedef sserialize::IFLArray<const uint32_t> ConstDataContainerWrapper;
	private:
		DataContainer * m_d;
		uint64_t m_off;

		uint32_t m_childrenSize;
		uint32_t m_parentsSize;
		uint32_t m_cellsSize;
	public:
		uint32_t ghId;
		uint32_t storeId;
		sserialize::spatial::GeoShapeType type;
		GeoRect boundary;
	public:
		Region();
		Region(DataContainer * d, uint64_t off, uint32_t childrenSize, uint32_t parentsSize, uint32_t cellsSize);
		Region(const Region & other);
		~Region() {}
		void swap(Region & other);
		inline iterator dataBegin() { return m_d->begin()+m_off; }
		inline const_iterator dataBegin() const { return m_d->begin()+m_off; }
		inline iterator childrenBegin() { return dataBegin(); }
		inline const_iterator childrenBegin() const { return dataBegin(); }
		inline iterator childrenEnd() { return childrenBegin()+m_childrenSize; }
		inline const_iterator childrenEnd() const { return childrenBegin()+m_childrenSize; }
		inline DataContainerWrapper children() { return DataContainerWrapper(childrenBegin(), childrenSize());}
		inline ConstDataContainerWrapper children() const { return ConstDataContainerWrapper(childrenBegin(), childrenSize());}
		inline uint32_t childrenSize() const { return m_childrenSize; }
		inline const uint32_t & child(uint32_t pos) const { return *(childrenBegin()+pos); }
		inline uint32_t & child(uint32_t pos) { return *(childrenBegin()+pos); }
		
		inline iterator parentsBegin() { return childrenEnd(); }
		inline const_iterator parentsBegin() const { return childrenEnd(); }
		inline iterator parentsEnd() { return parentsBegin()+m_parentsSize; }
		inline const_iterator parentsEnd() const { return parentsBegin()+m_parentsSize; }
		inline DataContainerWrapper parents() { return DataContainerWrapper(parentsBegin(), parentsSize());}
		inline ConstDataContainerWrapper parents() const { return ConstDataContainerWrapper(parentsBegin(), parentsSize());}
		inline uint32_t parentsSize() const { return m_parentsSize; }
		inline const uint32_t & parent(uint32_t pos) const { return *(parentsBegin()+pos); }
		inline uint32_t & parent(uint32_t pos) { return *(parentsBegin()+pos); }

		inline iterator cellsBegin() { return parentsEnd(); }
		inline const_iterator cellsBegin() const { return parentsEnd(); }
		inline iterator cellsEnd() { return cellsBegin()+m_cellsSize; }
		inline const_iterator cellsEnd() const { return cellsBegin()+m_cellsSize; }
		inline DataContainerWrapper cells() { return DataContainerWrapper(cellsBegin(), cellsSize());}
		inline ConstDataContainerWrapper cells() const { return ConstDataContainerWrapper(cellsBegin(), cellsSize());}
		inline uint32_t cellsSize() const { return m_cellsSize; }
		inline const uint32_t & cell(uint32_t pos) const { return *(cellsBegin()+pos); }
		inline uint32_t & cell(uint32_t pos) { return *(cellsBegin()+pos); }
	};
	typedef sserialize::MMVector<Region> RegionListContainer;
	typedef RegionListContainer::iterator iterator;
	typedef RegionListContainer::const_iterator const_iterator;
	
private:
	DataContainer m_data;
	RegionListContainer m_regions;
public:
	RegionList() {}
	~RegionList() {}
	void clear();
	inline uint32_t size() const { return m_regions.size(); }
	
	DataContainer & regionData() { return m_data; }
	const DataContainer & regionData() const { return m_data; }
	
	RegionListContainer & regionDescriptions() { return m_regions; }
	const RegionListContainer & regionDescriptions() const { return m_regions; }
	
	const Region & at(uint32_t pos) const { return m_regions.at(pos); }
	Region & at(uint32_t pos) { return m_regions.at(pos); }

	const Region & operator[](uint32_t pos) const { return m_regions[pos]; }
	Region & operator[](uint32_t pos) { return m_regions[pos]; }
	
	inline iterator begin() { return m_regions.begin(); }
	inline const_iterator begin() const { return m_regions.begin(); }
	inline const_iterator cbegin() const { return m_regions.cbegin(); }
	
	inline iterator end() { return m_regions.end(); }
	inline const_iterator end() const { return m_regions.end(); }
	inline const_iterator cend() const { return m_regions.cend(); }
	
};

inline void swap(RegionList::Region & a, RegionList::Region & b) {
	a.swap(b);
}

// static_assert(sizeof(RegionList::Region) <= 72, "sserialize::spatial::GeoHierarchy::detail::RegionList::Region is too large");


}}//end namespace detail::geohierarchy

//Regions are sorted in ascending order, meaning that children are before parents
class GeoHierarchy {
public:
	//Pointers are sorted

	typedef detail::geohierarchy::RegionList::Region Region;
	typedef detail::geohierarchy::CellList::Cell Cell;
	
	typedef detail::geohierarchy::CellList CellList;
	typedef detail::geohierarchy::RegionList RegionList;
	
private:
	CellList m_cells;
	RegionList m_regions; //the largest region is the last region
	Region m_rootRegion;
private:
	void depths(std::vector<uint32_t> & d, uint32_t me) const;
	template<typename T_SET_CONTAINER>
	void getAncestors(uint32_t regionId, T_SET_CONTAINER & out) const;
	//returned parents are sorted
	void splitCellParents(uint32_t cellId, std::vector<uint32_t> & directParents, std::vector<uint32_t> & remainingParents) const;
	//check functions
	bool regionEqTest(uint32_t i, const sserialize::spatial::GeoHierarchy::Region& r, const sserialize::Static::spatial::GeoHierarchy::Region& sr, const sserialize::Static::ItemIndexStore* idxStore) const;

public:
	GeoHierarchy() {}
	virtual ~GeoHierarchy() {}
	///This sets the root region which has every node without a parent as child
	///Call this only once
	void createRootRegion();
	inline CellList & cells() { return m_cells; }
	inline RegionList & regions() { return m_regions;}
	inline const CellList & cells() const { return m_cells; }
	inline const RegionList & regions() const { return m_regions;}
	inline const Cell & cell(uint32_t pos) const { return m_cells.at(pos);}
	inline Cell & cell(uint32_t pos) { return m_cells.at(pos);}
	inline const Region & region(uint32_t pos) const { return m_regions.at(pos);}
	inline Region & region(uint32_t pos) { return m_regions.at(pos);}
	bool checkConsistency();
	///data structure has to be consistent before using this
	UByteArrayAdapter append(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory, bool fullItemsIndex = true) const;
	void printStats(std::ostream & out) const;
	bool testEquality(const sserialize::Static::spatial::GeoHierarchy & sgh, const sserialize::Static::ItemIndexStore * idxStore = 0) const;
	///get the regions in level order, where the level of a region is defined by the longest path from root to it self
	std::vector<uint32_t> getRegionsInLevelOrder() const;
	///@return first = id, second = size
	std::vector< std::pair< uint32_t, uint32_t > > createFullRegionItemIndex(sserialize::ItemIndexFactory& idxFactory) const;
	
	///This reduces the size of the hierarchy by eliminating redundancy:
	///For each Cell only those parents are kept, that are not an ancestor of another parent of the cell
	///For each region, only those cells are kept, that are not part of a descendant of the region
	///Calculating aproximate region item counts during completion is then more involved
	///This should only be called before serialization
	void compactify(bool compactifyCells, bool compactifyRegions);
};

template<typename T_SET_CONTAINER>
void GeoHierarchy::getAncestors(uint32_t regionId, T_SET_CONTAINER & out) const {
	const Region & r = m_regions[regionId];
	out.insert(r.parentsBegin(), r.parentsEnd());
	for(uint32_t parent : r.parents()) {
		getAncestors(parent, out);
	}
}

}} //end namespace


#endif