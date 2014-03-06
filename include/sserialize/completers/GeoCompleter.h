#ifndef SSERIALIZE_GEO_COMPLETER_H
#define SSERIALIZE_GEO_COMPLETER_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/completers/GeoCompleterPrivate.h>

namespace sserialize {

class GeoCompleter: public RCWrapper<GeoCompleterPrivate> {
protected:
	typedef RCWrapper<GeoCompleterPrivate> MyParentClass;
public:
	GeoCompleter();
	GeoCompleter(const GeoCompleter & other);
	GeoCompleter(GeoCompleterPrivate * priv);
	GeoCompleter& operator=(const GeoCompleter & other);
	ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool approximate);
	ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool approximate);
	ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndex & partner);
	ItemIndexIterator filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndexIterator & partner);
	std::ostream & printStats(std::ostream & out) const;
	std::string getName() const;
};

}//end namespace

#endif