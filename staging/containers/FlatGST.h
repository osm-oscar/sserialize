#ifndef SSERIALIZE_FLAT_GST_H
#define SSERIALIZE_FLAT_GST_H
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/utility/DiacriticRemover.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/templated/WindowedArray.h>
#include <unordered_set>
#include <algorithm>

namespace sserialize {


/** IDEA:
  *
  *
  *
  */

class FlatGST: public StringCompleterPrivate {
public:
	typedef std::vector<std::string> OrderedStringTable;
	typedef uint32_t ItemIdType;
	typedef uint32_t StrIdType;
	static const uint32_t npos = 0xFFFFFFFF;
private:

	typedef enum {SC_SMALLER, SC_EQUAL, SC_LARGER} StringComparisonType;
	struct Entry {
		Entry(StrIdType strId, uint16_t pos, uint16_t len) : strId(strId), pos(pos), len(len) {}
		StrIdType strId;
		uint16_t pos;
		uint16_t len;
		WindowedArray<ItemIdType> exactMatchedItems;
		WindowedArray<ItemIdType> suffixMatchedItems;
		inline std::string::const_iterator cbegin(const OrderedStringTable * strTable) const {return strTable->at(strId).cbegin() + pos;}
		inline std::string::const_iterator cend(const OrderedStringTable * strTable) const {return strTable->at(strId).cbegin() + (pos+len);}
	};
	
	class EntryLcpCalculator {
		const OrderedStringTable * m_st;
		typedef std::string::const_iterator StrIt;
		inline bool calc(StrIt aIt, StrIt bIt, const StrIt & aEnd, const StrIt & bEnd) const {
			uint32_t c = 0;
			while (*aIt == *bIt && aIt != aEnd && bIt != bEnd) {
				++aIt;
				++bIt;
				++c;
			}
			return c;
		}
	public:
		EntryLcpCalculator(const OrderedStringTable * strTable) : m_st(strTable) {}
		~EntryLcpCalculator() {}
		inline uint32_t operator()(const Entry & a, const Entry & b) const {
			return calc(a.cbegin(m_st), b.cbegin(m_st), a.cend(m_st), b.cend(m_st));
		}
		inline uint32_t operator()(const Entry & a, const std::string & str) const {
			const std::string & bStr = str;
			return calc(a.cbegin(m_st), bStr.begin(), a.cend(m_st), bStr.begin());
		}
		
		inline uint32_t operator()(const std::string & str, const Entry & b) const {
			return operator()(b, str);
		}
	};
	
	class EntryComparator {
		const OrderedStringTable * m_st;
		typedef std::string::const_iterator StrIt;
	public:
		inline int cmp(StrIt aIt, StrIt bIt, const StrIt & aEnd, const StrIt & bEnd) const {
			while (aIt != aEnd && bIt != bEnd) {
				if (*aIt == *bIt) {
					++aIt;
					++bIt;
				}
				else if (*aIt < *bIt)
					return -1;
				else
					return 1;
			}
			if (bIt == bEnd) {
				if (aIt != aEnd) {
					return 1;
				}
				return 0;
			}
			else {
				return -1;
			}
		}
	public:
		EntryComparator(const OrderedStringTable * strTable) : m_st(strTable) {}
		~EntryComparator() {}
		inline int operator()(const Entry & a, const Entry & b, uint32_t lcp = 0) const {
			return cmp(a.cbegin(m_st)+lcp, b.cbegin(m_st)+lcp, a.cend(m_st), b.cend(m_st));
		}
		inline int operator()(const Entry & a, const std::string & str, uint32_t lcp = 0) const {
			return cmp(a.cbegin(m_st)+lcp, str.begin(), a.cend(m_st), str.begin());
		}
		
		inline int operator()(const std::string & str, const Entry & b, uint32_t lcp = 0) const {
			return cmp(str.cbegin()+lcp, b.cbegin(m_st)+lcp, str.cend(), b.cend(m_st));
		}
	};
	
	class EntrySmallerComparator {
		EntryComparator m_cmp;
	public:
		EntrySmallerComparator(const OrderedStringTable * strTable) : m_cmp(strTable) {}
		~EntrySmallerComparator() {}
		inline bool operator()(const Entry & a, const Entry & b) const {
			return (m_cmp(a, b) < (int)0);
		}
		inline bool operator()(const Entry & a, const std::string & b) const {
			return (m_cmp(a, b) < (int)0);
		}
		
		inline bool operator()(const std::string & a, const Entry & b) const {
			return (m_cmp(a, b) < (int)0);
		}
	};
	
	class EntryEqualityComparator {
		EntryComparator m_cmp;
	public:
		EntryEqualityComparator(const OrderedStringTable * strTable) : m_cmp(strTable) {}
		~EntryEqualityComparator() {}
		inline bool operator()(const Entry & a, const Entry & b) const {
			if (a.len != b.len) {
				return false;
			}
			if (a.strId == b.strId) {
				return a.pos == b.pos;
			}
			else {
				return m_cmp(a, b) == 0;
			}
		}
		inline bool operator()(const std::string & a, const Entry & b) const {
			if (a.size() != b.len) {
				return false;
			}
			else {
				return m_cmp(a, b) == 0;
			}
		}
		inline bool operator()(const Entry & a, const std::string & b) const {
			return operator()(b, a);
		}
	};
	
	class EntryHasher {
	private:
		OrderedStringTable * m_data;
	public:
		EntryHasher(OrderedStringTable * data) : m_data(data) {}
		inline std::size_t operator()(const Entry & e) const {
			std::size_t s = 0;
			const std::string & str = m_data->at(e.strId);
			std::string::const_iterator it(str.begin()+e.pos);
			std::string::const_iterator end(it+e.len);
			for(; it != end; ++it) {
				hash_combine(s, *it);
			}
			return s;
		}
	};
	
	typedef std::vector<Entry> TrieStorage;
	
	class Node {
	private:
		uint32_t m_begin;
		uint32_t m_end;
		///find the next theoretical child and returns the position of the beginning.
		uint32_t findNextChild(sserialize::FlatGST::TrieStorage * trie, const sserialize::FlatGST::OrderedStringTable * strTable, uint32_t begin, uint32_t end, uint32_t strOffset, uint32_t prevChildCp) const;
	public:
		Node() {}
		Node(uint32_t begin, uint32_t end);
		~Node();
		///very costly operation O(n) in the size of @trie
		std::map<uint32_t, Node> children(sserialize::FlatGST::TrieStorage * trie, const sserialize::FlatGST::OrderedStringTable * strTable) const;
	};
	
private:
	bool m_isSuffixTrie;
	bool m_caseSensitive;
	bool m_addTransDiacs;
	std::unordered_set<uint32_t> m_suffixDelimeters;
	OrderedStringTable m_strTable;
	TrieStorage m_trie;
	MmappedMemory<ItemIdType> m_exactMatchedItemData;
	MmappedMemory<ItemIdType> m_suffixMatchedItemData;
	
private:
	uint32_t find(const std::string & qStr, sserialize::StringCompleter::QuerryType qt) const;
	Node rootNode();
	template<typename T_ITEM_FACTORY, typename T_ITEM>
	bool populateStringTable(const T_ITEM_FACTORY & stringsFactory);
	
	void nextSuffixString(std::string::const_iterator & strIt, const std::string::const_iterator & strEnd);
	void trieFromMyStringTable();
public:
	FlatGST();
	virtual ~FlatGST();
	inline void setCaseSensitivity(bool c) { m_caseSensitive = c; }
	inline void setSuffixTrie(bool c) { m_isSuffixTrie = c; }
	inline void setSuffixDelimeters(const std::unordered_set<uint32_t> & s) { m_suffixDelimeters = s; }
	
	/** If you set this to true, then transliterated versions of itemstrings are added as well
	  * This is only compatible with the Static::Trie. The FlatGST does not support this!
	  *
	  */
	inline void setAddTransliteratedDiacritics(bool c) { m_addTransDiacs = c; }
	
	inline bool isCaseSensitive() { return m_caseSensitive; }
	inline bool isSuffixTrie() { return m_isSuffixTrie;}
	inline std::unordered_set<uint32_t> getSuffixDelimeters() { return m_suffixDelimeters; }
	
	template<typename T_ITEM_FACTORY, typename T_ITEM>
	bool create(const T_ITEM_FACTORY & stringsFactory);

	virtual ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
		
	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const { return std::string("sserialize::FlatGST"); }
	
	///Serialize the trie with CompactLargeNode, no merge index, direct index
	void serialize(sserialize::UByteArrayAdapter & dest);
};


template<typename T_ITEM_FACTORY, typename T_ITEM>
bool FlatGST::populateStringTable(const T_ITEM_FACTORY & stringsFactory) {
	DiacriticRemover transLiterator;
	if (m_addTransDiacs) {
		DiacriticRemover::DiacriticRemover::ErrorCodeType status = transLiterator.init();
		if (DiacriticRemover::DiacriticRemover::isFailure(status)) {
			std::cerr << "Failed to create translitorated on request: " << DiacriticRemover::DiacriticRemover::errorName(status) << std::endl;
			return false;
		}
	}

	ProgressInfo progressInfo;
	uint32_t count = 0;
	std::unordered_set<std::string> strings;
	progressInfo.begin(stringsFactory.end()-stringsFactory.begin(), "BaseTrie::trieFromStringsFactory: Gathering strings");
	for(typename T_ITEM_FACTORY::const_iterator itemsIt(stringsFactory.begin()), itemsEnd(stringsFactory.end()); itemsIt != itemsEnd; ++itemsIt) {
		T_ITEM item = *itemsIt;
		for(typename T_ITEM::const_iterator itemStrsIt(item.begin()), itemStrsEnd(item.end()); itemStrsIt != itemStrsEnd; ++itemStrsIt) {
			std::string str = *itemStrsIt;
			if (m_caseSensitive) {
				strings.insert(str);
				if (m_addTransDiacs) {
					transLiterator.transliterate(str); //ATTENTION: str should not be reused after this
					strings.insert( str );
				}
			}
			else {
				str = unicode_to_lower(str);
				if (m_addTransDiacs) {
					transLiterator.transliterate(str); //ATTENTION: str should not be reused after this
					strings.insert( str );
				}
			}
		}
		progressInfo(++count);
	}
	progressInfo.end();
	m_strTable = OrderedStringTable(strings.begin(), strings.end());
	std::sort(m_strTable.begin(), m_strTable.end());
	return true;
}


}//end namespaces

#endif