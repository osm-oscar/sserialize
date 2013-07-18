#include "datacreationfuncs.h"
#include <stdlib.h>

namespace sserialize {

std::string baseStringChars("ABCDEFGHIJKLMOPQRSTUVWXYZabcdevghijklmnopqrstuvxyz1234567890!$%&/{}()[]+-*~<> ");


std::deque<std::string> createStrings(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<std::string> ret;
	for(size_t i = 0; i< strCount; i++) {
		size_t strLen = (double)rand() / RAND_MAX * maxStrLen;
		std::string newStr;
		for(size_t j = 0; j < strLen; j++) {
			newStr += baseStringChars[(double)rand() / RAND_MAX * (baseStringChars.size()-1)];
		}
		ret.push_back(newStr);
	}
	return ret;
}

std::deque<uint32_t> createNumbers(uint32_t count) {
	std::deque<uint32_t> deque;
	//Fill the first
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < count; i++) {
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = (static_cast<uint32_t>(rand())+static_cast<uint32_t>(rand())) & mask;
		deque.push_back(key);
	}
	return deque;
}

std::deque<uint16_t> createNumbers16(uint32_t count) {
	std::deque<uint16_t> deque;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 15; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		deque.push_back(key);
	}
	return deque;
}


std::deque<uint8_t> createNumbers8(uint32_t count) {
	std::deque<uint8_t> deque;
	//Fill the first
	uint32_t rndNum;
	for(uint32_t i = 0; i < count; i++) {
		rndNum = rand();
		uint32_t key = rndNum & 0xFF;
		deque.push_back(key);
	}
	return deque;
}

std::set< std::string > createStringsSet(uint32_t maxStrLen, uint32_t strCount) {
	std::set<std::string> ret;
	while (ret.size() < strCount) {
		size_t strLen = (double)rand() / RAND_MAX * maxStrLen;
		std::string newStr;
		for(size_t j = 0; j < strLen; j++) {
			newStr += baseStringChars[(double)rand() / RAND_MAX * (baseStringChars.size()-1)];
		}
		ret.insert(newStr);
	}
	return ret;
}

std::set< uint32_t > createNumbersSet(uint32_t count) {
	std::set<uint32_t> ret;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	while( ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		ret.insert(key);
	}
	return ret;
}

std::set<uint32_t> createNumbersSet(uint32_t start, uint32_t end, uint32_t minDiff, uint32_t addDiff) {
	std::set<uint32_t> ret;
	uint32_t curKey = start;
	while (curKey <= end) {
		ret.insert(curKey);
		curKey += minDiff + (double)rand()/RAND_MAX * addDiff;
	}
	return ret;
}

std::set< uint16_t > createNumbers16Set(uint32_t count) {
	std::set<uint16_t> ret;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	while( ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 15; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		ret.insert(key);
	}
	return ret;
}

void createOverLappingSets(std::set<uint32_t> & a, std::set<uint32_t> & b, uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand) {
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(uint32_t i = 0; i < minEqual; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 23; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		a.insert(rndNum & mask);
		b.insert(rndNum & mask);
	}

	for(uint32_t i = 0; i < maxUnEqual; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 23; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		a.insert(rndNum & mask);
	}

	for(uint32_t i = 0; i < maxUnEqual; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 23; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		b.insert(rndNum & mask);
	}

	for(uint32_t i = 0; i < secondAddRand; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 23; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		b.insert(rndNum & mask);
	}
}

void createOverLappingSets(std::set<uint32_t> & a, std::set<uint32_t> & b, uint32_t maxValue, uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand) {
	uint32_t rndNum;
	for(uint32_t i = 0; i < minEqual; i++) {
		rndNum = ((double)rand())/RAND_MAX * maxValue;
		a.insert(rndNum);
		b.insert(rndNum);
	}

	for(uint32_t i = 0; i < maxUnEqual; i++) {
		rndNum = ((double)rand())/RAND_MAX * maxValue;
		a.insert(rndNum);
	}

	for(uint32_t i = 0; i < maxUnEqual; i++) {
		rndNum = ((double)rand())/RAND_MAX * maxValue;
		b.insert(rndNum);
	}

	for(uint32_t i = 0; i < secondAddRand; i++) {
		rndNum = ((double)rand())/RAND_MAX * maxValue;
		b.insert(rndNum);
	}
}

void createOverLappingSets(uint32_t count, uint32_t minEqual, uint32_t sizeVariance, std::deque< std::set<uint32_t> > & destination) {
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;

	std::set<uint32_t> equalPart;

	for(uint32_t i = 0; i < minEqual; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 23; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		equalPart.insert(rndNum & mask);
	}

	for(size_t i = 0; i < count; i++) {
		std::set<uint32_t> s = equalPart;
		uint16_t addCount = (double)rand()/RAND_MAX * sizeVariance;
		for(size_t j = 0; j < addCount; j++) {
			rndNum = rand();
			rndMask = (double)rand()/RAND_MAX * 23; 
			mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
			s.insert(rndNum & mask);
		}
		destination.push_back(s);
	}
}

void createHandSamplePolygons(SamplePolygonTestData & data) { 
	using namespace spatial;
	std::vector<spatial::GeoPoint> polyA(3);
	polyA[0] = GeoPoint(1,1); polyA[1] = GeoPoint(1.5,3.5);
	polyA[2] = GeoPoint(2.5,2.5);
	
	std::vector<spatial::GeoPoint> polyB(6);
	polyB[0] = GeoPoint(2,2.5); polyB[1] = GeoPoint(4,1);
	polyB[2] = GeoPoint(7,1.5); polyB[3] = GeoPoint(7,4.5);
	polyB[4] = GeoPoint(6,2); polyB[5] = GeoPoint(3,4);
	
	std::vector<spatial::GeoPoint> polyC(5);
	polyC[0] = GeoPoint(6,2.5); polyC[1] = GeoPoint(6,4.5);
	polyC[2] = GeoPoint(6.5,4.5); polyC[3] = GeoPoint(6.5,6.5);
	polyC[4] = GeoPoint(3.5,5);
	
	std::vector<spatial::GeoPoint> polyD(6);
	polyD[0] = GeoPoint(10,7); polyD[1] = GeoPoint(11,4);
	polyD[2] = GeoPoint(12,5); polyD[3] = GeoPoint(13,4.5);
	polyD[4] = GeoPoint(12.5,8); polyD[5] = GeoPoint(11,8.5);
	
	std::vector<spatial::GeoPoint> polyE(4);
	polyE[0] = GeoPoint(10.5,6.5); polyE[1] = GeoPoint(11.5,5);
	polyE[2] = GeoPoint(12,6); polyE[3] = GeoPoint(11.5,7);
	
	std::vector<spatial::GeoPoint> polyF(3);
	polyF[0] = GeoPoint(8.5, 2.5); polyF[1] = GeoPoint(10, 1);
	polyF[2] = GeoPoint(10, 2.5);
	
	
	std::vector<spatial::GeoPoint> polyG(7);
	polyG[0] = GeoPoint(6, 8.5); polyG[1] = GeoPoint(8,8.5);
	polyG[2] = GeoPoint(7, 9.5); polyG[3] = GeoPoint(8,10.5);
	polyG[4] = GeoPoint(6.5, 10.5); polyG[5] = GeoPoint(6.5,9);
	polyG[6] = GeoPoint(6,9);
	
	std::vector<spatial::GeoPoint> polyH(3);
	polyH[0] = GeoPoint(7.5, 9.5); polyH[1] = GeoPoint(8.5, 9);
	polyH[2] = GeoPoint(9.5, 10);
	
	std::vector<spatial::GeoPoint > polyI(5);
	polyI[0] = GeoPoint(8, 2); polyI[1] = GeoPoint(9, 2);
	polyI[2] = GeoPoint(9, 5.5); polyI[3] = GeoPoint(6.5, 7.5);
	polyI[4] = GeoPoint(4, 6.5);
	
	std::vector< spatial::GeoPoint > polyJ(5);
	polyJ[0] = GeoPoint(7.5, 5); polyJ[1] = GeoPoint(9.5, 5);
	polyJ[2] = GeoPoint(9.5, 6.5); polyJ[3] = GeoPoint(8, 7);
	polyJ[4] = GeoPoint(8, 5.5);

	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyA), POLY_A) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyB), POLY_B) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyC), POLY_C) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyD), POLY_D) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyE), POLY_E) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyF), POLY_F) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyG), POLY_G) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyH), POLY_H) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyI), POLY_I) );
	data.polys.push_back( std::pair<spatial::GeoPolygon, uint32_t>(spatial::GeoPolygon(polyJ), POLY_J) );

	
	std::vector< std::pair<uint32_t, uint32_t> > polyIntersects;
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_A, POLY_B) );
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_B, POLY_I) );
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_C, POLY_I) );
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_D, POLY_E) );
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_F, POLY_I) );
	polyIntersects.push_back( std::pair<uint32_t, uint32_t>(POLY_I, POLY_J) );
	
	for(size_t i = 0; i < polyIntersects.size(); ++i) {
		data.polyIntersects.insert(polyIntersects[i]);
		data.polyIntersects.insert(std::pair<uint32_t, uint32_t>(polyIntersects[i].second, polyIntersects[i].first) );
	}
	
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_A, POLY_B) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_B, POLY_A) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_B, POLY_I) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_C, POLY_I) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_E, POLY_D) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_F, POLY_I) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_I, POLY_F) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_I, POLY_J) );
	data.polyPointsInOtherPolys.insert( std::pair<uint32_t, uint32_t>(POLY_J, POLY_I) );
	
	data.points.resize(10);
	data.points[0] = GeoPoint(3.5, 2.5);
	data.points[1] = GeoPoint(4.5, 5);
	data.points[2] = GeoPoint(7, 6);
	data.points[3] = GeoPoint(7, 10);
	data.points[4] = GeoPoint(12, 7.5);
	data.points[5] = GeoPoint(11.5, 6);
	data.points[6] = GeoPoint(9, 2);
	data.points[7] = GeoPoint(7, 2.5);
	data.points[8] = GeoPoint(6, 2);
	data.points[9] = GeoPoint(4, 3.5);
	
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(0, POLY_B) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(1, POLY_C) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(2, POLY_I) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(3, POLY_G) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(4, POLY_D) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(5, POLY_D) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(5, POLY_E) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(6, POLY_F) );
// 	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(6, POLY_I) ); //TODO: fix points lying on the border
// 	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(7, POLY_B) );
	data.pointPolyIntersects.insert( std::pair<uint32_t, uint32_t>(8, POLY_B) );

	//RECTS
	data.rects.push_back( GeoRect(0.5, 3, 0.5, 9) );
	data.rectPolyIds.push_back( {POLY_A, POLY_B} ); data.rectPointIds.push_back( {} );

	data.rects.push_back( GeoRect(0.5, 6.1, 3.75, 8) );
	data.rectPolyIds.push_back( {POLY_B, POLY_C, POLY_I} ); data.rectPointIds.push_back( {1} );
	
	data.rects.push_back( GeoRect(5.5, 9, 8, 40) );
	data.rectPolyIds.push_back( {POLY_G, POLY_H} ); data.rectPointIds.push_back( {3} );

	data.rects.push_back( GeoRect(3, 9.75, -20, 100) );
	data.rectPolyIds.push_back( {POLY_B, POLY_C, POLY_F, POLY_G, POLY_H, POLY_I, POLY_J} ); data.rectPointIds.push_back( {0, 1, 2, 3, 6, 7, 8, 9} );
	
	data.rects.push_back( GeoRect(2, 3, 1, 1.4) );
	data.rectPolyIds.push_back( {} ); data.rectPointIds.push_back( {} );
	
	
}



}