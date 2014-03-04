#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

enum { BO_TREE_CASE_SENSITIVE=0x1, BO_SUFFIX_TREE=0x2, BO_MERGE_INDEX=0x4, BO_NO_FULL_INDEX_AT_ALL=0x8};

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
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testIndexEquality );
CPPUNIT_TEST_SUITE_END();
private: //builds opts;
	bool m_caseSensitive;
	bool m_suffixTrie;
	bool m_mergeIndex;
	bool m_noFullIndexAtAll;
private:
	GeneralizedTrie::MultiPassTrie m_trie;
	std::deque<uint8_t> m_stTrieList;
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig m_config;
	StringCompleter m_strCompleter;
	ItemIndexFactory m_indexFactory;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }

protected:
	bool setUpCompleter() {
		m_trie.setCaseSensitivity(m_caseSensitive);
		m_trie.setSuffixTrie(m_suffixTrie);
	
		m_trie.setDB(createDB());
		
		m_config.trieList = &m_stTrieList;
		m_config.mergeIndex = m_mergeIndex;
		
		if (m_noFullIndexAtAll)
			for(uint32_t i = 0; i <= 0xFF; ++i)
				m_config.levelsWithoutFullIndex.insert(i);
		
		m_config.nodeType = NODE_TYPE;

		m_trie.createStaticTrie(m_config);
		
		
		m_config.indexFactory->flush();
		UByteArrayAdapter idxAdap(m_config.indexFactory->getFlushedData());
		Static::ItemIndexStore idxStore(idxAdap);
		
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
	
public:
    StaticGeneralizedTrieTest() {
		m_caseSensitive = T_BUILD_OPTS & BO_TREE_CASE_SENSITIVE;
		m_suffixTrie = T_BUILD_OPTS & BO_SUFFIX_TREE;
		m_mergeIndex= T_BUILD_OPTS & BO_MERGE_INDEX;
		m_noFullIndexAtAll = T_BUILD_OPTS & BO_NO_FULL_INDEX_AT_ALL;
		m_config.indexFactory = &m_indexFactory;
		m_config.indexFactory->setIndexFile(UByteArrayAdapter(new std::vector<uint8_t>(1024*1024,0), true));
    }

	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}
	
	void testTrieEquality() {
		Static::GeneralizedTrie * stTriePtr = dynamic_cast<sserialize::Static::GeneralizedTrie*>( dynamic_cast<sserialize::Static::StringCompleter*>(stringCompleter().getPrivate())->priv().get());
		
		CPPUNIT_ASSERT( stTriePtr );
		
		CPPUNIT_ASSERT( m_trie.checkTrieEquality(m_config, *stTriePtr) );
	}
	
	void testIndexEquality() {
		Static::GeneralizedTrie * stTriePtr = dynamic_cast<sserialize::Static::GeneralizedTrie*>( dynamic_cast<sserialize::Static::StringCompleter*>(stringCompleter().getPrivate())->priv().get());
		
		CPPUNIT_ASSERT( stTriePtr );

		CPPUNIT_ASSERT( m_trie.checkIndexEquality(m_config, *stTriePtr, stringCompleter().getSupportedQuerries()) );
	}
	
	void testStringCompleterPrivateCast() {
		sserialize::Static::StringCompleter * p1 = dynamic_cast<sserialize::Static::StringCompleter*>(stringCompleter().getPrivate());
		Static::GeneralizedTrie * p = dynamic_cast<sserialize::Static::GeneralizedTrie*>(p1->priv().get());
 		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x4, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x5, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x6, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x7, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x8, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x9, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xA, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xB, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xC, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xD, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xE, Static::TrieNode::T_SIMPLE>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xF, Static::TrieNode::T_SIMPLE>::suite() );
	
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x4, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x5, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x6, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x7, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x8, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x9, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xA, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xB, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xC, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xD, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xE, Static::TrieNode::T_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xF, Static::TrieNode::T_COMPACT>::suite() );
	
	runner.addTest( StaticGeneralizedTrieTest<0x0, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x4, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x5, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x6, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x7, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x8, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x9, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xA, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xB, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xC, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xD, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xE, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0xF, Static::TrieNode::T_LARGE_COMPACT>::suite() );
	runner.run();
	return 0;
}
