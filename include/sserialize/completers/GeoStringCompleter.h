#ifndef SSERIALIZE_GEO_STRING_COMPLETER_H
#define SSERIALIZE_GEO_STRING_COMPLETER_H
#include "StringCompleter.h"
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/spatial/GeoRect.h>

namespace sserialize {

class GeoStringCompleterPrivate: public StringCompleterPrivate {
protected:
	typedef StringCompleterPrivate MyParentClass;
public:
	GeoStringCompleterPrivate();
	virtual ~GeoStringCompleterPrivate();
	using StringCompleterPrivate::complete;
	virtual ItemIndex complete(const spatial::GeoRect & rect, bool approximate) = 0;
	std::ostream& printStats(std::ostream& out) const = 0;
	std::string getName() const = 0;
};

class GeoStringCompleterPrivateEmpty: public GeoStringCompleterPrivate {
protected:
	typedef GeoStringCompleterPrivate MyParentClass;
public:
	GeoStringCompleterPrivateEmpty();
	virtual ~GeoStringCompleterPrivateEmpty();
	virtual ItemIndex complete(const spatial::GeoRect & rect, bool approximate);
	std::ostream& printStats(std::ostream& out) const;
	std::string getName() const;
};


class GeoStringCompleter: public StringCompleter {
protected:
	typedef StringCompleter MyParentClass;
	GeoStringCompleterPrivate * priv() const {
		return static_cast<GeoStringCompleterPrivate*>( priv() );
	}
public:
	GeoStringCompleter();
	GeoStringCompleter(const GeoStringCompleter & other);
	GeoStringCompleter(GeoStringCompleterPrivate * priv);
	GeoStringCompleter & operator=(const GeoStringCompleter & strc);
	ItemIndex complete(const spatial::GeoRect & rect, bool approximate = false);
	using MyParentClass::complete;
	GeoStringCompleterPrivate * getPrivate();
	std::ostream& printStats(std::ostream& out) const;
	std::string getName() const;
};
}


#endif