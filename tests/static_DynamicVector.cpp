#include <sserialize/Static/DynamicVector.h>
#include <sserialize/utility/log.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

using namespace sserialize;

struct ComplexData {
	ComplexData() : str1(createString(40)), str2(createString(20)), num1(rand()), num2(rand()) {} 
	std::string str1;
	std::string str2;
	uint32_t num1;
	uint64_t num2;
};

bool operator==(const ComplexData & a, const ComplexData & b) {
	return (a.str1 == b.str1) && (a.str2 == b.str2) && (a.num1 == b.num1) && (a.num2 == b.num2);
}

std::ostream & operator<<(std::ostream & out, const ComplexData & a) {
	out << "ComplexData[" << a.str1 << ", " << a.str2 << ", " << a.num1 << ", " << a.num2 << "]";
	return out;
}

struct ComplexDataSerializer {
	void operator()(const ComplexData & src, sserialize::UByteArrayAdapter & dest) const {
		dest << src.str1 << src.str2 << src.num1 << src.num2;
	}
};

struct ComplexDataDeserializer {
	ComplexData operator()(const UByteArrayAdapter & data) const {
		UByteArrayAdapter dd = data;
		ComplexData d;
		dd >> d.str1 >> d.str2 >> d.num1 >> d.num2;
		return d;
	};
};


template<int NumberOfItems>
class DynamicVectorTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( DynamicVectorTest );
CPPUNIT_TEST( testDefaultSerializer );
CPPUNIT_TEST( testComplexSerialize );
CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	void testDefaultSerializer() {
		std::deque<uint32_t> rawData = sserialize::createNumbers(NumberOfItems);
		sserialize::Static::DynamicVector<uint32_t, uint32_t> dynVec(NumberOfItems, NumberOfItems*sserialize::SerializationInfo<uint32_t>::length);
		for(uint32_t i = 0, s = (uint32_t) rawData.size(); i < s; ++i) {
			dynVec.push_back(rawData[i]);
		}
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", (uint32_t)rawData.size(), dynVec.size());
		
		for(uint32_t i = 0, s = (uint32_t) rawData.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item at ", i), rawData[i], dynVec.at(i));
		}
	}
	
	void testComplexSerialize() {
		std::vector<ComplexData> rawData(NumberOfItems);
		sserialize::Static::DynamicVector<ComplexData> dynVec(NumberOfItems, NumberOfItems*12);
		for(uint32_t i = 0, s = (uint32_t) rawData.size(); i < s; ++i) {
			dynVec.push_back(rawData[i], ComplexDataSerializer());
		}
		
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size does not match", (uint32_t)rawData.size(), dynVec.size());
		
		for(uint32_t i = 0, s = (uint32_t) rawData.size(); i < s; ++i) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("item at ", i), rawData[i], dynVec.at(i, ComplexDataDeserializer()));
		}
	}
	
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  DynamicVectorTest<10>::suite() );
	runner.addTest(  DynamicVectorTest<100>::suite() );
	runner.addTest(  DynamicVectorTest<1000>::suite() );
	runner.run();
	return 0;
}