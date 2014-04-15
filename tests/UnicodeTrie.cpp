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

class StaticUnicodeTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( StaticUnicodeTrieTest );
CPPUNIT_TEST( testCreateStringCompleter );
CPPUNIT_TEST( testSupportedQuerries );
CPPUNIT_TEST( testCompletionECS );
// CPPUNIT_TEST( testCompletionECI );
// CPPUNIT_TEST( testCompletionPCS );
// CPPUNIT_TEST( testCompletionPCI );
CPPUNIT_TEST( testCompletionSCS );
// CPPUNIT_TEST( testCompletionSCI );
// CPPUNIT_TEST( testCompletionSPCS );
// CPPUNIT_TEST( testCompletionSPCI );
CPPUNIT_TEST_SUITE_END();
private:
	struct TrieStorageData {
		std::vector<uint32_t> exact;
		std::vector<uint32_t> suffix;
	};

	typedef UnicodeTrie::Trie<TrieStorageData> MyTrie;

	class MyStringCompleterPrivate: public StringCompleterPrivate {
	private:
		const MyTrie * m_Trie;
		Static::ItemIndexStore m_store;
	public:
		MyStringCompleterPrivate(const MyTrie * trie) : m_Trie(trie) {}
		virtual ~MyStringCompleterPrivate() {}
		virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
			TrieStorageData rd;
			try {
				rd = m_Trie->find(str.cbegin(), str.cend(), (qtype & StringCompleter::QT_PREFIX || qtype & StringCompleter::QT_SUBSTRING));
			}
			catch (const OutOfBoundsException & c) {
				return ItemIndex();
			}
			if (qtype & StringCompleter::QT_SUBSTRING) {
				return ItemIndex(rd.suffix);
			}
			if (qtype & StringCompleter::QT_PREFIX) {
				return ItemIndex(rd.exact);
			}
			if (qtype & StringCompleter::QT_SUFFIX) {
				return ItemIndex(rd.suffix);
			}
			if (qtype & StringCompleter::QT_EXACT) {
				return ItemIndex(rd.exact);
			}
			return ItemIndex();
		}
		virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
			return (StringCompleter::SupportedQuerries) (StringCompleter::SQ_EXACT | StringCompleter::SQ_SUFFIX | StringCompleter::SQ_CASE_SENSITIVE);
		}
		virtual std::string getName() const {
			return std::string("MyStringCompleterPrivate");
		}
	};
	
private:
	sserialize::Static::ItemIndexStore m_indexStore;
	MyTrie m_trie;
private:
	StringCompleter m_strCompleter;
	virtual StringCompleter& stringCompleter() { return m_strCompleter; }

protected:
	bool setUpCompleter() {
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
		
		m_strCompleter = StringCompleter( new MyStringCompleterPrivate(&m_trie) );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EXACT | StringCompleter::SQ_CASE_SENSITIVE | StringCompleter::SQ_SUFFIX;
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
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( StaticUnicodeTrieTest::suite() );
	runner.eventManager().popProtector();
	runner.run();
	return 0;
}