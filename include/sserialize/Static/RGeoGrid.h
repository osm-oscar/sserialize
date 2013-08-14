#ifndef SSERIALIZE_STATIC_RGEO_GRID_H
#define SSERIALIZE_STATIC_RGEO_GRID_H
#include <sserialize/utility/log.h>
#include <sserialize/spatial/RWGeoGrid.h>
#include <sserialize/Static/Deque.h>

namespace sserialize {
namespace Static{
namespace spatial {

template<typename TValue>
class RGeoGrid: public sserialize::spatial::RGeoGrid<TValue, sserialize::Static::Deque<TValue> > {
protected:
	typedef sserialize::spatial::RGeoGrid<TValue, sserialize::Static::Deque<TValue> > MyParentClass;
	uint16_t m_headerSize;
public:
	RGeoGrid() : MyParentClass() {}
	RGeoGrid(UByteArrayAdapter data) : MyParentClass() {
		Static::spatial::GeoPoint bL, tR;
		uint32_t latcount, loncount;
		data += 1; //header
		data >> bL >> tR >> latcount >> loncount;
		m_headerSize = 1 + data.tellGetPtr();
		MyParentClass::storage() = Static::Deque<uint32_t>( data.shrinkToGetPtr() );
		
		if (latcount*loncount > MyParentClass::storage().size()) {
			sserialize::err("RGeoGrid", "Broken Grid detected");
			latcount = 0;
			loncount = 0;
			return;
		}
		
		//everythings alright here
		sserialize::spatial::GeoRect rect;
		rect.lat()[0] = bL.latD();
		rect.lon()[0] = bL.lonD();
		rect.lat()[1] = tR.latD();
		rect.lon()[1] = tR.lonD();
		MyParentClass::setGridInfo(rect, latcount, loncount);
	}
	uint32_t getSizeInBytes() const {
		return m_headerSize + MyParentClass::storage().getSizeInBytes();
	}
};


}}}//end namespace




#endif