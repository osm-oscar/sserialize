#ifndef SSERIALIZE_GEO_COMPLETER_PRIVATE_H
#define SSERIALIZE_GEO_COMPLETER_PRIVATE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexIterator.h>
#include <sserialize/spatial/GeoRect.h>

namespace sserialize {

class GeoCompleterPrivate: public RefCountObject {
public:
	GeoCompleterPrivate() : RefCountObject() {}
	virtual ~GeoCompleterPrivate() {}
	virtual ItemIndex complete(const spatial::GeoRect & rect, bool approximate) = 0;
	virtual ItemIndexIterator partialComplete(const spatial::GeoRect & rect, bool approximate) {
		return ItemIndexIterator( complete(rect, approximate) );
	}
	virtual ItemIndex filter(const spatial::GeoRect& rect, bool approximate, const ItemIndex& partner) = 0;
	virtual ItemIndexIterator filter(const spatial::GeoRect& rect, bool approximate, const ItemIndexIterator& partner) = 0;
	
	virtual std::ostream & printStats(std::ostream & out) const = 0;
	virtual std::string getName() const = 0;
};

class GeoCompleterPrivateEmpty: public GeoCompleterPrivate {
public:
	GeoCompleterPrivateEmpty() : GeoCompleterPrivate() {}
	virtual ~GeoCompleterPrivateEmpty() {}
	virtual ItemIndex complete(const spatial::GeoRect & rect, bool approximate) { return ItemIndex(); }
	virtual ItemIndex filter(const spatial::GeoRect& rect, bool approximate, const ItemIndex& partner) { return ItemIndex(); }
	virtual ItemIndexIterator filter(const spatial::GeoRect& rect, bool approximate, const ItemIndexIterator& partner) { return ItemIndexIterator(); }
	virtual std::ostream & printStats(std::ostream & out) const { return out; }
	virtual std::string getName() const { return "GeoCompleterPrivateEmpty"; }
};


}//end namespace
#endif