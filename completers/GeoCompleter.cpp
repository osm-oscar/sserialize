#include "GeoCompleter.h"
#include "GeoCompleterPrivate.h"

namespace sserialize {

GeoCompleter::GeoCompleter() : MyParentClass(new GeoCompleterPrivateEmpty()) {}

GeoCompleter::GeoCompleter(const GeoCompleter & other) : MyParentClass(other) {}

GeoCompleter::GeoCompleter(GeoCompleterPrivate * priv) : MyParentClass(priv) {}

GeoCompleter& GeoCompleter::operator=(const GeoCompleter & other) {
	MyParentClass::operator=(other);
	return *this;
}

ItemIndex GeoCompleter::complete(const spatial::GeoRect & rect, bool approximate) {
	return priv()->complete(rect, approximate);
}

ItemIndexIterator GeoCompleter::partialComplete(const spatial::GeoRect& rect, bool approximate) {
	return priv()->partialComplete(rect, approximate);
}

ItemIndex GeoCompleter::filter(const spatial::GeoRect& rect, bool approximate, const ItemIndex& partner) {
	return priv()->filter(rect, approximate, partner);
}

ItemIndexIterator GeoCompleter::filter(const spatial::GeoRect& rect, bool approximate, const ItemIndexIterator& partner) {
	return priv()->filter(rect, approximate, partner);
}


std::ostream& GeoCompleter::printStats(std::ostream& out) const {
	return priv()->printStats(out);
}

std::string GeoCompleter::getName() const {
	return priv()->getName();
}


}