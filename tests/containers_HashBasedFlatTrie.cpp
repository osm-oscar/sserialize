#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/containers/UnicodeTrie.h>
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/utility/printers.h>
#include <sserialize/storage/Size.h>
#include <sstream>
#include "TestBase.h"

const char * inFileName = 0;

class TestHashBasedFlatTrieBase: public sserialize::tests::TestBase {
public:
	//setup hft with strings of testString() function
	void setUp() override;
public:
	void testTrieEquality();
	void testFlatCorrect();
	//walks the trie in-order and checks for correctness of the Node implementation
	void testNode();
	void testParentChildRelation();
	void testSerialization();
	void testParallelSerialization();
	void testStaticNode();
	void testStaticSearch();
protected:
	using ValueType = sserialize::Size;
	using MyT = sserialize::HashBasedFlatTrie<ValueType>;
	using MyST = sserialize::Static::UnicodeTrie::FlatTrie<ValueType>;
protected:
	//Reference has to be valid until a call with a different @param pos
	virtual std::string const & testString(std::size_t pos) = 0;
	virtual std::size_t numTestStrings() const = 0;
	
	//Reference has to be valid until a call with a different @param pos
	virtual std::string const & checkString(std::size_t pos) = 0;
	virtual std::size_t numCheckStrings() const = 0;
protected:
	MyT m_ht;
};

class TestHashBasedFlatTrieBaseWithStrings: public TestHashBasedFlatTrieBase {
protected:
	std::string const & testString(std::size_t pos) override {
		return m_testStrings.at(pos);
	}
	std::size_t numTestStrings() const override {
		return m_testStrings.size();
	}
	std::string const & checkString(std::size_t pos) override {
		return m_checkStrings.at(pos);
	}
	std::size_t numCheckStrings() const override {
		return m_checkStrings.size();
	}
protected:
	std::vector<std::string> m_testStrings;
	std::vector<std::string> m_checkStrings;
};

class TestHashBasedFlatTrieSimple: public TestHashBasedFlatTrieBaseWithStrings {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrieSimple );
CPPUNIT_TEST( testFlatCorrect );
CPPUNIT_TEST( testNode );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testStaticNode );
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testStaticSearch );
// CPPUNIT_TEST( testParentChildRelation );
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

class TestHashBasedFlatTrieLarge: public TestHashBasedFlatTrieBase {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrieLarge );
CPPUNIT_TEST( testFlatCorrect );
CPPUNIT_TEST( testNode );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testStaticNode );
// CPPUNIT_TEST( testTrieEquality ); //Trie would be too large
CPPUNIT_TEST( testStaticSearch );
// CPPUNIT_TEST( testParentChildRelation );
CPPUNIT_TEST_SUITE_END();
public:
	TestHashBasedFlatTrieLarge() {
		m_testString.resize(uint64_t(std::numeric_limits<uint32_t>::max())+4096);
		for(uint64_t i(0); i < m_testString.size(); ++i) {
			m_testString.at(i) = '0'+i%10;
		}
	}
public:
	void setUp() override {
	auto sstr = m_ht.insert(m_testString);
	for(std::size_t i(0), s(m_testString.size()); i < s; ++i) {
		m_ht[sstr] = i;
		sstr = sstr.addOffset(1);
	}
	m_ht.finalize();
}
protected:
	std::string const & testString(std::size_t pos) override {
		return m_testString;
	}
	std::size_t numTestStrings() const override {
		return 1;
	}
	std::string const & checkString(std::size_t pos) override {
		return m_testString;
	}
	std::size_t numCheckStrings() const override {
		return 0;
	}
protected:
	std::string m_testString;
	std::size_t m_lastTestString{std::numeric_limits<std::size_t>::max()};
	
	std::string m_checkString;
	std::size_t m_lastCheckString{std::numeric_limits<std::size_t>::max()};
};

class TestHashBasedFlatTrieFile: public TestHashBasedFlatTrieBaseWithStrings {
CPPUNIT_TEST_SUITE( TestHashBasedFlatTrieFile );
CPPUNIT_TEST( testTrieEquality );
CPPUNIT_TEST( testNode );
CPPUNIT_TEST( testSerialization );
CPPUNIT_TEST( testParallelSerialization );
CPPUNIT_TEST( testStaticNode );
CPPUNIT_TEST( testStaticSearch );
CPPUNIT_TEST( testSpecialStaticSearch );
// CPPUNIT_TEST( testParentChildRelation );
CPPUNIT_TEST_SUITE_END();
public:
	TestHashBasedFlatTrieFile() {
		if (!inFileName) {
			return;
		}
		uint64_t newLineCount = 0;
		sserialize::MmappedMemory<char> inFile(inFileName);
		const char * data = inFile.data();
		const char * dataEnd = data+inFile.size();
		for(const char * it = data; it < dataEnd; ++it) {
			if (*it == '\n') {
				++newLineCount;
			}
		}
		std::cout << "Importing " << newLineCount << " strings from file" << std::endl;
		m_testStrings.reserve(newLineCount);
		const char * prev = data;
		for(const char * it = data; it < dataEnd; ++it) {
			if (*it == '\n') {
				m_testStrings.push_back( std::string(prev, it) );
				prev = it+1;
			}
		}
		sserialize::mt_sort(m_testStrings.begin(), m_testStrings.end(), std::less<std::string>());
		m_testStrings.resize(std::unique(m_testStrings.begin(), m_testStrings.end())-m_testStrings.begin());
		std::random_shuffle(m_testStrings.begin(), m_testStrings.end());
		std::cout << "Imported " << m_testStrings.size() << " unique strings from file" << std::endl;
	}

	void testSpecialStaticSearch() {
		if (!m_testStrings.size())
			return;

		sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
		m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); }, std::thread::hardware_concurrency());
		MyST sft(hftOut);
		std::vector< std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator> > nodeIts;
		
		for(uint32_t i(0), s((uint32_t) m_testStrings.size()); i < s; ++i) {
			const std::string & str = m_testStrings[i];
			uint32_t pos = sft.find(str, false);
			uint32_t v = sft.at(pos);
			if (i != v) {
				std::string sstr = sft.strAt(pos);
			}
			CPPUNIT_ASSERT_EQUAL_MESSAGE("search broken for" + str, ValueType(i), sft.at(str, false));
		}
		
// 		std::string str = "johnson";
// 		uint32_t testPos = sft.find("johnson", false);
// 		CPPUNIT_ASSERT_EQUAL_MESSAGE("search broken for " + str, str, sft.strAt(testPos));
	}
};

//BEGIN Implementation of TestHashBasedFlatTrie::Base

void
TestHashBasedFlatTrieBase::setUp() {
	for(std::size_t i(0), s(numTestStrings()); i < s; ++i) {
		m_ht[m_ht.insert(testString(i))] = i; 
	}
	m_ht.finalize();
}

void
TestHashBasedFlatTrieBase::testTrieEquality() {
	typedef sserialize::UnicodeTrie::Trie<ValueType> TrieTrie;
	struct TrieNodeState {
		std::string str;
		ValueType v;
		TrieTrie::Node::const_iterator childrenIt;
		TrieTrie::Node::const_iterator childrenEnd;
		TrieNodeState(const std::string & str, ValueType v, const TrieTrie::Node::const_iterator & childrenBegin, const TrieTrie::Node::const_iterator & childrenEnd) :
		str(str), v(v), childrenIt(childrenBegin), childrenEnd(childrenEnd) {}
	};

	TrieTrie trie;
	for(std::size_t i(0), s(numTestStrings()); i < s; ++i) {
		trie.at(testString(i).begin(), testString(i).end()) = i;
	}
	if (!numTestStrings())
		return;
	std::vector< std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator> > hNodeIts;
	std::vector<TrieNodeState> tNodeIts;
	MyT::NodePtr hnode = m_ht.root();
	TrieTrie::NodePtr tnode = trie.root();
	CPPUNIT_ASSERT_EQUAL_MESSAGE("root node", tnode->str(), m_ht.toStr(hnode->str()));
	hNodeIts.push_back( std::pair<MyT::Node::const_iterator, MyT::Node::const_iterator>(hnode->begin(), hnode->end()) );
	tNodeIts.push_back(TrieNodeState(tnode->str(), tnode->value(), tnode->cbegin(), tnode->cend()));
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
		tNodeIts.push_back( TrieNodeState(tNodeIts.back().str+tnode->str(), tnode->value(), tnode->cbegin(), tnode->cend()) );
		//check the node
		CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", tNodeIts.back().str , m_ht.toStr(hnode->str()));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("node value at " + tNodeIts.back().str, tNodeIts.back().v, hnode->value());
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("node count", tNodeIts.size(), hNodeIts.size());
}

void
TestHashBasedFlatTrieBase::testFlatCorrect() {
	if (!numCheckStrings()) {
		return;
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("size", numCheckStrings(), (std::size_t) m_ht.size());
	MyT::const_iterator tIt(m_ht.cbegin());
	for(std::size_t i(0), s(numCheckStrings()); i < s; ++i, ++tIt) {
		CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", checkString(i), m_ht.toStr(tIt->first));
	}
}

//walks the trie in-order and checks for correctness of the Node implementation
void
TestHashBasedFlatTrieBase::testNode() {
	if (!numTestStrings())
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

void
TestHashBasedFlatTrieBase::testParentChildRelation() {
	CPPUNIT_ASSERT_MESSAGE("Root node has a parent", m_ht.root()->parent());
	auto childParentChecker = [](const typename MyT::Node & n) {
		for(typename MyT::Node::const_iterator it(n.begin()), end(n.end()); it != end; ++it) {
			typename MyT::NodePtr parent = (*it)->parent();
			CPPUNIT_ASSERT_MESSAGE("child->parent != parent", parent != MyT::make_nodeptr(n));
		}
	};
	m_ht.root()->apply(childParentChecker);
}

void
TestHashBasedFlatTrieBase::testSerialization() {
	sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
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

void
TestHashBasedFlatTrieBase::testParallelSerialization() {
	sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); }, std::thread::hardware_concurrency());
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

void
TestHashBasedFlatTrieBase::testStaticNode() {
	if (!numTestStrings())
		return;

	sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
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
		CPPUNIT_ASSERT_MESSAGE("too many nodes", count < numTestStrings());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("node string", m_ht.toStr((m_ht.begin()+count)->first), node.str());
		++count;
	}
}

void
TestHashBasedFlatTrieBase::testStaticSearch() {
	if (!numTestStrings())
		return;

	sserialize::UByteArrayAdapter hftOut(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	m_ht.append(hftOut, [](const MyT::NodePtr & n) { return n->value(); });
	MyST sft(hftOut);
	std::vector< std::pair<MyST::Node::const_iterator, MyST::Node::const_iterator> > nodeIts;
	
	for(std::size_t i(0), s(numTestStrings()); i < s; ++i) {
		const std::string & str = testString(i);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("search broken for" + str, ValueType(i), sft.at(str, false));
	}
}
//END Implementation of TestHashBasedFlatTrieBase

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	
	srand( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(  TestHashBasedFlatTrieSimple::suite() );
	runner.addTest( TestHashBasedFlatTrieLarge::suite() );
	if (argc > 1 && sserialize::MmappedFile::fileExists(argv[1])) {
		inFileName = argv[1];
		runner.addTest( TestHashBasedFlatTrieFile::suite() );
	}
	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
}
