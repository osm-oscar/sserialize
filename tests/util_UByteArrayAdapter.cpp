#include <sserialize/storage/UByteArrayAdapter.h>
#include "datacreationfuncs.h"
#include "TestBase.h"

constexpr uint32_t TStringsCount = 1024;
constexpr uint32_t TIntegerCount = 10240;

struct IntegerType {
	int64_t value;
	typedef enum {U8=0, U16=1, U24=2, U32=3, U64=4, S32=5, S64=6, VU32=7, VU64=8, VS32=9, VS64=10, TYPE_COUNT=11} Type;
	int type;
	IntegerType(int64_t value, int type) : value(value), type(type) {}
	IntegerType() : value(0xFEFE), type(0xFEFE) {}
	bool operator==(const IntegerType & other) const { return value == other.value && type == other.type; }
};

std::ostream & operator<<(std::ostream & out, const IntegerType & t) {
	out << "IntegerType(value=" << t.value << ", type=";
	switch (t.type) {
#define singlecase(__NAME) case IntegerType::__NAME: out << #__NAME; break;
	singlecase(U8)
	singlecase(U16)
	singlecase(U24)
	singlecase(U32)
	singlecase(U64)
	singlecase(S32)
	singlecase(S64)
	singlecase(VU32)
	singlecase(VU64)
	singlecase(VS32)
	singlecase(VS64)
#undef singlecase
	default:
		std::cout << "invalid";
		break;
	}
	out << ")";
	return out;
}

template<typename TOutputIterator>
void createIntegers(int32_t count, TOutputIterator out) {
	*out = IntegerType(0xFF, IntegerType::U8); ++out;
	*out = IntegerType(0xFFFF, IntegerType::U16); ++out;
	*out = IntegerType(0xFFFFFF, IntegerType::U24); ++out;
	*out = IntegerType(std::numeric_limits<uint32_t>::max(), IntegerType::U32); ++out;
	*out = IntegerType(std::numeric_limits<uint64_t>::max(), IntegerType::U64); ++out;
	*out = IntegerType(std::numeric_limits<int32_t>::max(), IntegerType::S32); ++out;
	*out = IntegerType(std::numeric_limits<int32_t>::min(), IntegerType::S32); ++out;
	*out = IntegerType(std::numeric_limits<int64_t>::max(), IntegerType::S64); ++out;
	*out = IntegerType(std::numeric_limits<int64_t>::min(), IntegerType::S64); ++out;

	*out = IntegerType(std::numeric_limits<uint32_t>::max(), IntegerType::VU32); ++out;
	*out = IntegerType(std::numeric_limits<uint64_t>::max(), IntegerType::VU64); ++out;
	*out = IntegerType(std::numeric_limits<int32_t>::max(), IntegerType::VS32); ++out;
	*out = IntegerType(std::numeric_limits<int32_t>::min()+1, IntegerType::VS32); ++out;
	*out = IntegerType(std::numeric_limits<int64_t>::max(), IntegerType::VS64); ++out;
	*out = IntegerType(std::numeric_limits<int64_t>::min()+1, IntegerType::VS64); ++out;
	
	while(count > 0) {
		IntegerType t;
		int64_t & value = t.value;
		int & type = t.type;
		value = rand();
		value <<= 31;
		value |= rand();
		type = rand() % IntegerType::TYPE_COUNT;
		bool isSigned = false;
		switch (type) {
		case IntegerType::U8:
			value = value & 0xFF;
			break;
		case IntegerType::U16:
			value = value & 0xFFFF;
			break;
		case IntegerType::U24:
			value = value & 0xFFFFFF;
			break;
		case IntegerType::U32:
		case IntegerType::VU32:
			value = value & 0xFFFFFFFF;
			break;
		case IntegerType::S32:
		case IntegerType::VS32:
			isSigned = true;
			value = value & 0x7FFFFFFF;
			break;
		case IntegerType::S64:
		case IntegerType::VS64:
			isSigned = true;
			break;
		default:
			break;
		};
		if (isSigned && rand() % 2) {
			value = -value;
		}
		
		*out = t;
		++out;
		--count;
	}
}

bool put(sserialize::UByteArrayAdapter & d, const IntegerType & t) {
	switch (t.type) {
	case IntegerType::U8:
		return d.putUint8((uint8_t)t.value);
	case IntegerType::U16:
		return d.putUint16((uint16_t)t.value);
	case IntegerType::U24:
		return d.putUint24((uint32_t)t.value);
	case IntegerType::U32:
		return d.putUint32((uint32_t)t.value);
	case IntegerType::U64:
		return d.putUint64((uint64_t)t.value);
	case IntegerType::S32:
		return d.putInt32((int32_t)t.value);
	case IntegerType::S64:
		return d.putInt64((int64_t)t.value);
	case IntegerType::VU32:
		return d.putVlPackedUint32((uint32_t)t.value);
	case IntegerType::VS32:
		return d.putVlPackedInt32((int32_t)t.value);
	case IntegerType::VU64:
		return d.putVlPackedUint64((uint64_t)t.value);
	case IntegerType::VS64:
		return d.putVlPackedInt64((int64_t)t.value);
	default:
		return false;
	}
}

void get(sserialize::UByteArrayAdapter & d, const IntegerType & t, IntegerType & ot) {
	ot.type = t.type;
	switch (t.type) {
	case IntegerType::U8:
		ot.value = d.getUint8();
		break;
	case IntegerType::U16:
		ot.value = d.getUint16();
		break;
	case IntegerType::U24:
		ot.value = d.getUint24();
		break;
	case IntegerType::U32:
		ot.value = d.getUint32();
		break;
	case IntegerType::U64:
		ot.value = d.getUint64();
		break;
	case IntegerType::S32:
		ot.value = d.getInt32();
		break;
	case IntegerType::S64:
		ot.value = d.getInt64();
		break;
	case IntegerType::VU32:
		ot.value = d.getVlPackedUint32();
		break;
	case IntegerType::VS32:
		ot.value = d.getVlPackedInt32();
		break;
	case IntegerType::VU64:
		ot.value = d.getVlPackedUint64();
		break;
	case IntegerType::VS64:
		ot.value = d.getVlPackedInt64();
		break;
	default:
		break;
	}
}

class UBABaseTest: public sserialize::tests::TestBase {
protected:
	virtual sserialize::UByteArrayAdapter createUBA() { return sserialize::UByteArrayAdapter(); }
	virtual sserialize::UByteArrayAdapter createUBA(const sserialize::UByteArrayAdapter & /*src*/) { return sserialize::UByteArrayAdapter(); }
public:
	UBABaseTest() {}
	virtual ~UBABaseTest() {}
	virtual void setUp() {}
	virtual void tearDown() {}
	void testStrings() {
		std::vector<std::string> strs(sserialize::createStringsVec(128, TStringsCount));
		sserialize::UByteArrayAdapter d(createUBA());
		for(const std::string & str : strs) {
			CPPUNIT_ASSERT(d.putString(str));
		}
		
		for(uint32_t i(0); i < strs.size() && d.tellGetPtr() < d.size(); ++i) {
			CPPUNIT_ASSERT_EQUAL(strs.at(i), d.getString());
		}
		
		d.resetGetPtr();
		for(uint32_t i(0); i < strs.size() && d.tellGetPtr() < d.size(); ++i) {
			CPPUNIT_ASSERT_EQUAL(strs.at(i), d.getStringData().toString());
		}
		
		d.resetGetPtr();
		for(uint32_t i(0); i < strs.size() && d.tellGetPtr() < d.size(); ++i) {
			CPPUNIT_ASSERT_EQUAL((uint32_t) strs.at(i).size(), d.getStringLength());
			d.incGetPtr(strs.at(i).size());
		}
	}
	void testIntegers() {
		std::vector<IntegerType> src;
		createIntegers(TIntegerCount, std::back_inserter<decltype(src)>(src));
		sserialize::UByteArrayAdapter d(createUBA());
		
		for(uint32_t i(0); i < src.size(); ++i) {
			const IntegerType & t = src.at(i);
			CPPUNIT_ASSERT(put(d, t));
		}
		
		d.resetGetPtr();
		for(uint32_t i(0); i < src.size(); ++i) {
			const IntegerType & t = src.at(i);
			IntegerType ot;
			get(d, t, ot);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("At position " + std::to_string(i), t, ot);
		}
		
		d.resetGetPtr();
	}
	
	void testPutGetPtrs() {
		sserialize::UByteArrayAdapter d(createUBA());
		
		d.resize(8);
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)8, d.size());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.tellPutPtr());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.tellGetPtr());
		
		d.putUint32(123);
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)4, d.size());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)4, d.tellPutPtr());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.tellGetPtr());
		
		d.getUint32();
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)8, d.size());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)4, d.tellPutPtr());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)4, d.tellGetPtr());
		
		d.resize(0);
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.size());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.tellGetPtr());
		CPPUNIT_ASSERT_EQUAL((sserialize::UByteArrayAdapter::OffsetType)0, d.tellPutPtr());
	}
	
};

class UBAVec: public UBABaseTest {
CPPUNIT_TEST_SUITE( UBAVec );
CPPUNIT_TEST(testStrings);
CPPUNIT_TEST(testIntegers);
CPPUNIT_TEST(testPutGetPtrs);
CPPUNIT_TEST_SUITE_END();
protected:
	virtual sserialize::UByteArrayAdapter createUBA() override {
		return sserialize::UByteArrayAdapter(new std::vector<uint8_t>(), true);
	}
	virtual sserialize::UByteArrayAdapter createUBA(const sserialize::UByteArrayAdapter & src) override {
		sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
		tmp.put(src);
		return tmp;
	}
public:
	UBAVec() {}
	virtual ~UBAVec() {}
};



int main(int argc, char ** argv) {
	sserialize::tests::TestBase::argc = argc;
	sserialize::tests::TestBase::argv = argv;
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  UBAVec::suite() );

	runner.eventManager().popProtector();
	
	bool ok = runner.run();
	return ok ? 0 : 1;
}