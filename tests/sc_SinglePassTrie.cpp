#include <sserialize/containers/GeneralizedTrie/SinglePassTrie.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"

using namespace sserialize;

enum { BO_TREE_CASE_SENSITIVE=0x1, BO_SUFFIX_TREE=0x2, BO_COMPACT_NODE=0x4};

template<uint32_t T_BUILD_OPTS>
class StaticGeneralizedTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticGeneralizedTrieTest );
// CPPUNIT_TEST( testCreateStringCompleter );
// CPPUNIT_TEST( testSupportedQuerries );
// CPPUNIT_TEST( testCompletionECS );
// CPPUNIT_TEST( testCompletionECI );
// CPPUNIT_TEST( testCompletionPCS );
// CPPUNIT_TEST( testCompletionPCI );
// CPPUNIT_TEST( testCompletionSCS );
// CPPUNIT_TEST( testCompletionSCI );
// CPPUNIT_TEST( testCompletionSPCS );
// CPPUNIT_TEST( testCompletionSPCI );
// CPPUNIT_TEST( testStringCompleterPrivateCast );
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testIndexEquality );
CPPUNIT_TEST_SUITE_END();
private: //builds opts;
	bool m_caseSensitive;
	bool m_suffixTrie;
	bool m_mergeIndex;
	bool m_noFullIndexAtAll;
private:
	GeneralizedTrie::SinglePassTrie m_trie;
	std::deque<uint8_t> m_stTrieList;
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig m_config;
	StringCompleter m_strCompleter;
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
		
		if (BO_COMPACT_NODE)
			m_config.nodeType = Static::TrieNode::T_COMPACT;
		else
			m_config.nodeType = Static::TrieNode::T_SIMPLE;
		
		m_trie.createStaticTrie(m_config);
		
		
		m_config.indexFactory.flush();
		UByteArrayAdapter idxAdap(m_config.indexFactory.getFlushedData());
		sserialize::Static::ItemIndexStore idxStore(idxAdap);

		UByteArrayAdapter trieAdap(&m_stTrieList);
		trieAdap += STATIC_STRING_COMPLETER_HEADER_SIZE; //skip header
		sserialize::Static::GeneralizedTrie * p = new sserialize::Static::GeneralizedTrie(trieAdap, idxStore);
		stringCompleter() = StringCompleter( p );
		
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
		m_config.indexFactory.setIndexFile(UByteArrayAdapter(new std::vector<uint8_t>(1024*1024,0), true));
    }

	virtual void setUp() {
		setUpCompleter();
	}
	virtual void tearDown() {}
	
	void testTrieEquality() {
		Static::GeneralizedTrie * stTriePtr = dynamic_cast<sserialize::Static::GeneralizedTrie*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( stTriePtr );
		
		CPPUNIT_ASSERT( m_trie.checkTrieEquality(m_config, *stTriePtr) );
	}
	
	void testIndexEquality() {
		Static::GeneralizedTrie * stTriePtr = dynamic_cast<sserialize::Static::GeneralizedTrie*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( stTriePtr );

		CPPUNIT_ASSERT_MESSAGE("Exact indices", m_trie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_EXACT));
		if (stringCompleter().getSupportedQuerries() & StringCompleter::SQ_SUFFIX)
			CPPUNIT_ASSERT_MESSAGE("Suffix indices", m_trie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_SUFFIX));

		CPPUNIT_ASSERT_MESSAGE("Prefix indices", m_trie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_PREFIX));
		if (stringCompleter().getSupportedQuerries() & StringCompleter::SQ_SUFFIX_PREFIX)
			CPPUNIT_ASSERT_MESSAGE("Substring indices", m_trie.checkIndexEquality(m_config, *stTriePtr, StringCompleter::SQ_SUFFIX_PREFIX));
		
	}
	
	void testStringCompleterPrivateCast() {
		Static::GeneralizedTrie * p = dynamic_cast<sserialize::Static::GeneralizedTrie*>(stringCompleter().getPrivate());
		
		CPPUNIT_ASSERT( p != 0 );
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticGeneralizedTrieTest<0x0>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x1>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x2>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x3>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x4>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x5>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x6>::suite() );
	runner.addTest( StaticGeneralizedTrieTest<0x7>::suite() );
	runner.run();
	return 0;
}
