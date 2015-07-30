#include <sserialize/search/GeoStringCompleter.h>

namespace sserialize {

GeoStringCompleterPrivate::GeoStringCompleterPrivate() : MyParentClass() {}
GeoStringCompleterPrivate::~GeoStringCompleterPrivate() {}

GeoStringCompleterPrivateEmpty::GeoStringCompleterPrivateEmpty() : MyParentClass() {}
GeoStringCompleterPrivateEmpty::~GeoStringCompleterPrivateEmpty() {}

ItemIndex GeoStringCompleterPrivateEmpty::complete(const spatial::GeoRect& /*rect*/, bool /*approximate*/) {
	return ItemIndex();
}

std::string GeoStringCompleterPrivateEmpty::getName() const{
	return std::string("GeoStringCompleterPrivateEmpty");
}

std::ostream& GeoStringCompleterPrivate::printStats ( std::ostream& out ) const {
	return out << std::string("GeoStringCompleterPrivateEmpty: just empty");
}

GeoStringCompleter::GeoStringCompleter() : MyParentClass( new GeoStringCompleterPrivateEmpty() ) {}
GeoStringCompleter::GeoStringCompleter(const GeoStringCompleter & other) : MyParentClass(other) {}
GeoStringCompleter::GeoStringCompleter(GeoStringCompleterPrivate * priv) : MyParentClass(priv) {}

GeoStringCompleter & GeoStringCompleter::operator=(const GeoStringCompleter& other) {
	MyParentClass::operator=(other);
	return *this;
}

ItemIndex GeoStringCompleter::complete(const spatial::GeoRect& rect, bool approximate) {
	return priv()->complete(rect, approximate);
}

std::ostream& GeoStringCompleter::printStats(std::ostream& out) const {
	return priv()->printStats(out);
}

std::string GeoStringCompleter::getName() const {
	return priv()->getName();
}

GeoStringCompleterPrivate* GeoStringCompleter::getPrivate() {
	return priv();
}




}//End namespace
