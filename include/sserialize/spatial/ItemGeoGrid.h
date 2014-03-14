#ifndef SSERIALIZE_ITEM_GEO_GRID_H
#define SSERIALIZE_ITEM_GEO_GRID_H
#include <set>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/Static/GeoShape.h>
#include <sserialize/utility/ProgressInfo.h>
#include "RWGeoGrid.h"

namespace sserialize {
namespace spatial {

class ItemGeoGrid: public RWGeoGrid< std::set<uint32_t>* > {
protected:
	typedef RWGeoGrid< std::set<uint32_t>* > MyParentClass;
public:
	ItemGeoGrid() : MyParentClass() {}
	ItemGeoGrid(const GeoRect & rect, const uint32_t latcount, const uint32_t loncount) : MyParentClass(rect, latcount, loncount) {}
	virtual ~ItemGeoGrid() {
		for(size_t i = 0; i < storage().size(); ++i) {
			std::set<uint32_t> * &s = storage().at(i);
			if (s)
				delete s;
			s = 0;
		}
	}
	
	///@param shape can be NULL
	bool addItem(uint32_t itemId, const GeoShape * shape) {
		if (!shape)
			return false;
		std::vector<GridBin> bins = select( shape->boundary() );
		bool allOk = bins.size();
		for(size_t i = 0; i < bins.size(); ++i) {
			allOk = bins[i].valid() && allOk;
			if (bins[i].valid() && shape->intersects( cellBoundary(bins[i].x, bins[i].y) ) ) {
				std::set<uint32_t>* & tile = binAt(bins[i].x, bins[i].y);
				if ( ! tile )
					tile = new std::set<uint32_t>();
				tile->insert(itemId);
			}
		}
		return allOk;
	}

	bool addItem(uint32_t itemId, const sserialize::Static::spatial::GeoShape & shape) {
		std::vector<GridBin> bins = select( shape.boundary() );
		bool allOk = bins.size();
		size_t binsSize = bins.size();
		#pragma omp parallel for
		for(size_t i = 0; i < binsSize; ++i) {
			allOk = bins[i].valid() && allOk;
			if (bins[i].valid() && shape.intersects( cellBoundary(bins[i].x, bins[i].y) ) ) {
				#pragma omp critical
				{
					std::set<uint32_t>* & tile = binAt(bins[i].x, bins[i].y);
					if ( ! tile )
						tile = new std::set<uint32_t>();
					tile->insert(itemId);
				}
			}
		}
		return allOk;
	}

	
	using MyParentClass::serialize;
	
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination, ItemIndexFactory & indexFactory) const {
		serialize(destination);
		ProgressInfo info;
		info.begin(storage().size());
		Static::DequeCreator<uint32_t> dc(destination);
		for(size_t i = 0; i < storage().size(); ++i) {
			if (!storage().at(i))
				dc.put( indexFactory.addIndex(std::set<uint32_t>()) );
			else
				dc.put( indexFactory.addIndex( *(storage().at(i)) ) );
			info(i, "ItemGeoGrid::serialize");
		}
		dc.flush();
		info.end("ItemGeoGrid::serialize");
		return destination;
	}
};

}}//end namespace
#endif