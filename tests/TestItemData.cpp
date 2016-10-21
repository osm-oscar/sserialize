#include "TestItemData.h"
#include <sserialize/strings/unicode_case_functions.h>
#include "utilalgos.h"
#include "datacreationfuncs.h"

namespace sserialize {

bool TestItemData::matchesOneString(std::string str, sserialize::StringCompleter::QuerryType type) const {
	return (match(strs, str, type, 1).size() > 0);
}

std::ostream& TestItemData::dump(std::ostream& out) {
	out << "[(";
	for(size_t i = 0; i < strs.size(); i++) {
		out <<  strs[i] << ", ";
	}
	out << "):" << id << "]" << std::endl;
	return out;
}


std::deque<TestItemData> createSampleData() {
	std::deque<TestItemData> data;
	TestItemData tdata;
	
	SamplePolygonTestData polys;
	createHandSamplePolygons(polys);
	std::deque<std::string> steinhaldenfeld;
	steinhaldenfeld.push_back("Deutschland");
	steinhaldenfeld.push_back("Baden-Württemberg");
	steinhaldenfeld.push_back("Stuttgart");
	steinhaldenfeld.push_back("Steinhaldenfeld");
	
	tdata.strs.insert(tdata.strs.end(), steinhaldenfeld.begin(), steinhaldenfeld.end());
	tdata.strs.push_back("Bäcker"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_A).points(); tdata.geoId = POLY_A;
	tdata.strs.push_back("Steinhaldenstraße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_B).points(); tdata.geoId = POLY_B;
	tdata.strs.push_back("Jakob-Böhme-Weg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_C).points(); tdata.geoId = POLY_C;
	tdata.strs.push_back("Damaschkestraße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_D).points(); tdata.geoId = POLY_D;
	tdata.strs.push_back("Igelhecke"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_E).points(); tdata.geoId = POLY_E;
	tdata.strs.push_back("Marderweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_F).points(); tdata.geoId = POLY_F;
	tdata.strs.push_back("Nebelgasse"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_G).points(); tdata.geoId = POLY_G;
	tdata.strs.push_back("Zahlmeister"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_H).points(); tdata.geoId = POLY_H;
	tdata.strs.push_back("U-Bahn");
	tdata.strs.push_back("4wohner"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_I).points(); tdata.geoId = POLY_I;
	tdata.strs.push_back("5eck"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_J).points(); tdata.geoId = POLY_J;
	std::string longString = "Arschlöchle in Hofen unten beim Neckar, das eigentlich Neckarblick heißt, aber doch von vielen Arschlöchle genannt wird. ";
	longString += "Öffungszeiten: 0-24 Uhr, von Montag bis Montag, nur am 29. Ferbruar. ";
	longString += "Speisekarte: Mappenspeck, Bayernkraut auf gebratenen Neckarenten. ";
	longString += "Getränkekarte: Neckarwasser, Klärwasser unud Bauarbeiterschweiß.";
	longString += "Dessertkarte: Eisenspähne vom Hornbachrind";
	tdata.strs.push_back(longString);
	data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_A).points(); tdata.geoId = POLY_A;
	data.push_back(tdata);

	std::deque<std::string> neugereut;
	neugereut.push_back("Deutschland");
	neugereut.push_back("Baden-Württemberg");
	neugereut.push_back("Stuttgart");
	neugereut.push_back("Neugereut");

	tdata.strs.clear();
	tdata.strs.insert(tdata.strs.end(), neugereut.begin(), neugereut.end());
	tdata.strs.push_back("Sailer"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_B).points(); tdata.geoId = POLY_B;
	tdata.strs.push_back("Jörg-Ratgeb-Schule"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_C).points(); tdata.geoId = POLY_C;
	tdata.strs.push_back("Edeka"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_D).points(); tdata.geoId = POLY_D;
	tdata.strs.push_back("Arschlöchle"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_E).points(); tdata.geoId = POLY_E;
	tdata.strs.push_back("Marabustraße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_F).points(); tdata.geoId = POLY_F;
	tdata.strs.push_back("Rohrdommelweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_G).points(); tdata.geoId = POLY_G;
	tdata.strs.push_back("Flamingoweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_H).points(); tdata.geoId = POLY_H;
	tdata.strs.push_back("Gansstraße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_I).points(); tdata.geoId = POLY_I;
	tdata.strs.push_back("U-Bahn"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_J).points(); tdata.geoId = POLY_J;
	tdata.strs.push_back("Vogelweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_A).points(); tdata.geoId = POLY_A;
	tdata.strs.push_back("Brummelweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_B).points(); tdata.geoId = POLY_B;
	tdata.strs.push_back("Christopherus-Schule"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_C).points(); tdata.geoId = POLY_C;
	tdata.strs.push_back("Dummbad"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_D).points(); tdata.geoId = POLY_D;
	tdata.strs.push_back("6heim"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_E).points(); tdata.geoId = POLY_E;
	tdata.strs.push_back("8weg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_F).points(); tdata.geoId = POLY_F;
	tdata.strs.push_back("9brücken"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_G).points(); tdata.geoId = POLY_G;
	tdata.strs.push_back("Marabu"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_H).points(); tdata.geoId = POLY_H;
	tdata.strs.push_back("M"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = polys.poly(POLY_I).points(); tdata.geoId = POLY_I;
// 	tdata.strs.push_back(""); data.push_back(tdata); tdata.strs.pop_back();//TODO: Define if the empty string is a valid suffix
	data.push_back(tdata);
	
	std::deque<std::string> blaustein;
	blaustein.push_back("Deutschland");
	blaustein.push_back("Baden-Württemberg");
	blaustein.push_back("Ulm");
	blaustein.push_back("Blaustein");
	
	tdata.strs.clear();
	tdata.strs.insert(tdata.strs.end(), blaustein.begin(), blaustein.end());
	tdata.strs.push_back("Bäcker"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(0)); tdata.geoId = 0;
	tdata.strs.push_back("U-Bahn"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(1)); tdata.geoId = 1;
	tdata.strs.push_back("Ostergasse"); data.push_back(tdata); tdata.strs.pop_back();  tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(2)); tdata.geoId = 2;
	tdata.strs.push_back("Paul-Hauser-Straße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(3)); tdata.geoId = 3;
	tdata.strs.push_back("Eckstein"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(4)); tdata.geoId = 4;
	tdata.strs.push_back("Hausachtal"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(5)); tdata.geoId = 5;
	tdata.strs.push_back("Jodplatz"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points.clear();
	tdata.strs.push_back("Karlsplatz"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(7)); tdata.geoId = 7;
	tdata.strs.push_back("1live"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(8)); tdata.geoId = 8;
	tdata.strs.push_back("2raum"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(9)); tdata.geoId = 9;
	tdata.strs.push_back("3taler"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(0)); tdata.geoId = 0;
	tdata.strs.push_back("Ützel brützel"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(0)); tdata.geoId = 0;
	data.push_back(tdata);
	
	std::deque<std::string> erding;
	erding.push_back("Deutschland");
	erding.push_back("Bayern");
	erding.push_back("München");
	erding.push_back("Erding");
	
	tdata.strs.clear();
	tdata.strs.insert(tdata.strs.end(), erding.begin(), erding.end());
	tdata.strs.push_back("Bäcker"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(1)); tdata.geoId = 1;
	tdata.strs.push_back("U-Bahn"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(2)); tdata.geoId = 2;
	tdata.strs.push_back("Galaxy"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(3)); tdata.geoId = 3;
	tdata.strs.push_back("Wurzstraße"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(4)); tdata.geoId = 4;
	tdata.strs.push_back("Quaddelweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(5)); tdata.geoId = 5;
	tdata.strs.push_back("Ludwigsplatz"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(6)); tdata.geoId = 6;
	tdata.strs.push_back("Max-von-Fritz-Weg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(7)); tdata.geoId = 7;
	tdata.strs.push_back("Opern-Haus"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points.clear();
	tdata.strs.push_back("Radeweg"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(9)); tdata.geoId = 9;
	tdata.strs.push_back("Tennisplatz"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(0)); tdata.geoId = 0;
	tdata.strs.push_back("7up"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(1)); tdata.geoId = 1;
	tdata.strs.push_back("öbatzda"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(2)); tdata.geoId = 2;
	tdata.strs.push_back("ärsche"); data.push_back(tdata); tdata.strs.pop_back(); tdata.points = std::vector<sserialize::spatial::GeoPoint>(); tdata.points.push_back(polys.point(3)); tdata.geoId = 3;
	data.push_back(tdata);
	
	//assign item ids
	for(size_t i = 0; i < data.size(); i++) {
		data[i].id = (uint32_t) i;
	}
	return data;
}

std::deque< std::string > createSampleSingleCompletionStrings() {
	std::deque<std::string> singleCompStrs;
// 	singleCompStrs.push_back("");
	singleCompStrs.push_back("m");
	singleCompStrs.push_back("eisensp");
	singleCompStrs.push_back("dessert");
	singleCompStrs.push_back("Arsch");
	singleCompStrs.push_back("marabu");
	singleCompStrs.push_back("ae");
	singleCompStrs.push_back("ä");
	singleCompStrs.push_back("aße");
	singleCompStrs.push_back("AßE");
	singleCompStrs.push_back("Baden-Württemberg");
	singleCompStrs.push_back("Bä");
	singleCompStrs.push_back("Bäcker");
	singleCompStrs.push_back("bÄcKeR");
	singleCompStrs.push_back("jakob");
	singleCompStrs.push_back("böhme");
	singleCompStrs.push_back("löch");
	singleCompStrs.push_back("Deutschland");
	singleCompStrs.push_back("geb");
	singleCompStrs.push_back("Quaddelweg");
	singleCompStrs.push_back("Stuttgart");
	singleCompStrs.push_back("Stutt");
	singleCompStrs.push_back("tuTTgArT");
	singleCompStrs.push_back("Steinhaldenfeld");
	singleCompStrs.push_back("St");
	singleCompStrs.push_back("U-Ba");
	singleCompStrs.push_back("U-Bahn");
	singleCompStrs.push_back("ü");
	singleCompStrs.push_back("z");
	return singleCompStrs;
}


std::deque< std::deque< std::string > > createSampleCompletionStrings() {
	std::deque< std::deque<std::string> > strs;
	std::deque<std::string> comp;

	std::deque<std::string> singleCompStrs(createSampleSingleCompletionStrings());
	for(size_t i = 0; i < singleCompStrs.size();i++) {
		std::deque<std::string> oneCompletion;
		oneCompletion.push_back(singleCompStrs.at(i));
		strs.push_back(oneCompletion);
	}

	for(size_t i = 0; i < singleCompStrs.size(); i++) {
		for(size_t j = 0; j < singleCompStrs.size(); j++) { 
			std::deque<std::string> oneCompletion;
			oneCompletion.push_back(singleCompStrs.at(i));
			oneCompletion.push_back(singleCompStrs.at(j));
			strs.push_back(oneCompletion);
		}
	}
	return strs;
}

std::deque<TestItemData> getElementsWithString(const std::string & str, sserialize::StringCompleter::QuerryType matchType, const std::deque<TestItemData> & items) {
	std::deque<TestItemData> res;
	for(size_t i = 0; i < items.size(); i++) {
		if (items.at(i).matchesOneString(str, matchType)) {
			res.push_back(items.at(i));
		}
	}
	return res;
}


std::set<unsigned int> getItemIdsWithString(const std::string & str, sserialize::StringCompleter::QuerryType matchType, const std::deque<TestItemData> & items) {
	std::set<unsigned int> res;
	for(size_t i = 0; i < items.size(); i++) {
		if (items.at(i).matchesOneString(str, matchType)) {
			res.insert(items.at(i).id);
		}
	}
// 	std::cout << "Set with " << res.size() << " elements created. comparestr=" << str << std::endl;
	return res;
}

std::set<unsigned int>
getItemIdsWithString(const std::deque< std::string >& strs, sserialize::StringCompleter::QuerryType matchType, const std::deque< sserialize::TestItemData >& items) {
	std::deque< std::set<unsigned int> > sets;
	for(size_t i = 0; i < strs.size(); i++) {
		sets.push_back(getItemIdsWithString(strs.at(i), matchType, items));
	}
	return intersectSets(sets);
}

std::set< unsigned int > getItemIds(const std::deque< TestItemData >& items, ItemIndex idx) {
	std::set<unsigned int> ret;
	for(uint32_t i = 0; i < idx.size(); i++) {
		if (idx.at(i) < items.size()) {
			ret.insert( items[idx.at(i)].id );
		}
	}
	return ret;
}


}//end namespace
