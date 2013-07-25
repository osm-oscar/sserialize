#ifndef SSERIALIZE_STATIC_GEO_COMPLETER_H
#define SSERIALIZE_STATIC_GEO_COMPLETER_H
#include <sserialize/completers/GeoCompleter.h>
#include <sserialize/completers/GeoCompleterPrivateProxy.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/ItemGeoGrid.h>

namespace sserialize {
namespace Static {

struct GeoCompleter {
	
	template<typename DataBaseType>
	static sserialize::GeoCompleter fromDB(const DataBaseType & db) {
		return sserialize::GeoCompleter(new sserialize::GeoCompleterPrivateProxy< DataBaseType >(db));
	}
	
	template<typename DataBaseType>
	static sserialize::GeoCompleter fromData(const UByteArrayAdapter & data, const sserialize::Static::ItemIndexStore & indexStore, const DataBaseType & db) {
		GeoCompleterPrivateProxy< sserialize::Static::spatial::ItemGeoGrid<DataBaseType> > * gcp = 
			new GeoCompleterPrivateProxy< Static::spatial::ItemGeoGrid<DataBaseType> >(
				sserialize::Static::spatial::ItemGeoGrid<DataBaseType>(data, db, indexStore)
				);
		return sserialize::GeoCompleter(gcp);
	}
};


}}//end namespace


#endif