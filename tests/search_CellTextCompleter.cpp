#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include "datacreationfuncs.h"

#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/Static/CellTextCompleter.h>
#include <sserialize/search/OOMSACTCCreator.h>

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

struct RegionArrangement {
	std::unordered_map<uint32_t, std::vector<uint32_t> > cellItems;
	std::vector<Item> items;
	std::vector<Item> regions;
	
	sserialize::Static::spatial::GeoHierarchy gh;

	Item createItem(uint32_t minStrs, uint32_t maxStrs, uint32_t minCells, uint32_t maxCells, uint32_t maxCellId) {
		uint32_t strCount = (rand() % (maxStrs-minStrs)) + minStrs;
		uint32_t cellCount = (rand() % (maxCells-minCells)) + minCells;
		
		std::set<std::string> strs = sserialize::createStringsSet(50, strCount);
		std::set<uint32_t> cells = createCellIds(cellCount, maxCellId);

		Item item;
		item.id = 0xFFFFFFFF;
		item.isRegion = false;
		item.strs.insert(item.strs.end(), strs.begin(), strs.end());
		item.cells.insert(item.cells.end(), cells.begin(), cells.end());
		return item;
	}


	void init(uint32_t maxBranching, uint32_t maxDepth, uint32_t maxRegionCells,
				uint32_t itemCount, uint32_t minStrs, uint32_t maxStrs, uint32_t minCells, uint32_t maxCells) {
		struct Region: sserialize::RefCountObject {
			std::vector<uint32_t> cells;
			std::vector< sserialize::RCPtrWrapper<Region> > children;
			template<typename TVisitor>
			void visitRec(TVisitor & v) {
				v(this);
				for(auto x : children) {
					x->visitRec(v);
				}
			}
			template<typename TVisitor>
			void visit(TVisitor v) {
				visitRec(v);
			}
		};
		
		struct RegionGraphInfo {
			sserialize::RCPtrWrapper<Region> rgp;
			uint32_t depth;
			RegionGraphInfo(const sserialize::RCPtrWrapper<Region> & rgp, uint32_t depth) : rgp(rgp), depth(depth) {}
		};
		
		sserialize::RCPtrWrapper<Region> regionGraph(new Region());
		
		std::vector<RegionGraphInfo> rgi;
		uint32_t cellId = 0;
		
		
		rgi.emplace_back(regionGraph, 0);
		while(rgi.size()) {
			RegionGraphInfo crg = std::move(rgi.back());
			rgi.pop_back();
			
			uint32_t numChildren = rand() % std::min(maxBranching, maxDepth-crg.depth);
			
			for(uint32_t i(0); i < numChildren; ++i) {
				crg.rgp->children.emplace_back(new Region());
				rgi.emplace_back(crg.rgp->children.back(), crg.depth+1);
			}
			uint32_t cellCount = (rand() % maxCells) + 1;
			for(uint32_t i(0); i < cellCount; ++i) {
				crg.rgp->cells.push_back(cellId);
				++cellId;
			}
		}
		
		regionGraph->visit([](Region * r) {
			std::set<uint32_t> tmp(r->cells.begin(), r->cells.end());
			r->visit([&tmp](Region * r) {
				tmp.insert(r->cells.begin(), r->cells.end());
			});
			r->cells.assign(tmp.begin(), tmp.end());
		});
		
		//create the region items
		uint32_t itemId = 0;
		regionGraph->visit([this, &itemId, maxStrs, minStrs](Region * r) {
			uint32_t strCount = (rand() % (maxStrs-minStrs)) + minStrs;
			std::set<std::string> strs = sserialize::createStringsSet(50, strCount);
			
			Item item;
			item.isRegion = true;
			item.id = itemId++;
			item.strs.assign(strs.begin(), strs.end());
			item.cells = std::move(r->cells);
			
			this->regions.emplace_back(std::move(item));
		});
		
		//create normal items
		for(uint32_t i(0); i < itemCount; ++i) {
			items.emplace_back(createItem(minStrs, maxStrs, minCells, maxCells, cellId-1));
			items.back().id = itemId++;
		}

		//init the cell->item lists
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
			pmi = sserialize::ItemIndex( std::move(pmiTmp) );
		}
		
		cqr = sserialize::CellQueryResult(fmi, pmi, pmil.begin(), sserialize::Static::spatial::GeoHierarchy(), sserialize::Static::ItemIndexStore());
	}
};

uint32_t maxBranching = 16;
uint32_t maxDepth = 8;
uint32_t maxRegionCells = 4;
uint32_t itemCount = 1000;
uint32_t minStrs = 2;
uint32_t maxStrs = 10;
uint32_t minCells = 1;
uint32_t maxCells = 10;


class CTCBaseTest: public CppUnit::TestFixture {
private:
	RegionArrangement m_ra;
protected:
	virtual const sserialize::Static::CellTextCompleter & sctc();
	const RegionArrangement & ra() const { return m_ra; }
	virtual void create();
public:
	CTCBaseTest() {
		m_ra.init(maxBranching, maxDepth, maxRegionCells, itemCount, minStrs, maxStrs, minCells, maxCells);	
	}
	
	void testExact() {
		
	}
	
	void testPrefix() {
		
	}
	
	void testSuffix() {
		
	}
	
	void testSubString() {
	
	}
};

class OOM_SA_CTC_Traits {
public:
	typedef Item item_type;
	struct ExactStrings {
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(const auto & x : item.strs) {
				*out = x;
				++out;
			}
		}
	};
	typedef ExactStrings SuffixStrings;
	struct ItemId {
		uint32_t operator()(const item_type & item) { return item.id; }
	};
	struct ItemCells {
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(const auto & x : item.cells) {
				*out = x;
				++out;
			}
		}
	};
public:
	ExactStrings exactStrings() { return ExactStrings(); }
	SuffixStrings suffixStrings() { return SuffixStrings(); }
	ItemId itemId() { return ItemId(); }
	ItemCells itemCells() { return ItemCells(); }
};


class OOMCTCTest: public CTCBaseTest {
CPPUNIT_TEST_SUITE( OOMCTCTest );
CPPUNIT_TEST( testExact );
CPPUNIT_TEST( testPrefix );
CPPUNIT_TEST( testSuffix );
CPPUNIT_TEST( testSubString );
CPPUNIT_TEST_SUITE_END();
private:
	sserialize::Static::CellTextCompleter m_ctc;
private:
	virtual const sserialize::Static::CellTextCompleter & sctc() override {
		return m_ctc;
	}
public:
	virtual void setUp() {
		sserialize::ItemIndexFactory idxFactory(true);
		sserialize::UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		
		sserialize::appendSACTC(ra().items.begin(), ra().items.end(), ra().regions.begin(), ra().regions.end(),
								OOM_SA_CTC_Traits(), OOM_SA_CTC_Traits(), 0xFFFFFFFF, idxFactory, dest);
	}
	virtual void tearDown() {}

	void testExact() {
		
	}
	void testPrefix() {
		
	}
	void testSuffix() {
		
	}
	void testSubString() {}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestTemplate::suite() );
	runner.run();
	return 0;
}