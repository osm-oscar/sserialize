#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/log.h>
#include <sserialize/strings/stringfunctions.h>
#include <random>

#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;

uint32_t strTableSize = 1024;

class TestStringTable: public sserialize::tests::TestBase {
private:
	std::vector<std::string> m_strs;
public:
	std::vector<std::string> & strs() { return m_strs; }
	virtual const sserialize::Static::StringTable & stable() = 0;
	virtual const sserialize::UByteArrayAdapter & stableData() = 0;
public:
	virtual void setUp() {}
	virtual void tearDown() {}

	void testEq() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", stableData().size(), stable().getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)strs().size(), (uint32_t) stable().size());
		for(uint32_t i = 0, s = (uint32_t) strs().size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", i), stable().at(i), strs().at(i));
		}
	}
};

class TestSortedStringTable: public TestStringTable {
CPPUNIT_TEST_SUITE( TestSortedStringTable );
CPPUNIT_TEST( testEq );
CPPUNIT_TEST( testFind );
CPPUNIT_TEST( testRange );
CPPUNIT_TEST_SUITE_END();
private:
	sserialize::UByteArrayAdapter m_stableData;
	sserialize::Static::SortedStringTable m_stable;
public:
	virtual const sserialize::Static::StringTable & stable() { return m_stable; }
	virtual const sserialize::UByteArrayAdapter & stableData() { return m_stableData; }
public:
	virtual void setUp() {
		std::set<std::string> myStrs;
		std::geometric_distribution<uint32_t> dist(0.25);
		std::default_random_engine generator(0);
		for(uint32_t i = 0; i < strTableSize; ++i) {
			std::string str = createString(100);
			uint32_t numPrefix = std::min<uint32_t>(dist(generator), str.size());
			myStrs.insert(str);
			for(uint32_t i(0); i < numPrefix; ++i) {
				myStrs.insert(std::string(str.begin(), str.end()-i));
			}
		}
		strs().insert(strs().end(), myStrs.cbegin(), myStrs.cend());
		m_stableData = sserialize::UByteArrayAdapter(new std::vector<uint8_t>(), true);
		sserialize::Static::SortedStringTable::create(strs().cbegin(), strs().cend(), m_stableData);
		m_stable = sserialize::Static::SortedStringTable(m_stableData);
	}
	
	void testConstructionParameters() {
		CPPUNIT_ASSERT_MESSAGE("minimum size", m_stable.size() > strTableSize*2);
	}
	
	void testFind() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", stableData().size(), stable().getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)strs().size(), (uint32_t) stable().size());
		for(Static::SortedStringTable::SizeType i = 0, s = strs().size(); i < s; ++i) {
			auto pos = m_stable.find(strs().at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", i), i, pos);
		}
	}
	
	void testRange() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", stableData().size(), stable().getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)strs().size(), (uint32_t) stable().size());
		for(Static::SortedStringTable::SizeType i = 0, s = strs().size(); i < s; ++i) {
			auto lb = m_stable.find(strs().at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", i), i, lb);
			auto ub = lb+1;
			for(; ub < m_stable.size() &&  sserialize::isPrefix(strs().at(i), strs().at(ub)); ++ub) {}
			auto r = m_stable.range(strs().at(i));
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Lower bound", lb, r.first);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("Upper bound", ub, r.second);
		}
		auto r = m_stable.range("hjhkjadzofmaoihfdoannfojafojdsafhsafhskdafkdsafsafsafd");
		CPPUNIT_ASSERT_EQUAL_MESSAGE("Invalid range search: range size", uint32_t(0), uint32_t(r.second-r.first));
		
		r = m_stable.range("");
		CPPUNIT_ASSERT_EQUAL_MESSAGE("All range search: Lower bound", Static::SortedStringTable::SizeType(0), r.first);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("All range search: Upper bound", m_stable.size(), r.second);
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	for(uint32_t i = 0; i < 10; ++i) {
		runner.addTest( TestSortedStringTable::suite() );
	}
	bool ok = runner.run();
	return (ok ? 0 : 1);
}
