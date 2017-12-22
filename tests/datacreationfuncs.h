#ifndef SSERIALIZE_TESTST_DATA_CREATION_FUNCTIONS_H
#define SSERIALIZE_TESTST_DATA_CREATION_FUNCTIONS_H
#include <deque>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sserialize/spatial/GeoPolygon.h>

namespace sserialize {


template<typename T_OUTPUT_ITERATOR, typename T_TYPE = typename std::iterator_traits<T_OUTPUT_ITERATOR>::value_type >
struct TestDataGenerator {
	static void generate(uint32_t count, T_OUTPUT_ITERATOR out);
};

template<typename T_OUTPUT_ITERATOR>
struct TestDataGenerator<T_OUTPUT_ITERATOR, uint32_t> {
	static void generate(uint32_t count, T_OUTPUT_ITERATOR out) {
		for(uint32_t i(0); i < count; ++i) {
			*out = rand();
			++out;
		}
	}
};

//starting from begin, end is one passend the end
template<typename TCONTAINER>
void addRange(uint32_t begin, uint32_t end, TCONTAINER & dest) {
	for(;begin < end; ++begin)
		dest.insert(dest.end(), begin);
}

std::string createString(uint32_t maxStrLen);
std::vector<std::string> createStringsVec(uint32_t maxStrLen, uint32_t strCount);
std::deque<std::string> createStrings(uint32_t maxStrLen, uint32_t strCount);
std::deque<uint32_t> createNumbers(uint32_t count);
std::deque<uint16_t> createNumbers16(uint32_t count);
std::deque<uint8_t> createNumbers8(uint32_t count);


std::set<std::string> createStringsSet(uint32_t maxStrLen, uint32_t strCount);
std::set<uint32_t> createNumbersSet(uint32_t count);
std::set<uint32_t> createNumbersSet(uint32_t start, uint32_t end, uint32_t minDiff, uint32_t addDiff);
std::set<uint16_t> createNumbers16Set(uint32_t count);

void createOverLappingSets(std::set<uint32_t> & a, std::set<uint32_t> & b, uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand);
void createOverLappingSets(std::set<uint32_t> & a, std::set<uint32_t> & b, uint32_t maxValue, uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand);
void createOverLappingSets(uint32_t count, uint32_t minEqual, uint32_t sizeVariance, std::deque< std::set<uint32_t> > & destination);
void createOverLappingSets(uint32_t count, uint32_t minEqual, uint32_t sizeVariance, std::vector< std::vector<uint32_t> > & destination);


struct SamplePolygonTestData {
	inline sserialize::spatial::GeoPolygon poly(uint32_t p) { return polys.at(p).first; }
	inline sserialize::spatial::GeoPoint point(uint32_t p) { return points.at(p);}
	std::vector<  std::pair<sserialize::spatial::GeoPolygon, uint32_t> > polys;
	std::vector<sserialize::spatial::GeoPoint> points;
	std::set< std::pair< uint32_t, uint32_t > > polyIntersects;
	std::set< std::pair< uint32_t, uint32_t > > pointPolyIntersects;
	std::set< std::pair< uint32_t, uint32_t > > polyPointsInOtherPolys; //first: the points of poly, second: the poly sourrounding points from first
	
	std::vector< spatial::GeoRect > rects;
	std::vector< std::set<uint32_t> > rectPolyIds;
	std::vector< std::set<uint32_t> > rectPointIds;
};



typedef enum { POLY_A=0, POLY_B=1, POLY_C=2, POLY_D=3, POLY_E=4, POLY_F=5, POLY_G=6, POLY_H=7, POLY_I=8, POLY_J=9, POLY_COUNT=10} HandSampledPolys;
void createHandSamplePolygons(SamplePolygonTestData & data);


}

#endif
