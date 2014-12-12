#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/containers/UnicodeStringMap.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/Iterator.h>
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_BASE_VERSION 1
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_VERSION 1

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

class FlatTrieBase;

namespace detail {
namespace FlatTrie {

struct StaticString {
	StaticString(uint32_t off, uint32_t size) : m_off(off), m_size(size) {}
	StaticString() : m_off(0), m_size(0) {}
	uint32_t m_off;
	uint32_t m_size;
	inline uint32_t size() const { return m_size; }
	inline uint32_t off() const { return m_off; }
};

struct CompFunc {
	const FlatTrieBase * strHandler;
	uint32_t posInStr;
	CompFunc(const FlatTrieBase * strHandler, uint32_t posInStr) : strHandler(strHandler), posInStr(posInStr) {}
	bool operator()(uint32_t a, const StaticString & b) const;
	bool operator()(const StaticString & a, uint32_t b) const;
	inline bool operator==(const CompFunc & other) const { return posInStr == other.posInStr && strHandler == other.strHandler; }
	inline bool operator!=(const CompFunc & other) const { return posInStr != other.posInStr && strHandler != other.strHandler; }
};

class Node {
public:
	class Iterator {
	public:
		typedef Node value_type;
	private:
		uint32_t m_childNodeBegin;
		uint32_t m_childNodeEnd;
		uint32_t m_childrenEnd;
		CompFunc m_compFunc;
	public:
		Iterator(const uint32_t parentBegin, const uint32_t parentEnd, const CompFunc & compFunc);
		~Iterator() {}
		Iterator & operator++();
		bool operator!=(const Iterator & other);
		bool operator==(const Iterator & other);
		Node operator*() const;
	};
	typedef Iterator const_iterator;
	typedef Iterator iterator;
private:
	const FlatTrieBase * m_trie;
	uint32_t m_begin;
	uint32_t m_end;
public:
	Node(uint32_t begin, uint32_t end, const FlatTrieBase * trie);
	virtual ~Node() {}
	inline uint32_t id() const { return m_begin; }
	StaticString sstr() const;
	UByteArrayAdapter strData() const;
	std::string str() const;
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator end() const;
	const_iterator cend() const;
};

}}//end namespace detail::FlatTrie

/** Layout:
  *
  *-------------------------------------------------------------------
  *VERSION|StringDataSize|    StringData   |StaticStrings(offset, len)|
  *--------------------------------------------------------------------
  *uin8t  |OffsetType    |UByteArrayAdapter|MultiVarBitArray         
  *
  *
  */
  
class FlatTrieBase {
public:
	typedef enum {TA_STR_OFFSET=0, TA_STR_LEN=1} TrieAccessors;
	static constexpr uint32_t npos = 0xFFFFFFFF;
	typedef detail::FlatTrie::StaticString StaticString;
	typedef detail::FlatTrie::Node Node;
	class StaticStringsIterator: public sserialize::StaticIterator<std::forward_iterator_tag, detail::FlatTrie::StaticString>  {
	private:
		const FlatTrieBase * m_trie;
		uint32_t m_pos;
	public:
		StaticStringsIterator(uint32_t pos, const FlatTrieBase * trie) : m_trie(trie), m_pos(pos) {}
		~StaticStringsIterator() {}
		inline uint32_t id() const { return m_pos; }
		inline value_type operator*() const { return m_trie->sstr(m_pos); }
		inline StaticStringsIterator & operator++() { ++m_pos; return *this; }
		inline StaticStringsIterator operator++(int) { return StaticStringsIterator(m_pos++, m_trie); }
		inline StaticStringsIterator & operator+=(uint32_t o) { m_pos += o; return *this; }
		inline StaticStringsIterator operator+(uint32_t o) { return StaticStringsIterator(m_pos+o, m_trie); }
		inline bool operator!=(const StaticStringsIterator & other) const { return m_pos != other.m_pos || m_trie != other.m_trie; }
	};
private:
	sserialize::UByteArrayAdapter m_strData;
	sserialize::MultiVarBitArray m_trie;
public:
	FlatTrieBase();
	FlatTrieBase(const sserialize::UByteArrayAdapter & src);
	virtual ~FlatTrieBase() {}
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t size() const { return m_trie.size();}
	StaticStringsIterator staticStringsBegin() const { return StaticStringsIterator(0, this); }
	StaticStringsIterator staticStringsEnd() const { return StaticStringsIterator(size(), this); }
	inline StaticString sstr(uint32_t pos) const { return StaticString(m_trie.at(pos, TA_STR_OFFSET), m_trie.at(pos, TA_STR_LEN)); }
	inline UByteArrayAdapter strData(const StaticString & str) const { return UByteArrayAdapter(m_strData, str.off(), str.size()); }
	inline std::string strAt(const StaticString & str) const {
		UByteArrayAdapter::MemoryView mem(strData(str).asMemView());
		return std::string(mem.begin(), mem.end());
	}
	inline UByteArrayAdapter strData(uint32_t pos) const { return strData(sstr(pos)); }
	inline std::string strAt(uint32_t pos) const { return strAt(sstr(pos)); }
	uint32_t find(const std::string & str, bool prefixMatch) const;
	Node root() const { return Node(0, size(), this); }
	std::ostream & printStats(std::ostream & out) const;
};

/** Layout:
  *
  *----------------------------------
  *FlatTrieBase|VERSION|Payload
  *----------------------------------
  *            |uin8t  |Array<TValue>      
  *
  *
  */
template<typename TValue>
class FlatTrie: public FlatTrieBase {
public:
	typedef TValue value_type;
private:
	sserialize::Static::Array<TValue> m_values;
public:
	FlatTrie() {}
	FlatTrie(const sserialize::UByteArrayAdapter & src);
	virtual ~FlatTrie() {}
	UByteArrayAdapter::OffsetType getSizeInBytes() const { return FlatTrieBase::getSizeInBytes() + 1 + m_values.getSizeInBytes(); }
	inline TValue at(uint32_t pos) const { return m_values.at(pos); }
	///throws sserialize::OutOfBoundsException on miss
	TValue at(const std::string & str, bool prefixMatch) const;
	template<typename T_OCTET_ITERATOR>
	inline TValue at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const {
		return at(std::string(strIt, strEnd), prefixMatch);
	}
	inline const sserialize::Static::Array<TValue> & payloads() const { return m_values; }
	std::ostream & printStats(std::ostream & out) const;
};

template<typename TValue>
class UnicodeStringMapFlatTrie: public sserialize::detail::UnicodeStringMap<TValue> {
public:
	typedef FlatTrie<TValue> TrieType;
private:
	TrieType m_trie;
public:
	UnicodeStringMapFlatTrie() {}
	UnicodeStringMapFlatTrie(const UByteArrayAdapter & d) : m_trie(d) {}
	UnicodeStringMapFlatTrie(const FlatTrie<TValue> & t) : m_trie(t) {}
	virtual ~UnicodeStringMapFlatTrie() {}
	virtual TValue at(const std::string & str, bool prefixMatch) const override {
		return m_trie.at(str, prefixMatch);
	}
	virtual bool count(const std::string & str, bool prefixMatch) const override {
		return m_trie.find(str, prefixMatch) != FlatTrieBase::npos;
	}
	virtual std::ostream & printStats(std::ostream & out) const override {
		return m_trie.printStats(out);
	}
	std::string getName() const override {
		return std::string("UnicodeStringMapFlatTrie");
	}
	
	const TrieType & trie() const { return m_trie; }
	TrieType & trie() { return m_trie; }
};

template<typename TValue>
FlatTrie<TValue>::FlatTrie(const sserialize::UByteArrayAdapter & src) :
FlatTrieBase(src),
m_values(src+(1+FlatTrieBase::getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_VERSION, src.at(FlatTrieBase::getSizeInBytes()), "sserialize::Static::UnicodeTrie::FlatTrie");
	SSERIALIZE_EQUAL_LENGTH_CHECK(FlatTrieBase::size(), m_values.size(), "sserialize::Static::UnicodeTrie::FlatTrie");
}

template<typename TValue>
TValue 
FlatTrie<TValue>::at(const std::string & str, bool prefixMatch) const {
	uint32_t pos = find(str, prefixMatch);
	if (pos != npos) {
		return at(pos);
	}
	throw sserialize::OutOfBoundsException("sserialize::Static::UnicodeTrie::FlatTrie::at");
	return TValue();
}

template<typename TValue>
std::ostream & FlatTrie<TValue>::printStats(std::ostream & out) const {
	out << "sserialize::Static::UnicodeTrie::FlatTrie::stats--BEGIN" << std::endl;
	FlatTrieBase::printStats(out);
	m_values.printStats(out);
	out << "sserialize::Static::UnicodeTrie::FlatTrie::stats--END" << std::endl;
	return out;
}

}}}//end namespace

// namespace std {
// 
// template<>
// struct iterator_traits< sserialize::Static::UnicodeTrie::FlatTrieBase::StaticStringsIterator> {
// 	typedef typename _Iterator::iterator_category iterator_category;
// 	typedef typename _Iterator::value_type        value_type;
// 	typedef typename _Iterator::difference_type   difference_type;
// 	typedef typename _Iterator::pointer           pointer;
// 	typedef typename _Iterator::reference         reference;
// };
// 
// }

#endif