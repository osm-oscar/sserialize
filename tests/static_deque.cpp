#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include <deque>

using namespace sserialize;

std::deque< std::deque<uint32_t> > createDequeOfDequeNumbers(uint32_t maxCountPerDeque, uint32_t maxDequeCount) {
	std::deque< std::deque<uint32_t> > res;
	for(size_t i = 0; i < maxDequeCount; i++) {
		uint32_t tmpCount = (double)rand()/RAND_MAX * maxCountPerDeque;
		res.push_back(sserialize::createNumbers(tmpCount));
	}
	return res;
}

int TestCount = 10;
uint32_t TestMask = 0xFFF;

class TestDeque: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestDeque );
CPPUNIT_TEST( testNumbers );
CPPUNIT_TEST( testStrings );
CPPUNIT_TEST( testDequeInDeque );
CPPUNIT_TEST( testStringsRawPut );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testNumbers() {
		std::deque<uint32_t> realValues = createNumbers(TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		d.resetPutPtr();
		sserialize::Static::Deque<uint32_t> sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], sd.at(i));
		}
	}
	
	void testStrings() {
		std::deque<std::string> realValues = createStrings(33, TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		sserialize::Static::Deque<std::string> sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], sd.at(i));
		}
	}
	
	void testStringsRawPut() {
		std::deque<std::string> realValues = createStrings(33, TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		Static::DequeCreator<std::string> creator(d);
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			creator.beginRawPut();
			creator.rawPut() << realValues[i];
			creator.endRawPut();
		}
		sserialize::Static::Deque<std::string> sd(creator.flush());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], sd.at(i));
		}
	}
	
	void testDequeInDeque() {
		std::deque< std::deque<uint32_t> > realValues = createDequeOfDequeNumbers(TestMask & rand(), TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		sserialize::Static::Deque< Static::Deque<uint32_t>  > sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			Static::Deque<uint32_t> t = sd.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(toString("size at ", i), (uint32_t)realValues[i].size(), t.size());
			for(uint32_t j = 0, sj = realValues[i].size(); j < sj; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at [", i,":",j,"]"), realValues[i][j], t.at(j));
			}
		}
	}
	

	
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestDeque::suite() );
	runner.run();
	return 0;
}