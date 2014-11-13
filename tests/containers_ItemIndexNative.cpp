#include "containers_ItemIndexBaseTest.h"
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h>

class ItemIndexPrivateNativeTest: public ItemIndexPrivateBaseTest {
CPPUNIT_TEST_SUITE( ItemIndexPrivateNativeTest );
CPPUNIT_TEST( testRandomEquality );
CPPUNIT_TEST( testSpecialEquality );
CPPUNIT_TEST( testIntersect );
CPPUNIT_TEST( testUnite );
CPPUNIT_TEST( testDynamicBitSet );
CPPUNIT_TEST( testPutIntoVector );
CPPUNIT_TEST( testIterator );
CPPUNIT_TEST_SUITE_END();
protected:
	virtual bool create(const std::set<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		sserialize::UByteArrayAdapter dest(sserialize::UByteArrayAdapter::createCache(1, false));
		bool ok = sserialize::detail::ItemIndexPrivate::ItemIndexPrivateNative::create(srcSet, dest);
		idx = sserialize::ItemIndex(dest, sserialize::ItemIndex::T_NATIVE);
		return ok;
	}
	virtual bool create(const std::vector<uint32_t> & srcSet, sserialize::ItemIndex & idx) override {
		sserialize::UByteArrayAdapter dest(sserialize::UByteArrayAdapter::createCache(1, false));
		bool ok = sserialize::detail::ItemIndexPrivate::ItemIndexPrivateNative::create(srcSet, dest);
		idx = sserialize::ItemIndex(dest, sserialize::ItemIndex::T_NATIVE);
		return ok;
	}
};
int main() {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  ItemIndexPrivateNativeTest::suite() );
	runner.run();
	return 0;
}