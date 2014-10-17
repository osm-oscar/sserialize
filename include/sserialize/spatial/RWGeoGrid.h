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
	typedef StorageContainer StorageContainerType;
private:
	StorageContainerType m_storage;
protected:
	typedef GeoGrid MyParentClass;
public:
	RGeoGrid() : GeoGrid() {}
	RGeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : GeoGrid(rect, latcount, loncount) {}
	virtual ~RGeoGrid() {}
	size_t size() const { return m_storage.size(); }
	StorageContainerType & storage() { return m_storage; }
	const StorageContainerType & storage() const { return m_storage; }
	
	TValue at(double lat, double lon) const {
		GeoGrid::GridBin bin(  GeoGrid::select(lat, lon) );
		if (!bin.valid())
			return TValue();
		return m_storage.at(bin.tile);
	}
	
	TValue at(const spatial::GeoPoint & point) const {
		return at(point.lat(), point.lon());
	}
	
	TValue binAt(uint32_t x, uint32_t y) const {
		if (this->latCount() <= x || this->lonCount() <= y)
			return TValue();
		return m_storage.at( GeoGrid::selectBin(x,y));
	}
	
	TValue binAt(uint32_t tile) const {
		if (tile >= storage().size())
			return TValue();
		return m_storage.at( tile );
	}
	

	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const {
		destination << static_cast<uint8_t>(0);
		destination << spatial::GeoPoint(MyParentClass::rect().lat()[0], MyParentClass::rect().lon()[0]);
		destination << spatial::GeoPoint(MyParentClass::rect().lat()[1], MyParentClass::rect().lon()[1]);
		destination << latCount();
		destination << lonCount();
		return destination;
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
	
	TValue & binAt(uint32_t x, uint32_t y) {
		return RGeoGrid<TValue, StorageContainer>::storage().at( GeoGrid::selectBin(x, y) );
	}
};

}}//end namespace

#endif