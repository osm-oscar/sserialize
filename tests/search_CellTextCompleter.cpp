#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestResult.h>
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
		return *this;
	}
	
	Item & operator=(Item && other) {
		id = other.id;
		isRegion = other.isRegion;
		strs = std::move(other.strs);
		cells = std::move(other.cells);
		return *this;
	}
	
	bool matches(const std::string & qstr, sserialize::StringCompleter::QuerryType qt) const {
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

class RegionArrangement {
private:
	struct Region: sserialize::RefCountObject {
		std::vector<uint32_t> cells;
		std::vector< sserialize::RCPtrWrapper<Region> > children;
		std::vector< sserialize::RCPtrWrapper<Region> > parents;
		uint32_t ghId;
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
public:
	typedef enum {IT_NONE=0x0, IT_ITEM=0x1, IT_REGION=0x2, IT_ALL=0x3} ItemTypes;
public:
	std::unordered_map<uint32_t, std::vector<uint32_t> > cellItems;
	std::vector<Item> items;
	std::vector<Item> regions;
	
	sserialize::Static::ItemIndexStore ghIdxStore;
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


	void init(uint32_t targetCellCount, uint32_t maxBranching, uint32_t maxDepth, uint32_t maxRegionCells,
				uint32_t itemCount, uint32_t minStrs, uint32_t maxStrs, uint32_t minCells, uint32_t maxCells) {
		
		struct RegionGraphInfo {
			sserialize::RCPtrWrapper<Region> rgp;
			uint32_t depth;
			RegionGraphInfo(const sserialize::RCPtrWrapper<Region> & rgp, uint32_t depth) : rgp(rgp), depth(depth) {}
		};
		
		sserialize::RCPtrWrapper<Region> regionGraph(new Region());
		
		uint32_t cellId = 0;
		
		std::vector<RegionGraphInfo> rgi;
		rgi.emplace_back(regionGraph, 0);
		
		while (cellId < targetCellCount) {
			std::vector<RegionGraphInfo> rgQueue;
			rgQueue.emplace_back(rgi.at(rand() % rgi.size()));
			while(rgQueue.size()) {
				RegionGraphInfo crg = std::move(rgQueue.back());
				rgQueue.pop_back();
				rgi.emplace_back(crg);
				
				uint32_t numChildren = rand() % std::min(maxBranching, maxDepth-crg.depth);
				
				for(uint32_t i(0); i < numChildren; ++i) {
					crg.rgp->children.emplace_back(new Region());
					crg.rgp->children.back()->parents.push_back(crg.rgp);
					rgQueue.emplace_back(crg.rgp->children.back(), crg.depth+1);
				}
				uint32_t cellCount = (rand() % maxRegionCells) + 1;
				for(uint32_t i(0); i < cellCount; ++i) {
					crg.rgp->cells.push_back(cellId);
					++cellId;
				}
			}
		}
		
		if (cellId < 10) {
			std::cout << "BAM";
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
			std::sort(x.second.begin(), x.second.end());
		}
		
		//create the gh
		sserialize::spatial::GeoHierarchy gh;
		sserialize::spatial::detail::geohierarchy::CellList & cellList = gh.cells();
		sserialize::spatial::detail::geohierarchy::RegionList regionList = gh.regions();
		
		{//assign gh ids to regions, create the cellList
			std::unordered_map<uint32_t, std::vector<uint32_t> > cellParents;
			
			std::vector< sserialize::RCPtrWrapper<Region> > rgi;
			rgi.push_back(regionGraph);
			for(uint32_t i(0); i < rgi.size(); ++i) {
				auto cr = rgi.at(i);
				cr->ghId = i;
				
				//create cell parents
				for(uint32_t x : cr->cells) {
					cellParents[x].push_back(i);
				}
				
				rgi.insert(rgi.end(), cr->children.begin(), cr->children.end());
			}
			
			//we can now create the cellList
			uint32_t cellParentListSize(0), cellItemListSize(0);
			for(auto x : cellParents) {
				cellParentListSize += x.second.size();
			}
			
			for(auto x : cellItems) {
				cellItemListSize += x.second.size();
			}
			
			assert(cellParents.size() == cellId);
			
			cellList.cellRegionLists().resize(cellParentListSize);
			cellList.cellItemList().resize(cellItemListSize);
			
			uint32_t * cellRegionListIt = cellList.cellRegionLists().begin();
			uint32_t * cellItemListIt = cellList.cellItemList().begin();
			
			for(uint32_t cellId(0), s(cellParents.size()); cellId < s; ++cellId) {
				uint32_t * myCellRegionListBegin = cellRegionListIt;
				uint32_t * myCellItemListBegin = cellItemListIt;
				
				if (cellItems.count(cellId)) {
					cellItemListIt = std::copy(cellItems[cellId].cbegin(), cellItems[cellId].cend(), cellItemListIt);
				}
				assert(cellParents.count(cellId));
				cellRegionListIt = std::copy(cellParents[cellId].cbegin(), cellParents[cellId].cend(), cellRegionListIt);
				cellList.cells().emplace_back(myCellRegionListBegin, cellRegionListIt, myCellItemListBegin, cellItemListIt, sserialize::spatial::GeoRect());
			}
			
			//now take care of the region list
			for(const sserialize::RCPtrWrapper<Region> & rp : rgi) {
				assert(rp->ghId == regionList.regionDescriptions().size());
				uint64_t off = regionList.regionData().size();
				
				for(auto x : rp->children) {
					regionList.regionData().push_back(x->ghId);
				}
				for(auto x : rp->parents) {
					regionList.regionData().push_back(x->ghId);
				}
				regionList.regionData().push_back(rp->cells.begin(), rp->cells.end());
				
				regionList.regionDescriptions().emplace_back(&(regionList.regionData()), off, rp->children.size(), rp->parents.size(), rp->cells.size(), 0);
			}
			
			gh.createRootRegion();
			
			sserialize::UByteArrayAdapter sghD(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
			sserialize::ItemIndexFactory idxFactory(true);
			
			gh.append(sghD, idxFactory, false);
			
			idxFactory.flush();
			
			this->gh = sserialize::Static::spatial::GeoHierarchy(sghD);
		}
		
		//now kill the regionGraph, remove the cycles created by the parent-pointers
		regionGraph->visit([](Region * r) { r->parents.clear(); });
	}
	
	void find(const std::string & qstr, sserialize::StringCompleter::QuerryType qt, ItemTypes itemTypes,
	sserialize::CellQueryResult & cqr, std::vector<sserialize::ItemIndex> & pml) const
	{
		std::unordered_map<uint32_t, std::vector<uint32_t> > pm;
		std::unordered_set<uint32_t> fm;
		
		if (itemTypes & IT_ITEM) {
			for(const auto & x : items) {
				if (x.matches(qstr, qt)) {
					for(auto c : x.cells) {
						pm[c].push_back(x.id);
					}
				}
			}
		}
		
		if (itemTypes & IT_REGION) {
			for(const auto & x : regions) {
				if (x.matches(qstr, qt)) {
					fm.insert(x.cells.begin(), x.cells.end());
				}
			}
		}
		
		std::vector<uint32_t> fmTmp(fm.begin(), fm.end());
		std::sort(fmTmp.begin(), fmTmp.end());
		
		sserialize::ItemIndex fmi(std::move(fmTmp));
		sserialize::ItemIndex pmi;
		std::vector<uint32_t> pmil;
		{
			std::map<uint32_t, uint32_t> pmc2pml;
			for(auto & x : pm) {
				pmc2pml[x.first] = pml.size();
				pml.emplace_back(std::move(x.second));
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

uint32_t targetCellCount = 100;
uint32_t maxBranching = 16;
uint32_t maxDepth = 8;
uint32_t maxRegionCells = 4;
uint32_t itemCount = 1000;
uint32_t minStrs = 2;
uint32_t maxStrs = 10;
uint32_t minCells = 1;
uint32_t maxCells = 10;
sserialize::StringCompleter::SupportedQuerries supportedQuerries = sserialize::StringCompleter::SQ_EXACT;

class CTCBaseTest: public CppUnit::TestFixture {
// CPPUNIT_TEST_SUITE( CTCBaseTest );
// CPPUNIT_TEST_SUITE_END();
private:
	RegionArrangement m_ra;
protected:
	virtual const sserialize::Static::CellTextCompleter & sctc() = 0;
	const RegionArrangement & ra() const { return m_ra; }
	virtual void create() {}
public:

	void testCompletion(sserialize::StringCompleter::QuerryType qt, RegionArrangement::ItemTypes it) {
		std::unordered_set<std::string> baseTestStrings; 
		if (it & RegionArrangement::IT_ITEM) {
			for(const Item & item : ra().items) {
				baseTestStrings.insert(item.strs.begin(), item.strs.end());
			}
		}
		if (it & RegionArrangement::IT_REGION) {
			for(const Item & item : ra().regions) {
				baseTestStrings.insert(item.strs.begin(), item.strs.end());
			}
		}
		std::unordered_set<std::string> testStrings;
		if (qt == sserialize::StringCompleter::QT_EXACT) {
			testStrings = std::move(baseTestStrings);
		}
		else if (qt == sserialize::StringCompleter::QT_PREFIX) {
			for(const std::string & str : baseTestStrings) {
				for(std::string::const_iterator it(str.end()), begin(str.begin()); it != begin; utf8::prior(it, begin)) {
					testStrings.emplace(begin, it);
				}
			}
		}
		else if (qt == sserialize::StringCompleter::QT_SUFFIX) {
			for(const std::string & str : baseTestStrings) {
				for(std::string::const_iterator it(str.begin()), end(str.end()); it != end; utf8::next(it, end)) {
					testStrings.emplace(it, end);
				}
			}
		}
		else if (qt == sserialize::StringCompleter::QT_SUBSTRING) {
			for(const std::string & str : baseTestStrings) {
				for(std::string::const_iterator it(str.begin()), end(str.end()); it != end; utf8::next(it, end)) {
					testStrings.emplace(it, end);
				}
			}
			baseTestStrings = std::move(testStrings);
			for(const std::string & str : baseTestStrings) {
				for(std::string::const_iterator it(str.end()), begin(str.begin()); it != begin; utf8::prior(it, begin)) {
					testStrings.emplace(begin, it);
				}
			}
		}
		//now do the test
		for(const std::string & qstr : testStrings) {
			sserialize::CellQueryResult testCqr, realCqr;
			std::vector<sserialize::ItemIndex> realPm;
			std::string baseMessage = "qstr=" + qstr + ",type=";
			switch(it) {
			case RegionArrangement::IT_ITEM:
				testCqr = sctc().items<sserialize::CellQueryResult>(qstr, qt);
				ra().find(qstr, qt, it, realCqr, realPm);
				baseMessage += "items";
				break;
			case RegionArrangement::IT_REGION:
				testCqr = sctc().regions<sserialize::CellQueryResult>(qstr, qt);
				ra().find(qstr, qt, it, realCqr, realPm);
				baseMessage += "regions";
				break;
			case RegionArrangement::IT_ALL:
				testCqr = sctc().complete<sserialize::CellQueryResult>(qstr, qt);
				ra().find(qstr, qt, it, realCqr, realPm);
				baseMessage += "all";
				break;
			default:
				std::runtime_error("Invalid type");
			};
			baseMessage += ",";
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Cqr size", realCqr.cellCount(), testCqr.cellCount());
			for(uint32_t i(0), s(realCqr.cellCount()); i < s; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(baseMessage+"cellid at " + std::to_string(i), realCqr.cellId(i), testCqr.cellId(i));
				CPPUNIT_ASSERT_EQUAL_MESSAGE(baseMessage+"fullMatch at " + std::to_string(i), realCqr.fullMatch(i), testCqr.fullMatch(i));
				if (!realCqr.fullMatch(i)) {
					CPPUNIT_ASSERT_EQUAL_MESSAGE(baseMessage+"pm index at " + std::to_string(i), realPm.at(realCqr.idxId(i)), testCqr.idx(i));
				}
			}
		}
	}

public:
	CTCBaseTest() {
		m_ra.init(targetCellCount, maxBranching, maxDepth, maxRegionCells, itemCount, minStrs, maxStrs, minCells, maxCells);
	}
	virtual ~CTCBaseTest() {}
	
	void testExactItem() {
		testCompletion(sserialize::StringCompleter::QT_EXACT, RegionArrangement::IT_ITEM);
	}
	
	void testExactRegion() {
		testCompletion(sserialize::StringCompleter::QT_EXACT, RegionArrangement::IT_REGION);
	}
	
	void testExactAll() {
		testCompletion(sserialize::StringCompleter::QT_EXACT, RegionArrangement::IT_ALL);
	}
	
	void testPrefixItem() {
		testCompletion(sserialize::StringCompleter::QT_PREFIX, RegionArrangement::IT_ITEM);
	}
	
	void testPrefixRegion() {
		testCompletion(sserialize::StringCompleter::QT_PREFIX, RegionArrangement::IT_REGION);
	}
	
	void testPrefixAll() {
		testCompletion(sserialize::StringCompleter::QT_PREFIX, RegionArrangement::IT_ALL);
	}
	
	void testSuffixItem() {
		testCompletion(sserialize::StringCompleter::QT_SUFFIX, RegionArrangement::IT_ITEM);
	}
	
	void testSuffixRegion() {
		testCompletion(sserialize::StringCompleter::QT_SUFFIX, RegionArrangement::IT_REGION);
	}
	
	void testSuffixAll() {
		testCompletion(sserialize::StringCompleter::QT_SUFFIX, RegionArrangement::IT_ALL);
	}
	
	void testSubStringItem() {
		testCompletion(sserialize::StringCompleter::QT_SUBSTRING, RegionArrangement::IT_ITEM);
	}
	
	void testSubStringRegion() {
		testCompletion(sserialize::StringCompleter::QT_SUBSTRING, RegionArrangement::IT_REGION);
	}
	
	void testSubStringAll() {
		testCompletion(sserialize::StringCompleter::QT_SUBSTRING, RegionArrangement::IT_ALL);
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
CPPUNIT_TEST( testExactItem );
// CPPUNIT_TEST( testExactRegion );
// CPPUNIT_TEST( testExactAll );
// 
// CPPUNIT_TEST( testPrefixItem );
// CPPUNIT_TEST( testPrefixRegion );
// CPPUNIT_TEST( testPrefixAll );
// 
// CPPUNIT_TEST( testSuffixItem );
// CPPUNIT_TEST( testSuffixRegion );
// CPPUNIT_TEST( testSuffixAll );
// 
// CPPUNIT_TEST( testSubStringItem );
// CPPUNIT_TEST( testSubStringRegion );
// CPPUNIT_TEST( testSubStringAll );

CPPUNIT_TEST_SUITE_END();
private:
	sserialize::Static::CellTextCompleter m_ctc;
private:
	virtual const sserialize::Static::CellTextCompleter & sctc() override {
		return m_ctc;
	}
public:
	OOMCTCTest() : CTCBaseTest() {}
	virtual ~OOMCTCTest() {}
	virtual void setUp() {
		sserialize::ItemIndexFactory idxFactory(true);
		sserialize::UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		
		sserialize::appendSACTC(ra().items.begin(), ra().items.end(), ra().regions.begin(), ra().regions.end(),
								OOM_SA_CTC_Traits(), OOM_SA_CTC_Traits(), 0xFFFFFFFF, supportedQuerries, idxFactory, dest);
								
		idxFactory.flush();
		sserialize::Static::ItemIndexStore idxStore(idxFactory.getFlushedData());
// 		m_ctc = sserialize::Static::CellTextCompleter(dest, idxStore, ra().gh);
		
	}
	virtual void tearDown() {}
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.eventManager().popProtector();
// 	OOMCTCTest test;
// 	runner.addTest( CTCBaseTest::suite() );
	runner.addTest(  OOMCTCTest::suite() );
	runner.run();
	return 0;
}