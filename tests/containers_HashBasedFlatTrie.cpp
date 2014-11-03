#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>
#include <cppunit/TestResult.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/containers/UnicodeTrie.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/utility/printers.h>

const char * inFileName = 0;

class TestHashBasedFlatTrieBase: public CppUnit::TestFixture {
protected:
	typedef sserialize::HashBasedFlatTrie<uint32_t> MyT;
	typedef sserialize::Static::UnicodeTrie::FlatTrie<uint32_t> MyST;
	MyT m_ht;
	std::vector<std::string> m_testStrings;
	std::vector<std::string> m_checkStrings;
public:
	//setup hft with strings in m_testStrings
	virtual void setUp() {
		for(std::size_t i(0), s(m_testStrings.size()); i < s; ++i) {
			m_ht[m_ht.insert(m_testStrings[i])] = i; 
		}
		m_ht.finalize();
	}
	
	void testTrieEquality() {
		typedef sserialize::UnicodeTrie::Trie<uint32_t> TrieTrie;
		struct TrieNodeState {
			std::string str;
			TrieTrie::Node::const_iterator childrenIt;
			TrieTrie::Node::const_iterator childrenEnd;
			TrieNodeState(const std::string & str, const TrieTrie::Node::const_iterator & childrenBegin, const TrieTrie::Node::const_iterator & childrenEnd) :
			str(str), childrenIt(childrenBegin), childrenEnd(childrenEnd) {}
		};

		TrieTrie trie;
		for(const std::string & str : m_testStrings) {
			trie.insert(str.cbegin(), str.cend());
		}
		if (!m_testStrings.size())
			return;
		std::vector< std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator> > hNodeIts;
		std::vector<TrieNodeState> tNodeIts;
		MyT::NodePtr hnode = m_ht.root();
		TrieTrie::NodePtr tnode = trie.root();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("root node", tnode->str(), m_ht.toStr(hnode->str()));
		hNodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(hnode->begin(), hnode->end()) );
		tNodeIts.push_back(TrieNodeState(tnode->str(), tnode->cbegin(), tnode->cend()));
		while(hNodeIts.size() && tNodeIts.size()) {
			//ascend
			if (tNodeIts.back().childrenIt == tNodeIts.back().childrenEnd) {
				CPPUNIT_ASSERT_MESSAGE("hash node iterators should be equal for ascend", hNodeIts.back().first == hNodeIts.back().second);
				while (tNodeIts.size() && hNodeIts.size() && tNodeIts.back().childrenIt == tNodeIts.back().childrenEnd) {
					CPPUNIT_ASSERT_MESSAGE("hash node iterators should be equal for ascend", hNodeIts.back().first == hNodeIts.back().second);
					hNodeIts.pop_back();
					tNodeIts.pop_back();
				}
				continue;
			}
			CPPUNIT_ASSERT_MESSAGE("hash node iterators should be unequal", hNodeIts.back().first != hNodeIts.back().second);
			MyT::Node::const_iterator & hNIt = hNodeIts.back().first;
			TrieTrie::Node::const_iterator & tNIt = tNodeIts.back().childrenIt;
			hnode = *hNIt;
			tnode = tNIt->second;
			++hNIt;
			++tNIt;
			//insert children
			hNodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(hnode->begin(), hnode->end()) );
			tNodeIts.push_back( TrieNodeState(tNodeIts.back().str+tnode->str(), tnode->cbegin(), tnode->cend()) );
			//check the node
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string",tNodeIts.back().str , m_ht.toStr(hnode->str()));
		}
		CPPUNIT_ASSERT_EQUAL_MESSAGE("node count", tNodeIts.size(), hNodeIts.size());
	}
	
	void testFlatCorrect() {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", (uint32_t) m_checkStrings.size(), m_ht.size());
		MyT::const_iterator tIt(m_ht.cbegin()), tEnd(m_ht.cend());
		std::vector<std::string>::const_iterator cIt(m_checkStrings.cbegin());
		for(; tIt != tEnd; ++tIt, ++cIt) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", *cIt, m_ht.toStr(tIt->first));
		}
	}
	
	//walks the trie in-order and checks for correctness of the Node implementation
	void testNode() {
		if (!m_testStrings.size())
			return;
		std::vector< std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator> > nodeIts;
		uint32_t count = 0;
		MyT::NodePtr node = m_ht.root();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("root node", m_ht.toStr(m_ht.begin()->first), m_ht.toStr(node->str()));
		++count;
		nodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(node->begin(), node->end()) );
		while(nodeIts.size()) {
			//descend upwards if need be
			if (nodeIts.back().first == nodeIts.back().second) {
				while (nodeIts.size() && nodeIts.back().first == nodeIts.back().second) {
					nodeIts.pop_back();
				}
				continue;
			}
			MyT::Node::const_iterator & nIt = nodeIts.back().first;
			node = *nIt;
			++nIt;
			//insert children
			nodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(node->begin(), node->end()) );
			//check the node
			CPPUNIT_ASSERT_MESSAGE("too many nodes", count < m_ht.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", m_ht.toStr((m_ht.begin()+count)->first), m_ht.toStr(node->str()));
			++count;
		}
	}
	
	void testSerialization() {
		sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, false));
		m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); });
		MyST sft(hftOut);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("size", m_ht.size(), sft.size());
		MyT::const_iterator rIt(m_ht.begin()), rEnd(m_ht.end());
		uint32_t sI(0), sS(sft.size());
		for(; sI < sS && rIt != rEnd; ++sI, ++rIt) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("string at", sI), m_ht.toStr(rIt->first), sft.strAt(sI));
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sserialize::toString("payload at", sI), rIt->second, sft.at(sI));
		}
		CPPUNIT_ASSERT_MESSAGE("real iterator not at the end", rIt == rEnd);
		CPPUNIT_ASSERT_MESSAGE("static iterator not at the end", sI == sS);
	}
	
	void testStaticNode() {
		if (!m_testStrings.size())
			return;

		sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, false));
		m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); });
		MyST sft(hftOut);
		std::vector< std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator> > nodeIts;
		uint32_t count = 0;
		MyST::Node node = sft.root();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("root node", m_ht.toStr(m_ht.begin()->first), node.str());
		++count;
		nodeIts.push_back( std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator>(node.begin(), node.end()) );
		while(nodeIts.size()) {
			//descend upwards if need be
			if (nodeIts.back().first == nodeIts.back().second) {
				while (nodeIts.size() && nodeIts.back().first == nodeIts.back().second) {
					nodeIts.pop_back();
				}
				continue;
			}
			MyST::Node::const_iterator & nIt = nodeIts.back().first;
			node = *nIt;
			++nIt;
			//insert children
			nodeIts.push_back( std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator>(node.begin(), node.end()) );
			//check the node
			CPPUNIT_ASSERT_MESSAGE("too many nodes", count < m_testStrings.size());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", m_ht.toStr((m_ht.begin()+count)->first), node.str());
			++count;
		}
	}
	
	void testStaticSearch() {
		if (!m_testStrings.size())
			return;

		sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, false));
		m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); });
		MyST sft(hftOut);
		std::vector< std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator> > nodeIts;
		
		for(uint32_t i(0), s(m_testStrings.size()); i < s; ++i) {
			const std::string & str = m_testStrings[i];
			CPPUNIT_ASSERT_EQUAL_MESSAGE("broken for" + str, i, sft.at(str, false));
		}

	}
	
};

class TestHashBasedFlatTrieSimple: public TestHashBasedFlatTrieBase {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrieSimple );
CPPUNIT_TEST( testFlatCorrect );
CPPUNIT_TEST( testNode );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testStaticNode );
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testStaticSearch );
CPPUNIT_TEST_SUITE_END();
public:
	TestHashBasedFlatTrieSimple() {
		m_testStrings = {//all: missing empty parent
			"A", //single
			"BB", "BC", "BD", //missing parent
			"C", "CD", "CE", //parent available with children 
			"DAAAA", "DAAAB", "DAAABE", "DAAAC", //longer node string, but missing parent
			"EAAAA", "EAAAAA", "EAAAAB", "EAAAAC", //longer node string, no missing parent
			"FF", "FEG", "FEHI", "FEHJ" //multiple missing parents
		};
		m_checkStrings = {
			"",
			"A", //single
			"B",
			"BB", "BC", "BD", //missing parent
			"C", "CD", "CE", //parent available with children
			"DAAA",
			"DAAAA", "DAAAB", "DAAABE", "DAAAC", //longer node string, but missing parent
			"EAAAA", "EAAAAA", "EAAAAB", "EAAAAC", //longer node string, no missing parent
			"F", "FE",
			"FEG",
			"FEH", "FEHI", "FEHJ",
			"FF" //multiple missing parents
		};
		std::random_shuffle(m_testStrings.begin(), m_testStrings.end());
	}
};


class TestHashBasedFlatTrieFile: public TestHashBasedFlatTrieBase {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrieFile );
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testNode );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testStaticNode );
CPPUNIT_TEST( testStaticSearch );
CPPUNIT_TEST_SUITE_END();
public:
	TestHashBasedFlatTrieFile() {
		if (!inFileName) {
			return;
		}
		std::ifstream inFile;
		inFile.open(inFileName);
		while(!inFile.eof()) {
			std::string str;
			std::getline(inFile, str);
			m_testStrings.push_back(str);
		}
		std::random_shuffle(m_testStrings.begin(), m_testStrings.end());
		
	}
};

int main(int argc, const char ** argv) {
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestHashBasedFlatTrieSimple::suite() );
	if (argc > 1) {
		inFileName = argv[1];
		runner.addTest( TestHashBasedFlatTrieFile::suite() );
	}
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}