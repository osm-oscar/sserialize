#ifndef SSERIALAIZE_SPATIAL_RW_GEO_GRID_H
#define SSERIALAIZE_SPATIAL_RW_GEO_GRID_H
#include "GeoGrid.h"
#include <stdexcept>
#include <sstream>

namespace sserialize {
namespace spatial {

template<typename TValue, class StorageContainer = std::vector<TValue>>
class RGeoGrid: public GeoGrid {
public:
	typedef GeoGrid MyParentClass;
	typedef StorageContainer StorageContainerType;
private:
	StorageContainerType m_storage;
public:
	RGeoGrid() : GeoGrid() {}
	RGeoGrid(const GeoGrid & gridBase) : GeoGrid(gridBase) {}
	RGeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : GeoGrid(rect, latcount, loncount) {}
	virtual ~RGeoGrid() {}
	size_t size() const { return m_storage.size(); }
	StorageContainerType & storage() { return m_storage; }
	const StorageContainerType & storage() const { return m_storage; }
	
	TValue at(double lat, double lon) const {
		GeoGrid::GridBin bin(  GeoGrid::select(lat, lon) );
		return m_storage.at(bin.tile);
	}
	
	TValue at(const spatial::GeoPoint & point) const {
		return at(point.lat(), point.lon());
	}
	
	TValue at(uint32_t tile) const {
		return m_storage.at(tile);
	}
	
	TValue binAt(uint32_t x, uint32_t y) const {
		return m_storage.at( GeoGrid::selectBin(x,y));
	}
	
	TValue binAt(uint32_t tile) const {
		return m_storage.at( tile );
	}
};

template<typename TValue, class StorageContainer = std::vector<TValue> >
class RWGeoGrid: public RGeoGrid<TValue, StorageContainer> {
protected:
	typedef RGeoGrid<TValue, StorageContainer> MyParentClass;
public:
	RWGeoGrid() : RGeoGrid<TValue, StorageContainer>() {}
	RWGeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : RGeoGrid<TValue, StorageContainer>(rect, latcount, loncount) {
		MyParentClass::storage().resize(latcount*loncount);
	}
	virtual ~RWGeoGrid() {}
	TValue & at(double lat, double lon) {
		GeoGrid::GridBin bin(  GeoGrid::select(lat, lon) );
		if (!bin.valid()) {
			std::stringstream ss;
			ss << "RWGeoGrid::FatalError: (" << lat << ", " << lon << ") access is out of range";
			throw std::out_of_range(ss.str());
		}
		return RGeoGrid<TValue, StorageContainer>::storage().at(bin.tile);
	}
	const TValue & at(double lat, double lon) const {
		GeoGrid::GridBin bin(  GeoGrid::select(lat, lon) );
		if (!bin.valid()) {
			std::stringstream ss;
			ss << "RWGeoGrid::FatalError: (" << lat << ", " << lon << ") access is out of range";
			throw std::out_of_range(ss.str());
		}
		return RGeoGrid<TValue, StorageContainer>::storage().at(bin.tile);
	}
	
	const TValue & at(uint32_t tile) const {
		return binAt(tile);
	}
	
	TValue & at(uint32_t tile) {
		return binAt(tile);
	}
	
	TValue & binAt(uint32_t x, uint32_t y) {
		return RGeoGrid<TValue, StorageContainer>::storage().at( GeoGrid::selectBin(x, y) );
	}
	
	const TValue & binAt(uint32_t tile) const {
		return RGeoGrid<TValue, StorageContainer>::storage().at( tile );
	}
	
	TValue & binAt(uint32_t tile) {
		return RGeoGrid<TValue, StorageContainer>::storage().at( tile );
	}
};

}}//end namespace

#endif