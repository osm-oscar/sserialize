#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/Static/FlatGSTStrIds.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

typedef enum {BO_NONE=0x0, BO_TREE_CASE_SENSITIVE=0x1, BO_SUFFIX_TREE=0x2, BO_DELETE_TRIE=0x4, BO_MERGE_INDEX=0x8} FlatGSTBuildOptions;

template<unsigned int T_BUILD_OPTS, uint32_t T_MAX_ITEM_IDS_FOR_STRID_INDEX=0, uint32_t T_MIN_STR_LEN_FOR_ITEM_ID_INDEX=0>
class StaticFlatGeneralizedTrieStrIdsTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticFlatGeneralizedTrieStrIdsTest );
CPPUNIT_TEST( testCreateStringCompleter );
CPPUNIT_TEST( testSupportedQuerries );
CPPUNIT_TEST( testCompletionECS );
CPPUNIT_TEST( testCompletionECI );
CPPUNIT_TEST( testCompletionPCS );
CPPUNIT_TEST( testCompletionPCI );
CPPUNIT_TEST( testCompletionSCS );
CPPUNIT_TEST( testCompletionSCI );
CPPUNIT_TEST( testCompletionSPCS );
CPPUNIT_TEST( testCompletionSPCI );
CPPUNIT_TEST( testStringCompleterPrivateCast );
CPPUNIT_TEST( testObeysBuildsOpts );
CPPUNIT_TEST( testFlatGSTEquality );
CPPUNIT_TEST_SUITE_END();
private:
	typedef StringsItemDBWrapper<TestItemData> ItemDBType;
	typedef sserialize::Static::FlatGSTStrIds<StringsItemDBWrapper<TestItemData> > FGSTType;
	GeneralizedTrie::FlatTrie m_trie;
	
	bool m_caseSensitive;
	bool m_suffixTree;
	bool m_deleteTree;
	bool m_mergeIndex;
	StringCompleter m_strCompleter;
	
	StringsItemDBWrapper<TestItemData> m_db;
	
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
	
protected:
	bool setUpCompleter() {
		m_trie.setCaseSensitivity(m_caseSensitive);
		m_trie.setSuffixTrie(m_suffixTree);

		m_trie.setDB(db());
		
		UByteArrayAdapter data(new std::vector<uint8_t>, true);
		ItemIndexFactory idxFactory;
		
		GeneralizedTrie::FlatGSTConfig config(data, idxFactory, true, T_MAX_ITEM_IDS_FOR_STRID_INDEX, T_MIN_STR_LEN_FOR_ITEM_ID_INDEX, m_deleteTree, m_mergeIndex);
		
		m_trie.createStaticFlatTrie(config);
		
		
		idxFactory.flush();
		UByteArrayAdapter idxAdap(idxFactory.getFlushedData());

		data += STATIC_STRING_COMPLETER_HEADER_SIZE; //skip StringCompleter::header

		Static::ItemIndexStore idxStore(idxAdap);
		stringCompleter() = StringCompleter(
			new sserialize::Static::FlatGSTStrIds<StringsItemDBWrapper<TestItemData> >(data, idxStore, db()) );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP;
		if (m_caseSensitive)
			sq |= StringCompleter::SQ_CASE_SENSITIVE;
		else
			sq |= StringCompleter::SQ_CASE_INSENSITIVE;
			
		if (m_suffixTree)
			sq |= StringCompleter::SQ_SSP;
			
		return (StringCompleter::SupportedQuerries) sq;
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
public:
	StaticFlatGeneralizedTrieStrIdsTest() :
		m_caseSensitive(T_BUILD_OPTS & BO_TREE_CASE_SENSITIVE),
		m_suffixTree(T_BUILD_OPTS & BO_SUFFIX_TREE),
		m_deleteTree(T_BUILD_OPTS & BO_DELETE_TRIE),
		m_mergeIndex(T_BUILD_OPTS & BO_MERGE_INDEX)
	{}

	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}

	void testStringCompleterPrivateCast() {
		FGSTType * p = dynamic_cast<FGSTType*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
	
	void testObeysBuildsOpts() {
		FGSTType * p = dynamic_cast<FGSTType*>(stringCompleter().getPrivate());
		CPPUNIT_ASSERT( p != 0 );
		uint32_t s = p->size();
		sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_PREFIX;
		if (m_suffixTree) {
			qt = sserialize::StringCompleter::QT_SUFFIX_PREFIX;
		}

		for(size_t i = 0; i< s; ++i) {
			sserialize::Static::FlatGST::IndexEntry e( p->indexEntryAt(i) );
			CPPUNIT_ASSERT(e.mergeIndex() == m_mergeIndex);

			UByteArrayAdapter fgstStr  = p->fgstStringAt(i);
			uint32_t strLen = utf8CharCount(fgstStr, fgstStr.end() );
			ItemIndex idx = p->indexFromPosition(i, qt);
			bool shouldBeItemIdIndex = (idx.size() < T_MAX_ITEM_IDS_FOR_STRID_INDEX) && (strLen >= T_MIN_STR_LEN_FOR_ITEM_ID_INDEX);
			bool isItemIdIndex = (e.indexType() & sserialize::Static::FlatGST::IndexEntry::IT_ITEM_ID_INDEX);
			CPPUNIT_ASSERT(shouldBeItemIdIndex == isItemIdIndex);
		}

	}
	
	void testFlatGSTEquality() {
		if (!m_deleteTree) {
			FGSTType * p = dynamic_cast<FGSTType*>(stringCompleter().getPrivate());
			CPPUNIT_ASSERT( m_trie.checkFlatTrieEquality(*p) );
		}
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF>::suite() );
	

	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE, 5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF, 5>::suite() );
	

	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE, 15>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF, 15>::suite() );
	

	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE, 1015>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF, 1015>::suite() );
	
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE, 5, 2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF, 5, 2>::suite() );

	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x0, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x1, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x2, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x3, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x4, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x5, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x6, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x7, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x8, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0x9, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xA, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xB, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xC, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xD, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xE, 1015, 3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieStrIdsTest<0xF, 1015, 3>::suite() );

	
	
// 	runner.eventManager();
	runner.run();
	return 0;
}
