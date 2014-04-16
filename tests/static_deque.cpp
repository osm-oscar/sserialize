#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <sserialize/Static/Array.h>
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

class TestArray: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( TestArray );
CPPUNIT_TEST( testNumbers );
CPPUNIT_TEST( testStrings );
CPPUNIT_TEST( testArrayInArray );
CPPUNIT_TEST( testStringsRawPut );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST( testAbstractArray );
CPPUNIT_TEST( testAbstractArrayIterator );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testNumbers() {
		std::deque<uint32_t> realValues = createNumbers(TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		d.resetPutPtr();
		sserialize::Static::Array<uint32_t> sd(d);
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
		sserialize::Static::Array<std::string> sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], sd.at(i));
		}
	}
	
	void testStringsRawPut() {
		std::deque<std::string> realValues = createStrings(33, TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		Static::ArrayCreator<std::string> creator(d);
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			creator.beginRawPut();
			creator.rawPut() << realValues[i];
			creator.endRawPut();
		}
		sserialize::Static::Array<std::string> sd(creator.flush());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], sd.at(i));
		}
	}
	
	void testArrayInArray() {
		std::deque< std::deque<uint32_t> > realValues = createDequeOfDequeNumbers(TestMask & rand(), TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		sserialize::Static::Array< Static::Array<uint32_t>  > sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			Static::Array<uint32_t> t = sd.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(toString("size at ", i), (uint32_t)realValues[i].size(), t.size());
			for(uint32_t j = 0, sj = realValues[i].size(); j < sj; ++j) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at [", i,":",j,"]"), realValues[i][j], t.at(j));
			}
		}
	}
	
	void testIterator() {
		std::deque<uint32_t> realValues = createNumbers(TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		d.resetPutPtr();
		sserialize::Static::Array<uint32_t> sd(d);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("data size", d.size(), sd.getSizeInBytes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		{
			sserialize::Static::Array<uint32_t>::const_iterator it(sd.cbegin());
			sserialize::Static::Array<uint32_t>::const_iterator end(sd.cend());
			for(uint32_t i = 0; it != end; ++it, ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], *it);
			}
		}
	}
	
	void testAbstractArray() {
		std::deque<uint32_t> realValues = createNumbers(TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		d.resetPutPtr();
		sserialize::Static::Array<uint32_t> sd(d);
		
		sserialize::AbstractArray<uint32_t> asd( new sserialize::Static::detail::VectorAbstractArray<uint32_t>(sd) );
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), asd.size());
		for(uint32_t i = 0, s = realValues.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], asd.at(i));
		}
	}
	
	void testAbstractArrayIterator() {
		std::deque<uint32_t> realValues = createNumbers(TestMask & rand());
		UByteArrayAdapter d(new std::vector<uint8_t>(), true);
		d << realValues;
		d.resetPutPtr();
		sserialize::Static::Array<uint32_t> sd(d);

		sserialize::AbstractArray<uint32_t> asd( new sserialize::Static::detail::VectorAbstractArray<uint32_t>(sd) );

		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t)realValues.size(), sd.size());
		{
			sserialize::AbstractArray<uint32_t>::const_iterator it(asd.cbegin());
			sserialize::AbstractArray<uint32_t>::const_iterator end(asd.cend());
			for(uint32_t i = 0; it != end; ++it, ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("at ", i), realValues[i], *it);
			}
		}
	}
	
};

int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestArray::suite() );
	runner.run();
	return 0;
}