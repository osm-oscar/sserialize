#ifndef SSERIALIZE_GEO_HIERARCHY_H
#define SSERIALIZE_GEO_HIERARCHY_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/debug.h>
#include <sserialize/containers/CFLArray.h>

namespace sserialize {
namespace spatial {

class GeoHierarchy;

namespace detail {
namespace geohierarchy {
///A List of cells storing parent regions in a contigous space
/// and storing items in cells in a contigous space

class CellList {
public:
	class Cell {
	public:
		typedef CFLArray<uint32_t> ItemsContainer;
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
		sserialize::spatial::GeoRect m_boundary;
	public:
		Cell() : m_parentsBegin(0), m_itemsBegin(0), m_parentsSize(0), m_itemsSize(0) {}
		Cell(uint32_t * cellIdBegin, uint32_t * cellIdEnd, uint32_t * cellItemsBegin, uint32_t * cellItemsEnd, const sserialize::spatial::GeoRect & boundary) :
		m_parentsBegin(cellIdBegin), m_itemsBegin(cellItemsBegin),
		m_parentsSize((uint32_t)(cellIdEnd-cellIdBegin)), m_itemsSize((uint32_t)(cellItemsEnd-cellItemsBegin)),
		m_boundary(boundary)
		{}
		Cell(uint32_t * cellIdBegin, uint32_t cellIdSize, uint32_t * cellItemsBegin, uint32_t cellItemsSize, const sserialize::spatial::GeoRect & boundary) :
		m_parentsBegin(cellIdBegin), m_itemsBegin(cellItemsBegin),
		m_parentsSize(cellIdSize), m_itemsSize(cellItemsSize),
		m_boundary(boundary)
		{}
		inline ParentsContainer parents() { return ParentsContainer(parentsBegin(), parentsSize());}
		inline ConstItemsContainer parents() const { return ConstItemsContainer(const_cast<Cell*>(this)->parentsBegin(), parentsSize());}
		
		inline ItemsContainer items() { return ItemsContainer(itemsBegin(), itemsSize());}
		inline ConstItemsContainer items() const { return ConstItemsContainer(const_cast<Cell*>(this)->itemsBegin(), itemsSize());}
		
		inline const sserialize::spatial::GeoRect & boundary() const { return m_boundary; }
		inline sserialize::spatial::GeoRect & boundary() { return m_boundary; }
		
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
		
		inline uint32_t parentAt(uint32_t pos) const { return *(parentsBegin()+pos);}
		inline uint32_t itemAt(uint32_t pos) const { return *(itemsBegin()+pos);}
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
	
	sserialize::MMVector<Cell> & cells() { return m_d; }
	const sserialize::MMVector<Cell> & cells() const { return m_d; }
	
	sserialize::MMVector<uint32_t> & cellRegionLists() { return m_cellIdData;}
	const sserialize::MMVector<uint32_t> & cellRegionLists() const { return m_cellIdData;}
	sserialize::MMVector<uint32_t> & cellItemList() { return m_cellItems;}
	const sserialize::MMVector<uint32_t> & cellItemList() const { return m_cellItems;}
	
	void swap(CellList & other);
	CellList & operator=(CellList && other);
	CellList & operator=(const CellList & other);
	
	inline uint32_t size() const { return (uint32_t)m_d.size(); }
	
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

	//A Region (occupies 96 Bytes of memory)
	///The order of ids in in the data storage is [children][parents][neighbors][cells]
	class Region final {
	private:
		friend class sserialize::spatial::GeoHierarchy;
	public:
		typedef uint32_t* iterator;
		typedef const uint32_t* const_iterator;
		typedef sserialize::CFLArray<uint32_t> DataContainerWrapper;
		typedef sserialize::CFLArray<const uint32_t> ConstDataContainerWrapper;
	private:
		DataContainer * m_d;
		uint64_t m_off;

		uint32_t m_childrenSize;
		uint32_t m_parentsSize;
		uint32_t m_neighborsSize;
		uint32_t m_cellsSize;
	public:
		uint32_t ghId;
		uint32_t storeId;
		sserialize::spatial::GeoShapeType type;
		GeoRect boundary;
	public:
		Region();
		Region(DataContainer * d, uint64_t off, uint32_t childrenSize, uint32_t parentsSize, uint32_t cellsSize, uint32_t neighborsSize);
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

		inline iterator neighborsBegin() { return parentsEnd(); }
		inline const_iterator neighborsBegin() const { return parentsEnd(); }
		inline iterator neighborsEnd() { return neighborsBegin()+neighborsSize(); }
		inline const_iterator neighborsEnd() const { return neighborsBegin()+neighborsSize(); }
		inline DataContainerWrapper neighbors() { return DataContainerWrapper(neighborsBegin(), neighborsSize());}
		inline ConstDataContainerWrapper neighbors() const { return ConstDataContainerWrapper(neighborsBegin(), neighborsSize());}
		inline uint32_t neighborsSize() const { return m_neighborsSize; }
		inline const uint32_t & neighbor(uint32_t pos) const { return *(neighborsBegin()+pos); }
		inline uint32_t & neighbor(uint32_t pos) { return *(neighborsBegin()+pos); }

		inline iterator cellsBegin() { return neighborsEnd(); }
		inline const_iterator cellsBegin() const { return neighborsEnd(); }
		inline iterator cellsEnd() { return cellsBegin()+cellsSize(); }
		inline const_iterator cellsEnd() const { return cellsBegin()+cellsSize(); }
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
	inline uint32_t size() const { return (uint32_t) m_regions.size(); }
	
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

static_assert(sizeof(RegionList::Region) <= 96, "sserialize::spatial::GeoHierarchy::detail::RegionList::Region is too large");


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
	///@param cellGraph should have a typedef Node which has a function cellId() and the function begin(), end() which iterate over the node's neighbors
	template<typename T_CELL_GRAPH>
	void createNeighborPointers(const T_CELL_GRAPH & cellGraph);
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

template<typename T_CELL_GRAPH>
inline void GeoHierarchy::createNeighborPointers(const T_CELL_GRAPH & cellGraph) {
	//since the hierarchy has only about 10M entries, we can do this very inefficient
	
	typedef typename T_CELL_GRAPH::Node Node;
	typedef sserialize::CFLArray< sserialize::MMVector<uint32_t> > RegionNeighbors;
	
	
	std::unordered_set<uint32_t> regionCells;
	std::unordered_set<uint32_t> regionBoundary;
	std::unordered_set<uint32_t> overlappingRegions;
	std::unordered_set<uint32_t> regionNeighborCandidates;
	std::vector<uint32_t> regionNeighbors;
	
	sserialize::MMVector<uint32_t> rnDataList(sserialize::MM_FILEBASED);
	sserialize::MMVector<RegionNeighbors> rnList(sserialize::MM_FILEBASED); 
	
	for(const Region & region : regions()) {
		regionCells.clear();
		regionBoundary.clear();
		overlappingRegions.clear();
		regionNeighborCandidates.clear();
		regionNeighbors.clear();
		
		regionCells.insert(region.cellsBegin(), region.cellsEnd());
		
		for(uint32_t cellId : regionCells) {
			Node n(cellGraph.node(cellId));
			for(uint32_t i(0), s(n.neighborCount()); i < s; ++i) {
				uint32_t neighborCellId = n.neighbor(i).cellId();
				if (!regionCells.count(neighborCellId)) {
					regionBoundary.insert(neighborCellId);
				}
			}
		}
		
		for(uint32_t cellId : regionCells) {
			const Cell & c = this->cell(cellId);
			overlappingRegions.insert(c.parentsBegin(), c.parentsEnd());
		}
		
		for(uint32_t cellId : regionBoundary) {
			const Cell & c = this->cell(cellId);
			regionNeighborCandidates.insert(c.parentsBegin(), c.parentsEnd());
		}
		
		for(uint32_t regionId : regionNeighborCandidates) {
			if (!overlappingRegions.count(regionId)) {
				regionNeighbors.push_back(regionId);
			}
		}
		
		std::sort(regionNeighbors.begin(), regionNeighbors.end());
		
		rnList.emplace_back(&rnDataList, rnDataList.size(), regionNeighbors.size());
		rnDataList.push_back(regionNeighbors.begin(), regionNeighbors.end());
	}
	//reassemble the region info, we can do this in-place for the region description
	//and a single copy of the region data (don't do a swap there since the regions have a pointer on the container)
	sserialize::MMVector<uint32_t> regionDataList;
	SSERIALIZE_CHEAP_ASSERT_EQUAL(regions().size(), rnList.size());
	for(uint32_t i(0), s(regions().size()); i < s; ++i) {
		Region & r = region(i);
		RegionNeighbors & rn = rnList.at(i);
		uint64_t off = regionDataList.size();
		
		regionDataList.push_back(r.childrenBegin(), r.childrenEnd()); //the old children
		regionDataList.push_back(r.parentsBegin(), r.parentsEnd()); //the old parents
		regionDataList.push_back(rn.begin(), rn.end()); //the neighbor pointers
		regionDataList.push_back(r.cellsBegin(), r.cellsEnd());//the old cells
		
		r.m_off = off; //update to new offset, rest has relative addresses
		r.m_neighborsSize = rn.size(); //set the neighbors count
	}
	
	regions().regionData().clear();
	regions().regionData().push_back(regionDataList.begin(), regionDataList.end());
	
}


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