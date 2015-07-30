#include <sserialize/containers/GeneralizedTrie/SinglePassTrie.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"
#include <sserialize/storage/MmappedMemory.h>
#include <cppunit/TestResult.h>

using namespace sserialize;

enum { BO_TREE_CASE_SENSITIVE=0x1, BO_SUFFIX_TREE=0x2};

template<uint32_t T_BUILD_OPTS, Static::TrieNode::Types NODE_TYPE>
class StaticGeneralizedTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticGeneralizedTrieTest );
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
CPPUNIT_TEST( testSinglePassTrieEquality );
CPPUNIT_TEST( testSinglePassTrieIndexEquality );
CPPUNIT_TEST_SUITE_END();
private: //builds opts;
	bool m_caseSensitive;
	bool m_suffixTrie;
private:
	GeneralizedTrie::SinglePassTrie m_trie;
	std::deque<uint8_t> m_stTrieList;
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig m_config;
	StringCompleter m_strCompleter;
	ItemIndexFactory m_indexFactory;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }

	void createTrie(GeneralizedTrie::SinglePassTrie & tempTrie) {
		tempTrie.setCaseSensitivity(m_caseSensitive);
	
		StringsItemDBWrapper<TestItemData> & myDB = db();
		bool suffixTrie = m_suffixTrie;
		
		auto derefer = [&myDB, suffixTrie](uint32_t itemId, GeneralizedTrie::SinglePassTrie::StringsContainer & prefixStrings, GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings) {
			const TestItemData & item = myDB.at(itemId);
			if (suffixTrie) {
				suffixStrings.insert(item.strs.cbegin(), item.strs.cend());
			}
			prefixStrings.insert(item.strs.cbegin(), item.strs.cend());
		};
	
		tempTrie.fromStringsFactory(derefer, 0, myDB.size(), sserialize::MM_PROGRAM_MEMORY);
		tempTrie.trieSerializationProblemFixer();
	}

protected:
	bool setUpCompleter() {
		createTrie(m_trie);
		
// 		std::cout << m_trie.complete("m", sserialize::StringCompleter::QT_SUFFIX) << std::endl;
		
		m_config.trieList = &m_stTrieList;
		
		m_config.nodeType = NODE_TYPE;
		m_trie.createStaticTrie(m_config);

		m_config.indexFactory->flush();
		UByteArrayAdapter idxAdap(m_config.indexFactory->getFlushedData());
		sserialize::Static::ItemIndexStore idxStore(idxAdap);

		UByteArrayAdapter trieAdap(&m_stTrieList);
		stringCompleter() = StringCompleter( new sserialize::Static::StringCompleter(trieAdap, idxStore) );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP | StringCompleter::SQ_CASE_INSENSITIVE;
		if (m_caseSensitive)
			sq |= StringCompleter::SQ_CASE_SENSITIVE;
			
		if (m_suffixTrie)
			sq |= StringCompleter::SQ_SSP;
			
		return (StringCompleter::SupportedQuerries) sq;
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
	sserialize::Static::GeneralizedTrie * priv() {
		return dynamic_cast<sserialize::Static::GeneralizedTrie*>(dynamic_cast<sserialize::Static::StringCompleter*>(stringCompleter().getPrivate())->priv().get());
	}
	
public:
    StaticGeneralizedTrieTest() {
		m_caseSensitive = T_BUILD_OPTS & BO_TREE_CASE_SENSITIVE;
		m_suffixTrie = T_BUILD_OPTS & BO_SUFFIX_TREE;
		m_config.indexFactory = &m_indexFactory;
		m_config.indexFactory->setIndexFile(UByteArrayAdapter(new std::vector<uint8_t>(1024*1024,0), true));
    }

	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}
	
	void testSinglePassTrieEquality() {
		Static::GeneralizedTrie * stTriePtr = priv();
		
		CPPUNIT_ASSERT( stTriePtr );
		
		CPPUNIT_ASSERT( m_trie.checkTrieEquality(m_config, *stTriePtr) );
	}

	void testSinglePassTrieIndexEquality() {
		
		sserialize::GeneralizedTrie::SinglePassTrie tempTrie;
		createTrie(tempTrie);
		
		Static::GeneralizedTrie * stTriePtr = priv();
		
		CPPUNIT_ASSERT( stTriePtr );

		CPPUNIT_ASSERT_MESSAGE("Exact indices", tempTrie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_EXACT));
		if (stringCompleter().getSupportedQuerries() & StringCompleter::SQ_SUFFIX)
			CPPUNIT_ASSERT_MESSAGE("Suffix indices", tempTrie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_SUFFIX));

		CPPUNIT_ASSERT_MESSAGE("Prefix indices", tempTrie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_PREFIX));
		if (stringCompleter().getSupportedQuerries() & StringCompleter::SQ_SUBSTRING)
			CPPUNIT_ASSERT_MESSAGE("Substring indices", tempTrie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_SUBSTRING));
		
	}
	
	void testStringCompleterPrivateCast() {
		Static::GeneralizedTrie * p = priv();
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_SIMPLE>::suite() );
	
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_COMPACT>::suite() );
// 
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}
