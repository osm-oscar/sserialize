#ifndef SSERIALIZE_TESTS_GEO_STRING_COMPLETER_TEST_H
#define SSERIALIZE_TESTS_GEO_STRING_COMPLETER_TEST_H
#include "StringCompleterTest.h"


namespace sserialize {


class GeoStringCompleterTest: public StringCompleterTest {
private:
	virtual StringCompleter & stringCompleter() { return geoStringCompleter();}

protected:
	virtual GeoStringCompleter & geoStringCompleter() = 0;

public:
	GeoStringCompleterTest() : StringCompleterTest() {}
    virtual ~GeoStringCompleterTest() {}

	void testGeoCompletion() {
		
	}
};



}//end namespace

#endif