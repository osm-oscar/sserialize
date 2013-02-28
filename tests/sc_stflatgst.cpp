#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/containers/GeneralizedTrieHelpers.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"
#include "datacreationfuncs.h"

using namespace sserialize;

typedef enum {BO_NONE=0x0, BO_TREE_CASE_SENSITIVE=0x1, BO_SUFFIX_TREE=0x2, BO_DELETE_TRIE=0x4, BO_MERGE_INDEX=0x8} FlatGSTBuildOptions;

template<unsigned int T_BUILD_OPTS>
class StaticFlatGeneralizedTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticFlatGeneralizedTrieTest );
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
CPPUNIT_TEST( testFlatGSTEquality );
CPPUNIT_TEST( testConsistency );
CPPUNIT_TEST_SUITE_END();
private:
	GeneralizedTrie<TestItemData> m_trie;
	bool m_treeCaseSensitive;
	bool m_suffixTree;
	bool m_deletTree;
	bool m_mergeIndex;
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }
protected:
	bool setUpCompleter() {
		m_trie.setCaseSensitivity(m_treeCaseSensitive);
		m_trie.setSuffixTrie(m_suffixTree);
		
		m_trie.setDB(createDB());
		
		UByteArrayAdapter data(new std::vector<uint8_t>, true);
		ItemIndexFactory idxFactory;
		
		FlatGSTConfig config(data, idxFactory, false, 0, 0xFFFFFFFF, false, m_mergeIndex);
		
		m_trie.createStaticFlatTrie(config);
		
		idxFactory.flush();
		UByteArrayAdapter idxAdap( idxFactory.getFlushedData() );

		data += STATIC_STRING_COMPLETER_HEADER_SIZE; //skip StringCompleter::header
		
		sserialize::Static::FlatGST * p = new sserialize::Static::FlatGST(data, idxAdap);
		stringCompleter() = StringCompleter( p );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP;
		if (m_treeCaseSensitive)
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
	StaticFlatGeneralizedTrieTest() :
	m_treeCaseSensitive(T_BUILD_OPTS & BO_TREE_CASE_SENSITIVE),
	m_suffixTree(T_BUILD_OPTS & BO_SUFFIX_TREE),
	m_deletTree(T_BUILD_OPTS & BO_DELETE_TRIE),
	m_mergeIndex(T_BUILD_OPTS & BO_MERGE_INDEX)
	{}

	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}

	void testStringCompleterPrivateCast() {
		Static::FlatGST * p = dynamic_cast<sserialize::Static::FlatGST*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
	
	void testFlatGSTEquality() {
		if (!m_deletTree) {
			Static::FlatGST * p = dynamic_cast<sserialize::Static::FlatGST*>(stringCompleter().getPrivate());
			CPPUNIT_ASSERT( m_trie.checkFlatTrieEquality(*p) );
		}
	}
	
	void testConsistency() {
		if (!m_deletTree) {
			CPPUNIT_ASSERT( m_trie.consistencyCheck() );
		}
	}
};

class StaticFlatGSTUtilTest : public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE( StaticFlatGSTUtilTest );
CPPUNIT_TEST( testFlatGSTIndexEntry );
CPPUNIT_TEST( testFlatGSTStringEntry );
CPPUNIT_TEST_SUITE_END();
public:
	void testFlatGSTIndexEntry() {
		uint32_t count = 0x1FFFF;
		std::deque<IndexEntry> entries;
		std::deque<uint32_t> ptrs[5] = {
			createNumbers(count),
			createNumbers(count),
			createNumbers(count),
			createNumbers(count),
			createNumbers(count)
		};
		for(size_t i = 0; i < 5; ++i) {
			ptrs[i].push_back(0xFEFEFEFE);
		}
		for(size_t i = 0; i < count; ++i) {
			IndexEntry e;
			e.exactValues = ptrs[0][i];
			e.suffixValues = ptrs[1][i];
			e.prefixValues = ptrs[2][i];
			e.suffixPrefixValues = ptrs[3][i];
			e.itemIdIndex = true;
			entries.push_back(e);
		}
		
		{
			UByteArrayAdapter dest(new std::deque<uint8_t>(), true);
			dest << entries;
			dest.resetPtrs();
			Static::Deque<Static::FlatGST::IndexEntry> staticEntries(dest);
			for(size_t i = 0; i < count; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("exactValues", entries[i].exactValues, staticEntries.at(i).exactPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixValues", entries[i].suffixValues, staticEntries.at(i).suffixPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("prefixValues", entries[i].prefixValues, staticEntries.at(i).prefixPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixPrefixValues", entries[i].suffixPrefixValues, staticEntries.at(i).suffixPrefixPtr());
				CPPUNIT_ASSERT(staticEntries.at(i).itemIdIndex());
			}
		}
		
		for(size_t i = 0; i < count; ++i) {
			entries[i].minId = ptrs[4][i] & 0xFFFFF;
			entries[i].maxId = ptrs[4][i] + (rand() & 0xFF);
			if (entries[i].minId > entries[i].maxId)
				std::swap(entries[i].minId, entries[i].maxId);
			entries[i].itemIdIndex = false;
		}

		{
			UByteArrayAdapter dest(new std::deque<uint8_t>(), true);
			dest << entries;
			dest.resetPtrs();
			Static::Deque<Static::FlatGST::IndexEntry> staticEntries(dest);
			for(size_t i = 0; i < count; ++i) {
				CPPUNIT_ASSERT_EQUAL_MESSAGE("exactValuesItemId", entries[i].exactValues, staticEntries.at(i).exactPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixValuesItemId", entries[i].suffixValues, staticEntries.at(i).suffixPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("prefixValuesItemId", entries[i].prefixValues, staticEntries.at(i).prefixPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixPrefixValuesItemId", entries[i].suffixPrefixValues, staticEntries.at(i).suffixPrefixPtr());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("minIdItemId", entries[i].minId, staticEntries.at(i).minId());
				CPPUNIT_ASSERT_EQUAL_MESSAGE("maxIdItemId", entries[i].maxId, staticEntries.at(i).maxId());
				CPPUNIT_ASSERT(!staticEntries.at(i).itemIdIndex());
			}
		}
	}
	
	void testFlatGSTStringEntry() {
		uint32_t count = 0x1FFFF;
		std::deque<uint32_t> strId = createNumbers(count);
		std::deque<uint16_t> strPos = createNumbers16(count);
		std::deque<uint16_t> strLen = createNumbers16(count);
		strId.push_back(0xFEFEFEFE);
		strPos.push_back(0xFEFE);
		strLen.push_back(0xFEFE);
		
		UByteArrayAdapter dest(new std::vector<uint8_t>(), true);
		std::vector<uint8_t> bitConfig(3, 32);
		MultiVarBitArrayCreator creator(bitConfig, dest);
		creator.reserve(count/2);
		for(size_t i = 0; i < count; ++i) {
			creator.set(i, 0, strId[i]);
			creator.set(i, 1, strPos[i]);
			creator.set(i, 2, strLen[i]);
		}
		MultiVarBitArray staticEntries( creator.flush() );
		
		dest.resetPtrs();
		for(size_t i = 0; i < count; ++i) {
			std::stringstream ss;
			ss << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE("stringId at " + ss.str(), strId[i], sserialize::Static::FlatGST::StringEntry(staticEntries, i ).strId());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("strBegin at " + ss.str(), strPos[i], sserialize::Static::FlatGST::StringEntry(staticEntries, i ).strBegin());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("strLen at " + ss.str(), strLen[i],  sserialize::Static::FlatGST::StringEntry(staticEntries, i ).strLen());
		}
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticFlatGeneralizedTrieTest<BO_NONE>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x1>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x2>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x3>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x4>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x5>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x6>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x7>::suite() );

	runner.addTest( StaticFlatGeneralizedTrieTest<0x8>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0x9>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xA>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xB>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xC>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xD>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xE>::suite() );
	runner.addTest( StaticFlatGeneralizedTrieTest<0xF>::suite() );
	
	runner.addTest( StaticFlatGSTUtilTest::suite() );
	
	runner.run();
	return 0;
}
