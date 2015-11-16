#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include "datacreationfuncs.h"

#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/Static/CellTextCompleter.h>

struct Item {
	uint32_t id;
	bool isRegion;
	std::vector<std::string> strs;
	std::vector<uint32_t> cells;
	Item() : id(std::numeric_limits<uint32_t>::max()), isRegion(0) {}
	Item(Item && other) : id(other.id), isRegion(other.isRegion), strs(std::move(other.strs)), cells(std::move(other.cells)) {}
	Item(const Item & other) : id(other.id), isRegion(other.isRegion), strs(other.strs), cells(other.cells) {}
	
	Item & operator=(const Item & other) {
		id = other.id;
		isRegion = other.isRegion;
		strs = other.strs;
		cells = other.cells;
	}
	
	Item & operator=(Item && other) {
		id = other.id;
		isRegion = other.isRegion;
		strs = std::move(other.strs);
		cells = std::move(other.cells);
	}
	
	bool matches(const std::string & qstr, sserialize::StringCompleter::QuerryType qt) {
		for(const auto & x : strs) {
			if (sserialize::StringCompleter::matches(x, qstr, qt)) {
				return true;
			}
		}
		return false;
	}
};


std::set<uint32_t> createCellIds(uint32_t count, uint32_t maxCellId) {
	assert(count < maxCellId);
	std::set<uint32_t> res;
	while (res.size() < count) {
		res.insert(rand() % (maxCellId+1));
	}
	return res;
}

template<typename TOutPutIterator>
void createItem(uint32_t count, uint32_t minStrs, uint32_t maxStrs, uint32_t minCells, uint32_t maxCells, uint32_t maxCellId, TOutPutIterator out) {
	for(uint32_t i(0); i < count; ++i) {
		uint32_t strCount = (rand() % (maxStrs-minStrs)) + minStrs;
		uint32_t cellCount = (rand() % (maxCells-minCells)) + minCells;
		
		std::set<std::string> strs = sserialize::createStringsSet(50, strCount);
		std::set<uint32_t> cells = createCellIds(cellCount, maxCellId);

		Item item;
		item.id = i;
		item.isRegion = false;
		item.strs.insert(item.strs.end(), strs.begin(), strs.end());
		item.cells.insert(item.cells.end(), cells.begin(), cells.end());
		
		*out = std::move(item);
		++out;
	}
}

sserialize::Static::spatial::GeoHierarchy createGH(uint32_t regionCount, uint32_t maxBranching, uint32_t maxDepth) {
	struct Region {
		std::vector<uint32_t> cells;
	};
}


struct RegionArrangement {
	std::unordered_map<uint32_t, std::vector<uint32_t> > cellItems;
	std::vector<Item> items;
	std::vector<Item> regions;
	
	sserialize::Static::spatial::GeoHierarchy gh;
	
	void moveInit(std::vector<Item>::iterator begin, std::vector<Item>::iterator end) {
		for(; begin != end; ++begin) {
			if (begin->isRegion) {
				regions.push_back(std::move(*begin));
			}
			else {
				items.push_back(std::move(*begin));
			}
		}
		
		for(const auto & x : items) {
			for(auto c : x.cells) {
				cellItems[c].push_back(x.id);
			}
		}
		for(auto & x : cellItems) {
			std::sort(x.second);
		}
	}
	
	void find(const std::string & qstr, sserialize::StringCompleter::QuerryType qt, sserialize::CellQueryResult & cqr, std::vector<sserialize::ItemIndex> & pml) {
		std::unordered_map<uint32_t, std::vector<uint32_t> > pm;
		std::unordered_set<uint32_t> fm;
		
		for(const auto & x : items) {
			if (x.matches(qstr, qt)) {
				for(auto c : x.cells) {
					pm[c].push_back(x.id);
				}
			}
		}
		
		for(const auto & x : items) {
			if (x.matches(qstr, qt)) {
				for(auto c : x.cells) {
					pm[c].push_back(x.id);
				}
			}
		}
		
		sserialize::ItemIndex fmi(std::move(fm));
		sserialize::ItemIndex pmi;
		std::vector<uint32_t> pmil;
		{
			std::map<uint32_t, uint32_t> pmc2pml;
			for(auto & x : pm) {
				pmc2pml[x.first] = pml.size();
				pml.push_back(std::move(x.second));
			}
			
			
			std::vector<uint32_t> pmiTmp;
			for(const auto & x : pmc2pml) {
				pmiTmp.push_back(x.first);
				pmil.push_back(x.second);
			}
			pmi.absorb(pmiTmp);
		}
		
		cqr = sserialize::CellQueryResult(fmi, pmi, pmil.begin(), sserialize::Static::spatial::GeoHierarchy(), sserialize::Static::ItemIndexStore());
	}
};

class TestTemplate: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestTemplate );
CPPUNIT_TEST( test );
CPPUNIT_TEST_SUITE_END();
private:
	virtual const sserialize::Static::CellTextCompleter & sctc();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void test() {}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestTemplate::suite() );
	runner.run();
	return 0;
}