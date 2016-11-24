#include <iostream>
#include <sstream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>

#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>
#include "TestBase.h"

//uint8_t charWidth, bool withSubTries, const std::string & str, const std::deque<uint16_t> & childChars, bool fullIndex, uint32_t indexPtr, std::deque<uint8_t> & destination

using namespace sserialize;
using namespace sserialize::Static;

struct NodeConfig {
	NodeConfig() :
		charWidth(0), indexType(TrieNodePrivate::IT_NONE),
		childCount(0),
		exactIndexPtr(0), prefixIndexPtr(0), suffixIndexPtr(0), suffixPrefixIndexPtr(0),
		nodeSize(0), nodeOffSet(0) {}
	//Creation config
	uint8_t charWidth;
	sserialize::Static::TrieNodePrivate::IndexTypes indexType;


	//Filled during creation
	std::string nodeStr;
	uint32_t childCount;
	std::vector<uint32_t> childChars;
	std::vector<uint32_t> childOffSets;
	uint32_t exactIndexPtr;
	uint32_t prefixIndexPtr;
	uint32_t suffixIndexPtr;
	uint32_t suffixPrefixIndexPtr;

	//raw node data
	std::deque<uint8_t> rawNodeData;
	uint32_t nodeSize;
	uint32_t nodeOffSet;

	Static::TrieNodeCreationInfo creationNodeInfo() {
		TrieNodeCreationInfo info;
		info.childChars = childChars;
		info.childPtrs = childOffSets;
		info.exactIndexPtr = exactIndexPtr;
		info.indexTypes = indexType;
		info.nodeStr = nodeStr;
		info.prefixIndexPtr = prefixIndexPtr;
		info.suffixIndexPtr = suffixIndexPtr;
		info.suffixPrefixIndexPtr = suffixPrefixIndexPtr;
		return info;
	}

};

std::string baseStringChars("ABCDEFGHIJKLMOPQRSTUVWXYZabcdevghijklmnopqrstuvxyz1234567890!$%&/{}()[]+-*~<>");

template<typename T_STATIC_TRIE_NODE_CREATOR, typename T_STATIC_TRIE_NODE, uint8_t T_CHAR_WIDTH, int T_INDEX_TYPE>
class StaticTrieNodeTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( StaticTrieNodeTest );
CPPUNIT_TEST( testChildChars );
CPPUNIT_TEST( testExactIndex );
CPPUNIT_TEST( testPrefixIndex );
CPPUNIT_TEST( testSuffixIndex );
CPPUNIT_TEST( testSubStrIndex );
CPPUNIT_TEST( testChildCount );
CPPUNIT_TEST( testStorageSize );
CPPUNIT_TEST( testCummulatedStorageSize );
CPPUNIT_TEST( testNodeStrSize );
CPPUNIT_TEST( testIndexType );
CPPUNIT_TEST( testNodeStr );
CPPUNIT_TEST( testChildrenChar );
CPPUNIT_TEST( testChildrenOffsets );
CPPUNIT_TEST_SUITE_END();
private:
	NodeConfig m_config;
	T_STATIC_TRIE_NODE * m_node;
private:
	void createNodeBottomUp() {
		NodeConfig & config = m_config;
		uint8_t strLen = (double)rand()/RAND_MAX * 255;
		for(uint8_t i = 0; i < strLen; i++) {
			char c = baseStringChars.at((double)rand()/RAND_MAX * baseStringChars.size());
			config.nodeStr.push_back(c);
		}

		config.childCount = (static_cast<uint32_t>((double)rand()/RAND_MAX * 2048)) & 0x1FF;
		if (config.childCount) {
			config.childOffSets.push_back(0);
			for(uint16_t i = 1; i < config.childCount; i++) {
				uint32_t addOffSet = (double)rand()/RAND_MAX * 0x1FF;
				config.childOffSets.push_back(config.childOffSets.back()+addOffSet);
			}
			for(uint32_t i = 1; i < config.childCount; i++) {
				if (config.charWidth == 1) {
					config.childChars.push_back((double)rand()/RAND_MAX * 0xFF);
				}
				else if (config.charWidth == 2) {
					config.childChars.push_back((double)rand()/RAND_MAX * 0xFFFF);
				}
				else if (config.charWidth == 3) {
					config.childChars.push_back((double)rand()/RAND_MAX * 0xFFFFFF);
				}
				else if (config.charWidth == 4) {
					config.childChars.push_back((double)rand()/RAND_MAX * 0xFFFFFFFF);
				}
				else {
					std::cout << "Tester: Unsupported charWidth: " << static_cast<uint32_t>( config.charWidth ) << std::endl;
				}
			}
			config.childChars.push_back( static_cast<uint32_t>(0xFE) << ((config.charWidth-1)*8) );
			std::sort(config.childChars.begin(), config.childChars.end());
		}
		
		config.exactIndexPtr = rand() + rand();
		config.prefixIndexPtr = rand() + rand();
		config.suffixIndexPtr = rand() + rand();
		config.suffixPrefixIndexPtr = rand() + rand();
		config.nodeOffSet = rand() & 0xFFF;
		std::deque<uint8_t> nodeData(config.nodeOffSet, 0xFE);
		

		//uint8_t charWidth, const std::string& nodeString, const std::deque< uint16_t >& childChars, const std::deque< uint32_t >& childPtrs, bool fullIndex, uint32_t indexPtr, std::deque< uint8_t >& destination
		unsigned int err = T_STATIC_TRIE_NODE_CREATOR::prependNewNode(config.creationNodeInfo(), nodeData);
		
		
		if (T_STATIC_TRIE_NODE_CREATOR::isError(err)) {
			throw sserialize::CreationException(T_STATIC_TRIE_NODE_CREATOR::errorString(err));
		}


		narrow_check_assign(config.nodeSize) = nodeData.size()-config.nodeOffSet;
		prependToDeque(std::deque<uint8_t>(config.nodeOffSet, 0xFE), nodeData);

		config.rawNodeData.swap(nodeData);
	}
public:
	virtual void setUp() {
		m_config.charWidth = T_CHAR_WIDTH;
		m_config.indexType = (sserialize::Static::TrieNodePrivate::IndexTypes) T_INDEX_TYPE;
		
		createNodeBottomUp();

		UByteArrayAdapter adap(&m_config.rawNodeData);
		m_node = new T_STATIC_TRIE_NODE(adap + m_config.nodeOffSet);
	}
	virtual void tearDown() override {
		delete m_node;
	}
public:
	
	void testChildChars() {
		if (m_config.childChars.size()) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("charWidth", (uint32_t) m_config.charWidth, (uint32_t) m_node->charWidth());
		}
	}
	
	void testExactIndex() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("exactIndex", (bool) (m_config.indexType & TrieNodePrivate::IT_EXACT), m_node->hasExactIndex());
	}

	void testPrefixIndex() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("prefixIndex", (bool) (m_config.indexType & TrieNodePrivate::IT_PREFIX), m_node->hasPrefixIndex());
	}

	void testSuffixIndex() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixIndex", (bool) (m_config.indexType & TrieNodePrivate::IT_SUFFIX), m_node->hasSuffixIndex());
	}

	void testSubStrIndex() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("subStrIndex", (bool) (m_config.indexType & TrieNodePrivate::IT_SUFFIX_PREFIX), m_node->hasSuffixPrefixIndex());
	}

	void testChildCount() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("childCount", m_config.childCount, m_node->childCount());
	}

	void testStorageSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("storageSize", m_config.nodeSize, (uint32_t) m_node->getStorageSize());
	}

	void testCummulatedStorageSize() {
		auto cummulatedStorageSize = m_node->getHeaderStorageSize() + m_node->getNodeStringStorageSize();
		cummulatedStorageSize += m_node->getChildPtrStorageSize() + m_node->getChildCharStorageSize();
		cummulatedStorageSize += m_node->getIndexPtrStorageSize();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("cummulated Storage Size", m_config.nodeSize, narrow_check<uint32_t>(cummulatedStorageSize));
	}

	void testNodeStrSize() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("nodestr size", (uint32_t) m_config.nodeStr.size(), (uint32_t) m_node->strLen());
	}
	
	void testIndexType() {
		if (m_config.indexType & TrieNodePrivate::IT_EXACT) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("exactIndexPtr", m_config.exactIndexPtr, m_node->getExactIndexPtr());
		}

		if (m_config.indexType & TrieNodePrivate::IT_PREFIX) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("prefixIndexPtr", m_config.prefixIndexPtr, m_node->getPrefixIndexPtr());
		}

		if (m_config.indexType & TrieNodePrivate::IT_SUFFIX) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixIndexPtr", m_config.suffixIndexPtr, m_node->getSuffixIndexPtr());
		}

		if (m_config.indexType & TrieNodePrivate::IT_SUFFIX_PREFIX) { 
			CPPUNIT_ASSERT_EQUAL_MESSAGE("suffixPrefixIndexPtr", m_config.suffixPrefixIndexPtr, m_node->getSuffixPrefixIndexPtr());
		}
	}

	void testNodeStr() {
		std::string nodeStr = m_node->str();
		for(uint8_t i = 0; i < m_node->strLen(); i++) {
			std::stringstream ss;
			ss << "Single node char at position " << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), m_config.nodeStr.at(i), nodeStr.at(i));
		}
	}

	void testChildrenChar() {
		for(uint32_t i = 0; i < m_node->childCount(); i++) {
			std::stringstream ss;
			ss << "Single child char at position " << i;
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), m_config.childChars.at(i), m_node->childCharAt(i));
		}
	}

	void testChildrenOffsets() {
		for(uint32_t i = 0; i < m_node->childCount(); i++) {
			std::stringstream ss;
			ss << "Single child ptr at position " << i;
			uint32_t realChildPtr = m_config.childOffSets.at(i);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), realChildPtr, m_node->getChildPtr(i));
		}
	}
};


#define ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, __INDEX_TYPE) \
	runner.addTest(  StaticTrieNodeTest<__CREATOR, __NODE, 1, __INDEX_TYPE>::suite() ); \
	runner.addTest(  StaticTrieNodeTest<__CREATOR, __NODE, 2, __INDEX_TYPE>::suite() ); \
	if (charWidth > 2) { \
		runner.addTest(  StaticTrieNodeTest<__CREATOR, __NODE, 3, __INDEX_TYPE>::suite() ); \
		runner.addTest(  StaticTrieNodeTest<__CREATOR, __NODE, 4, __INDEX_TYPE>::suite() ); \
	}
	
#define ADD_TEST(__CREATOR, __NODE) \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 0); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 1); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 2); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 3); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 4); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 5); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 6); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 7); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 8); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 9); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 10); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 11); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 12); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 13); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 14); \
	ADD_TEST_CHAR_WIDTH(__CREATOR, __NODE, 15);

int main(int argc, char ** argv) {
	srand( 0 );
	
	sserialize::tests::TestBase::init(argc, argv);
	
	CppUnit::TextUi::TestRunner runner;
	
	int charWidth = 4;
	
	ADD_TEST(LargeCompactTrieNodeCreator, LargeCompactTrieNodePrivate);
	
	charWidth = 2;
	ADD_TEST(SimpleStaticTrieCreationNode, SimpleTrieNodePrivate);
	
	charWidth = 2;
	ADD_TEST(CompactStaticTrieCreationNode, CompactTrieNodePrivate);
// 	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;	
}