#include "datacreationfuncs.h"
#include "TestBase.h"

#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/Static/CellTextCompleter.h>
#include <sserialize/search/OOMSACTCCreator.h>

//config stuff

using trie_nodeid_type = uint32_t;
using itemid_type = uint32_t;
using cellid_type = uint32_t;
using indexid_type = uint32_t;
using regionid_type = uint32_t;

cellid_type targetCellCount = 100;
uint32_t maxBranching = 16;
uint32_t maxDepth = 8;
cellid_type maxRegionCells = 4;
indexid_type itemCount = 1000;
trie_nodeid_type minStrs = 2;
trie_nodeid_type maxStrs = 10;
cellid_type minCells = 1;
cellid_type maxCells = 10;
// sserialize::StringCompleter::SupportedQuerries supportedQuerries = sserialize::StringCompleter::SQ_EXACT;
// sserialize::StringCompleter::SupportedQuerries supportedQuerries = sserialize::StringCompleter::SQ_PREFIX;
// sserialize::StringCompleter::SupportedQuerries supportedQuerries = sserialize::StringCompleter::SQ_SUFFIX;
sserialize::StringCompleter::SupportedQuerries supportedQuerries = sserialize::StringCompleter::SQ_EPSP;


struct Item {
	itemid_type id;
	bool isRegion;
	std::vector<std::string> strs;
	std::vector<cellid_type> cells;
	Item() : id(std::numeric_limits<itemid_type>::max()), isRegion(0) {}
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


std::set<cellid_type> createCellIds(cellid_type count, cellid_type maxCellId) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(count, maxCellId);
	std::set<cellid_type> res;
	while (res.size() < count) {
		res.insert(rand() % (maxCellId+1));
	}
	return res;
}

class RegionArrangement {
private:
	struct Region: sserialize::RefCountObject {
		std::vector<cellid_type> cells;
		std::vector< sserialize::RCPtrWrapper<Region> > children;
		std::vector< sserialize::RCPtrWrapper<Region> > parents;
		regionid_type ghId;
		itemid_type storeId;
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
	sserialize::CellQueryResult::CellInfo ci;

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
			item.cells = r->cells;
			
			r->storeId = (uint32_t) this->regions.size();
			
			this->regions.emplace_back(std::move(item));
		});
		
		//create normal items
		for(uint32_t i(0); i < itemCount; ++i) {
			items.emplace_back(createItem(minStrs, maxStrs, minCells, maxCells, cellId-1));
			items.back().id = itemId++;
		}
		
		//for debbugging
		items.front().strs.push_back("debugstr!");

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
		sserialize::spatial::detail::geohierarchy::RegionList & regionList = gh.regions();
		
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
			
			SSERIALIZE_CHEAP_ASSERT_EQUAL(cellParents.size(), cellId);
			
			cellList.cellRegionLists().resize(cellParentListSize);
			cellList.cellItemList().resize(cellItemListSize);
			
			uint32_t * cellRegionListIt = cellList.cellRegionLists().begin();
			uint32_t * cellItemListIt = cellList.cellItemList().begin();
			
			for(uint32_t cellId(0), s((uint32_t) cellParents.size()); cellId < s; ++cellId) {
				uint32_t * myCellRegionListBegin = cellRegionListIt;
				uint32_t * myCellItemListBegin = cellItemListIt;
				
				if (cellItems.count(cellId)) {
					cellItemListIt = std::copy(cellItems[cellId].cbegin(), cellItems[cellId].cend(), cellItemListIt);
				}
				SSERIALIZE_CHEAP_ASSERT(cellParents.count(cellId));
				cellRegionListIt = std::copy(cellParents[cellId].cbegin(), cellParents[cellId].cend(), cellRegionListIt);
				cellList.cells().emplace_back(myCellRegionListBegin, cellRegionListIt, myCellItemListBegin, cellItemListIt, sserialize::spatial::GeoRect());
			}
			
			//now take care of the region list
			for(const sserialize::RCPtrWrapper<Region> & rp : rgi) {
				SSERIALIZE_CHEAP_ASSERT_EQUAL(rp->ghId, regionList.regionDescriptions().size());
				uint64_t off = regionList.regionData().size();
				
				for(auto x : rp->children) {
					regionList.regionData().push_back(x->ghId);
				}
				for(auto x : rp->parents) {
					regionList.regionData().push_back(x->ghId);
				}
				regionList.regionData().push_back(rp->cells.begin(), rp->cells.end());
				
				regionList.regionDescriptions().emplace_back(&(regionList.regionData()), off, (uint32_t) rp->children.size(), (uint32_t) rp->parents.size(), (uint32_t) rp->cells.size(), 0);
			}
			
			gh.createRootRegion();
			
			sserialize::UByteArrayAdapter sghD(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
			sserialize::ItemIndexFactory idxFactory(true);
			
			gh.append(sghD, idxFactory, false);
			
			idxFactory.flush();
			
			this->gh = sserialize::Static::spatial::GeoHierarchy(sghD);
			
			SSERIALIZE_CHEAP_ASSERT_EQUAL(this->gh.cellSize(), cellId);
			
			this->ci = sserialize::Static::spatial::GeoHierarchyCellInfo::makeRc(this->gh);
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
				pmc2pml[x.first] = (uint32_t) pml.size();
				pml.emplace_back(std::move(x.second));
			}
			
			std::vector<uint32_t> pmiTmp;
			for(const auto & x : pmc2pml) {
				pmiTmp.push_back(x.first);
				pmil.push_back(x.second);
			}
			pmi = sserialize::ItemIndex( std::move(pmiTmp) );
		}
		
		cqr = sserialize::CellQueryResult(fmi, pmi, pmil.begin(), ci, sserialize::Static::ItemIndexStore(), sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS);
	}
};

class CTCBaseTest: public sserialize::tests::TestBase {
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
		if ((qt & supportedQuerries) == sserialize::StringCompleter::QT_NONE) {
			return;
		}
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
		std::vector<std::string> myTestStrings(testStrings.begin(), testStrings.end());
		std::sort(myTestStrings.begin(), myTestStrings.end());
		
		//now do the test
		for(uint32_t i(0), s((uint32_t) myTestStrings.size()); i < s; ++i) {
			const std::string & qstr  = myTestStrings[i];
			sserialize::CellQueryResult testCqr, realCqr;
			std::vector<sserialize::ItemIndex> realPm;
			std::string baseMessage = "strs[" + std::to_string(i) + "]=" + qstr + ",type=";
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
			CPPUNIT_ASSERT_EQUAL_MESSAGE(baseMessage+"cqr.cellCount()", realCqr.cellCount(), testCqr.cellCount());
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
				if (x == "debugstr!") {
					std::cout << "Inserting debug string" << std::endl;
				}
			}
		}
	};
	typedef ExactStrings SuffixStrings;
	struct ItemId {
		using id_type = uint32_t;
		static constexpr bool HasCellLocalIds = false;
		id_type operator()(const item_type & item) { return item.id; }
		id_type operator()(const item_type & item, uint32_t) { return item.id; }
	};
	struct ItemCells {
		using id_type = uint32_t;
		static constexpr bool HasCellLocalIds = false;
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(const auto & x : item.cells) {
				*out = x;
				++out;
			}
		}
	};
	struct ForeignObjectsPredicate {
		bool operator()(item_type) { return false; }
	};
	struct ForeignObjects {
		template<typename TOutputIterator>
		void cells(item_type, TOutputIterator) {}
		template<typename TOutputIterator>
		void items(item_type, uint32_t, TOutputIterator) {}
	};
public:
	ExactStrings exactStrings() { return ExactStrings(); }
	SuffixStrings suffixStrings() { return SuffixStrings(); }
	ItemId itemId() { return ItemId(); }
	ItemCells itemCells() { return ItemCells(); }
	ForeignObjectsPredicate foreignObjectsPredicate() { return ForeignObjectsPredicate(); }
	ForeignObjects foreignObjects() { return ForeignObjects(); }
};


class OOMCTCTest: public CTCBaseTest {
CPPUNIT_TEST_SUITE( OOMCTCTest );
CPPUNIT_TEST( testExactItem );
CPPUNIT_TEST( testExactRegion );
CPPUNIT_TEST( testExactAll );

CPPUNIT_TEST( testPrefixItem );
CPPUNIT_TEST( testPrefixRegion );
CPPUNIT_TEST( testPrefixAll );

CPPUNIT_TEST( testSuffixItem );
CPPUNIT_TEST( testSuffixRegion );
CPPUNIT_TEST( testSuffixAll );

CPPUNIT_TEST( testSubStringItem );
CPPUNIT_TEST( testSubStringRegion );
CPPUNIT_TEST( testSubStringAll );

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
	virtual void setUp() override{
		sserialize::ItemIndexFactory idxFactory(true);
		sserialize::UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		
		sserialize::appendSACTC(ra().items.begin(), ra().items.end(),
								ra().regions.begin(), ra().regions.end(),
								OOM_SA_CTC_Traits(), OOM_SA_CTC_Traits(),
								0xFFFFFFFF, 0, 2, 0, sserialize::MM_SLOW_FILEBASED, supportedQuerries,
								idxFactory, dest);
		idxFactory.flush();
		sserialize::Static::ItemIndexStore idxStore(idxFactory.getFlushedData());
		m_ctc = sserialize::Static::CellTextCompleter(dest, idxStore, ra().gh);
	}
	virtual void tearDown() override {}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  OOMCTCTest::suite() );
	if (sserialize::tests::TestBase::popProtector()) {
		runner.eventManager().popProtector();
	}
	bool ok = runner.run();
	return ok ? 0 : 1;
}
