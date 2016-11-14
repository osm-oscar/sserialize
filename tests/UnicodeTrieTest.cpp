#include <sserialize/containers/UnicodeTrie.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/strings/stringfunctions.h>
#include "test_stringcompleter.h"
#include "TestItemData.h"
#include "StringCompleterTest.h"
#include <cppunit/TestResult.h>

using namespace sserialize;

struct TrieStorageData {
	std::vector<uint32_t> exact;
	std::vector<uint32_t> suffix;
};

typedef UnicodeTrie::Trie<TrieStorageData> MyTrie;

template<bool T_CASE_INSENSITIVE>
class UnicodeTrieTest: public StringCompleterTest {
CPPUNIT_TEST_SUITE( UnicodeTrieTest );
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
CPPUNIT_TEST( testConsistency );
CPPUNIT_TEST_SUITE_END();
private:
	class MyStringCompleterPrivate: public StringCompleterPrivate {
	private:
		const MyTrie * m_Trie;
		Static::ItemIndexStore m_store;
	protected:
		void uniteSuffix(const MyTrie::Node * node, std::set<uint32_t> & substr) const {
			if (node) {
				substr.insert(node->value().suffix.cbegin(), node->value().suffix.cend());
				for(const auto & x: node->children()) {
					uniteSuffix(x.second, substr);
				}
			}
		}
		void uniteExact(const MyTrie::Node * node, std::set<uint32_t> & prefix) const {
			if (node) {
				prefix.insert(node->value().exact.cbegin(), node->value().exact.cend());
				for(const auto & x: node->children()) {
					uniteExact(x.second, prefix);
				}
			}
		}
	public:
		MyStringCompleterPrivate(const MyTrie * trie) : m_Trie(trie) {}
		virtual ~MyStringCompleterPrivate() {}
		virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
			const MyTrie::Node * node = 0;
			if (T_CASE_INSENSITIVE) {
				std::string tmp(unicode_to_lower(str));
				node = m_Trie->findNode(tmp.cbegin(), tmp.cend(), (qtype & StringCompleter::QT_PREFIX || qtype & StringCompleter::QT_SUBSTRING));
			}
			else {
				node = m_Trie->findNode(str.cbegin(), str.cend(), (qtype & StringCompleter::QT_PREFIX || qtype & StringCompleter::QT_SUBSTRING));
			}
			if (!node)
				return ItemIndex();
			if (qtype & StringCompleter::QT_SUBSTRING) {
				std::set<uint32_t> tmp;
				uniteSuffix(node, tmp);
				return ItemIndex(std::vector<uint32_t>(tmp.cbegin(), tmp.cend()));
			}
			if (qtype & StringCompleter::QT_PREFIX) {
				std::set<uint32_t> tmp;
				uniteExact(node, tmp);
				return ItemIndex(std::vector<uint32_t>(tmp.cbegin(), tmp.cend()));
			}
			if (qtype & StringCompleter::QT_SUFFIX) {
				return ItemIndex(node->value().suffix);
			}
			if (qtype & StringCompleter::QT_EXACT) {
				return ItemIndex(node->value().exact);
			}
			return ItemIndex();
		}
		virtual StringCompleter::SupportedQuerries getSupportedQuerries() const {
			uint8_t msq = (StringCompleter::SQ_EPSP | StringCompleter::SQ_SSP);
			if (T_CASE_INSENSITIVE) {
				msq |= StringCompleter::SQ_CASE_INSENSITIVE;
			}
			else {
				msq |= StringCompleter::SQ_CASE_SENSITIVE;
			}
			return (StringCompleter::SupportedQuerries) msq;
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
		
		for(uint32_t i(0), s((uint32_t) items().size()); i < s; ++i) {
			const TestItemData & item = items()[i];
			for(const std::string & itemStr : item.strs) {
				std::string putStr = itemStr;
				if (T_CASE_INSENSITIVE) {
					putStr = sserialize::unicode_to_lower(putStr);
				}
				std::string::const_iterator it(putStr.cbegin()), end(putStr.cend());
				{
					MyTrie::value_type & v = m_trie.at(putStr.cbegin(), putStr.cend());
					v.exact.push_back(i);
					v.suffix.push_back(i);
					sserialize::nextSuffixString(it, end, separators);
				}
				for(;it != end; sserialize::nextSuffixString(it, end, separators)) {
					MyTrie::value_type & v = m_trie.at(it, end);
					if ( v.suffix.size() == 0 || v.suffix.back() != i) {
						v.suffix.push_back(i);
					}
				}
			}
		}
		
		m_strCompleter = StringCompleter( new MyStringCompleterPrivate(&m_trie) );
		
		return true;
	}
	
	virtual StringCompleter::SupportedQuerries supportedQuerries() {
		uint8_t sq = StringCompleter::SQ_EP | StringCompleter::SQ_SSP;
		if (T_CASE_INSENSITIVE) {
			sq |= StringCompleter::SQ_CASE_INSENSITIVE;
		}
		else {
			sq |= StringCompleter::SQ_CASE_SENSITIVE;
		}
			
		return (StringCompleter::SupportedQuerries) sq;
	}

	virtual bool createStringCompleter() {
		return true;
	}
	
public:
    UnicodeTrieTest() {}

	virtual void setUp() {
		setUpCompleter();
	}
	
	virtual void tearDown() {}
	
	void testConsistency() {
		bool ok = true;
		auto consistenyChecker = [&ok](const MyTrie::Node & n) {
			for(const auto & x : n.children()) {
				if (x.second->parent() != &n) {
					ok = false;
					return;
				}
			}
		};
		m_trie.root()->apply(consistenyChecker);
		CPPUNIT_ASSERT(ok);
	}
};

int main() {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( UnicodeTrieTest<false>::suite() );
	runner.addTest( UnicodeTrieTest<true>::suite() );
// 	runner.eventManager().popProtector();
	bool ok = runner.run();
	return ok ? 0 : 1;
}