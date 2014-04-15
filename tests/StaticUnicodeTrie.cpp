#include <sserialize/containers/UnicodeTrie.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/UnicodeTrie/Trie.h>
#include <sserialize/Static/UnicodeTrie/detail/SimpleNode.h>
#include <sserialize/utility/stringfunctions.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"
#include <cppunit/TestResult.h>

using namespace sserialize;

struct TriePayload {
	TriePayload() : exact(0), prefix(0), suffix(0), substr(0) {}
	TriePayload(UByteArrayAdapter d) {
		d.resetPtrs();
		d >> exact >> prefix >> suffix >> substr;
	}
	uint32_t exact;
	uint32_t prefix;
	uint32_t suffix;
	uint32_t substr;
};

UByteArrayAdapter & operator<<(UByteArrayAdapter & d, const TriePayload & src) {
	return d << src.exact << src.prefix << src.suffix << src.substr;
}
UByteArrayAdapter & operator>>(UByteArrayAdapter & d, TriePayload & src) {
	return d >> src.exact >> src.prefix >> src.suffix >> src.substr;
}

class StaticUnicodeTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticUnicodeTrieTest );
CPPUNIT_TEST( testCreateStringCompleter );
CPPUNIT_TEST( testSupportedQuerries );
CPPUNIT_TEST( testCompletionECS );
// CPPUNIT_TEST( testCompletionECI );
CPPUNIT_TEST( testCompletionPCS );
// CPPUNIT_TEST( testCompletionPCI );
CPPUNIT_TEST( testCompletionSCS );
// CPPUNIT_TEST( testCompletionSCI );
CPPUNIT_TEST( testCompletionSPCS );
// CPPUNIT_TEST( testCompletionSPCI );
CPPUNIT_TEST( testTrieEquality );
// CPPUNIT_TEST( testIndexEquality );
CPPUNIT_TEST_SUITE_END();
private:
	struct TrieStorageData {
		std::vector<uint32_t> exact;
		std::vector<uint32_t> suffix;
	};
	
	struct TrieNodeSerializationData {
		std::vector<uint32_t> prefix;
		std::vector<uint32_t> substr;
		void merge(const std::vector<uint32_t> & prefix, const std::vector<uint32_t> & substr) {
			mergeSortedContainer(this->prefix, this->prefix, prefix);
			mergeSortedContainer(this->substr, this->substr, substr);
		}
	};
	
	typedef UnicodeTrie::Trie<TrieStorageData> MyTrie;
	typedef Static::UnicodeTrie::Trie<TriePayload> MyStaticTrie;
	

	
	struct PayloadHandler {
		PayloadHandler(std::shared_ptr<sserialize::ItemIndexFactory> store) :
		store(store),
		nodeData(new std::unordered_map<MyTrie::Node*, TrieNodeSerializationData >() )
		{}
		std::shared_ptr<sserialize::ItemIndexFactory> store;
		std::shared_ptr<std::unordered_map<MyTrie::Node*, TrieNodeSerializationData > > nodeData;
		TriePayload operator()(MyTrie::Node * node) {
			TriePayload tp;
			tp.exact = store->addIndex(node->value().exact);
			tp.suffix = store->addIndex(node->value().suffix);
			if (nodeData->count(node)) {
				TrieNodeSerializationData & nd = (*nodeData)[node];
				nd.merge(node->value().exact, node->value().suffix);
				tp.prefix = store->addIndex(nd.prefix);
				tp.substr = store->addIndex(nd.substr);
				if (node->parent())
					(*nodeData)[node->parent()].merge(nd.prefix, nd.substr);
			}
			else {
				tp.prefix = tp.exact;
				tp.substr = tp.suffix;
				if (node->parent()) {
					(*nodeData)[node->parent()].merge(node->value().exact, node->value().suffix);
				}
			}
			nodeData->erase(node);
			return tp;
		}
	};
	
	class MyStringCompleterPrivate: public StringCompleterPrivate {
	private:
		MyStaticTrie m_sTrie;
		Static::ItemIndexStore m_store;
	public:
		MyStringCompleterPrivate(const MyStaticTrie & trie, Static::ItemIndexStore & store) : m_sTrie(trie), m_store(store) {}
		virtual ~MyStringCompleterPrivate() {}
		virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
			TriePayload rd;
			try {
				rd = m_sTrie.at(str, (qtype & StringCompleter::QT_PREFIX || qtype & StringCompleter::QT_SUBSTRING));
			}
			catch (const OutOfBoundsException & c) {
				return ItemIndex();
			}
			if (qtype & StringCompleter::QT_SUBSTRING) {
				return m_store.at(rd.substr);
			}
			if (qtype & StringCompleter::QT_PREFIX) {
				return m_store.at(rd.prefix);
			}
			if (qtype & StringCompleter::QT_SUFFIX) {
				return m_store.at(rd.suffix);
			}
			if (qtype & StringCompleter::QT_EXACT) {
				return m_store.at(rd.exact);
			}
			return ItemIndex();
		}
		virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
			return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EPSP | StringCompleter::SQ_SSP | StringCompleter::SQ_CASE_SENSITIVE);
		}
		virtual std::string getName() const {
			return std::string("MyStringCompleterPrivate");
		}
	};
	
private:
	sserialize::Static::ItemIndexStore m_indexStore;
	MyStaticTrie m_sTrie;
	MyTrie m_trie;
private:
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }

protected:
	bool setUpCompleter() {
		std::shared_ptr<sserialize::ItemIndexFactory> indexFactory(new sserialize::ItemIndexFactory());
		indexFactory->setIndexFile(UByteArrayAdapter(new std::vector<uint8_t>(1024*1024,0), true));

		UByteArrayAdapter trieData(new std::vector<uint8_t>(), true);
		PayloadHandler phandler(indexFactory);
		std::shared_ptr<sserialize::Static::UnicodeTrie::NodeCreator> nodeCreator( new Static::UnicodeTrie::detail::SimpleNodeCreator() );
		std::unordered_set<uint32_t> separators;
		
		for(uint32_t i(0), s(items().size()); i < s; ++i) {
			const TestItemData & item = items()[i];
			for(const std::string & itemStr : item.strs) {
				m_trie.at(itemStr.cbegin(), itemStr.cend()).exact.push_back(i);
				for(std::string::const_iterator it(itemStr.cbegin()), end(itemStr.cend()); it != end; sserialize::nextSuffixString(it, end, separators)) {
					MyTrie::value_type & v = m_trie.at(it, end);
					if ( !v.suffix.size() || v.suffix.back() != i) {
						v.suffix.push_back(i);
					}
				}
			}
		}
		
		m_trie.append<PayloadHandler, TriePayload>(trieData, phandler, nodeCreator);
		indexFactory->flush();
		trieData.resetPtrs();
		
		m_sTrie = MyStaticTrie(trieData);
		m_indexStore = Static::ItemIndexStore(indexFactory->getFlushedData());
		m_strCompleter = StringCompleter( new MyStringCompleterPrivate(m_sTrie, m_indexStore) );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP | StringCompleter::SQ_CASE_SENSITIVE | StringCompleter::SQ_SSP;
		return (StringCompleter::SupportedQuerries) sq;
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
public:
    StaticUnicodeTrieTest() {}

	virtual void setUp() {
		setUpCompleter();
	}
	
	virtual void tearDown() {}
	
	void testTrieEquality() {
		CPPUNIT_ASSERT(m_trie.checkTrieEquality(m_sTrie.getRootNode()));
	}
	
	void testIndexEquality() {

	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticUnicodeTrieTest::suite() );
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}