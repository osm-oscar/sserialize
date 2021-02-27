#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/Version.h>
#include <sserialize/containers/UnicodeStringMap.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/iterator/Iterator.h>
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_BASE_VERSION 1
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_VERSION 1

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

class FlatTrieBase;

namespace detail {
namespace FlatTrie {

class StaticString final {
public:
	using SizeType = uint32_t;
	using OffsetType = uint64_t;
	static constexpr SizeType OffsetBits = 32;
	static constexpr SizeType SizeBits = 64-OffsetBits;
	static constexpr OffsetType noff = sserialize::createMask64(OffsetBits);
	static constexpr OffsetType nsize = sserialize::createMask64(SizeBits);
	static constexpr OffsetType MaxOffset = noff-1;
	static constexpr SizeType MaxStringSize = nsize-1;
public:
	StaticString() : m_off(noff), m_size(nsize) {}
	StaticString(const StaticString & other) :
	m_off(other.m_off),
	m_size(other.m_size)
	{}
	explicit StaticString(OffsetType offset, SizeType size) :
	m_off(offset),
	m_size(size)
	{
		if ( UNLIKELY_BRANCH(m_off != offset) ) {
			throw std::out_of_range("StaticString: offset is too large");
		}
		if ( UNLIKELY_BRANCH(m_size != size) ) {
			throw std::out_of_range("StaticString: size is too large");
		}
	}
	explicit StaticString(SizeType size) :
	StaticString(noff, size)
	{}
	~StaticString() {}
public:
	inline OffsetType offset() const { return m_off; }
	inline SizeType size() const { return m_size; }
	inline bool isSpecial() const { return m_off == noff; }
	inline bool isInvalid() const { return m_off == noff && m_size == nsize; }
	///returns a copy with adjusted size
	inline StaticString addOffset(SizeType off) const { return StaticString(m_off + off, m_size-off); }
	inline StaticString shrinkBy(SizeType size) const {
		return StaticString(m_off, this->size()-std::min(this->size(), size));
	}
private:
	uint64_t m_off:OffsetBits;
	uint64_t m_size:SizeBits;
};

struct CompFunc {
	const FlatTrieBase * strHandler;
	StaticString::SizeType posInStr;
	CompFunc(const FlatTrieBase * strHandler, uint32_t posInStr) : strHandler(strHandler), posInStr(posInStr) {}
	bool operator()(uint32_t a, const StaticString & b) const;
	bool operator()(const StaticString & a, uint32_t b) const;
	inline bool operator==(const CompFunc & other) const { return posInStr == other.posInStr && strHandler == other.strHandler; }
	inline bool operator!=(const CompFunc & other) const { return posInStr != other.posInStr && strHandler != other.strHandler; }
};

class Node {
public:
	using SizeType = sserialize::Size;
	class Iterator {
	public:
		typedef Node value_type;
	private:
		SizeType m_childNodeBegin;
		SizeType m_childNodeEnd;
		SizeType m_childrenEnd;
		CompFunc m_compFunc;
	public:
		Iterator(const SizeType parentBegin, const SizeType parentEnd, const CompFunc & compFunc);
		~Iterator() {}
		Iterator & operator++();
		bool operator!=(const Iterator & other) const;
		bool operator==(const Iterator & other) const;
		Node operator*() const;
	};
	typedef Iterator const_iterator;
	typedef Iterator iterator;
private:
	const FlatTrieBase * m_trie;
	SizeType m_begin;
	SizeType m_end;
public:
	Node(SizeType begin, SizeType end, const FlatTrieBase * trie);
	virtual ~Node() {}
	inline SizeType id() const { return m_begin; }
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
  
class FlatTrieBase: sserialize::Static::SimpleVersion<1, FlatTrieBase> {
public:
	using Version = sserialize::Static::SimpleVersion<1, FlatTrieBase>;
	using SizeType = sserialize::MultiVarBitArray::SizeType;
	typedef enum {TA_STR_OFFSET=0, TA_STR_LEN=1} TrieAccessors;
	static constexpr SizeType npos = std::numeric_limits<SizeType>::max();
	typedef detail::FlatTrie::StaticString StaticString;
	using StringSizeType = StaticString::SizeType;
	typedef detail::FlatTrie::Node Node;
	class StaticStringsIterator: public sserialize::StaticIterator<std::forward_iterator_tag, detail::FlatTrie::StaticString>  {
	private:
		const FlatTrieBase * m_trie;
		SizeType m_pos;
	public:
		StaticStringsIterator(SizeType pos, const FlatTrieBase * trie) : m_trie(trie), m_pos(pos) {}
		~StaticStringsIterator() {}
		inline SizeType id() const { return m_pos; }
		inline value_type operator*() const { return m_trie->sstr(m_pos); }
		inline StaticStringsIterator & operator++() { ++m_pos; return *this; }
		inline StaticStringsIterator operator++(int) { return StaticStringsIterator(m_pos++, m_trie); }
		inline StaticStringsIterator & operator+=(SizeType o) { m_pos += o; return *this; }
		inline StaticStringsIterator operator+(SizeType o) { return StaticStringsIterator(m_pos+o, m_trie); }
		inline bool operator!=(const StaticStringsIterator & other) const { return m_pos != other.m_pos || m_trie != other.m_trie; }
	};
private:
	sserialize::UByteArrayAdapter m_strData;
	sserialize::MultiVarBitArray m_trie;
private:
	template<typename TVISITOR>
	void visitDF(const Node & node, TVISITOR & visitor) {
		visitor(node);
		for(auto x : node) {
			visitDF(x, visitor);
		}
	}
public:
	FlatTrieBase();
	FlatTrieBase(const sserialize::UByteArrayAdapter & src);
	virtual ~FlatTrieBase() {}
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	UByteArrayAdapter data() const;
	SizeType size() const;
	inline const UByteArrayAdapter & strData() const { return m_strData; }
	StaticStringsIterator staticStringsBegin() const;
	StaticStringsIterator staticStringsEnd() const;
	StaticString sstr(SizeType pos) const;
	std::string strAt(const StaticString & str) const;
	std::string strAt(SizeType pos) const;
	UByteArrayAdapter strData(const StaticString & str) const;
	UByteArrayAdapter strData(uint32_t pos) const;
	inline StringSizeType strSize(const StaticString & str) const { return str.size(); }
	inline StringSizeType strSize(SizeType pos) const { return strSize(sstr(pos)); }
	SizeType find(const std::string & str, bool prefixMatch) const;
	Node root() const;
	std::ostream & printStats(std::ostream & out) const;
	///visit all nodes in depth-first search
	template<typename TVISITOR>
	inline void visitDF(TVISITOR visitor) { visitDF(root(), visitor); }
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
class FlatTrie: public FlatTrieBase, sserialize::Static::SimpleVersion<1, FlatTrie<TValue> > {
public:
	using Version = sserialize::Static::SimpleVersion<1, FlatTrie<TValue> >;
	using SizeType = FlatTrieBase::SizeType;
public:
	typedef TValue value_type;
private:
	sserialize::Static::Array<TValue> m_values;
public:
	FlatTrie() {}
	FlatTrie(const sserialize::UByteArrayAdapter & src);
	virtual ~FlatTrie() {}
	UByteArrayAdapter::OffsetType getSizeInBytes() const { return FlatTrieBase::getSizeInBytes() + 1 + m_values.getSizeInBytes(); }
	inline TValue at(SizeType pos) const { return m_values.at(pos); }
	///throws sserialize::OutOfBoundsException on miss
	TValue at(const std::string & str, bool prefixMatch) const;
	template<typename T_OCTET_ITERATOR>
	TValue at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const {
		return at(std::string(strIt, strEnd), prefixMatch);
	}
	const sserialize::Static::Array<TValue> & payloads() const { return m_values; }
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
	virtual bool count(const std::string::const_iterator & strBegin, const std::string::const_iterator & strEnd, bool prefixMatch) const override {
		return count(std::string(strBegin, strEnd), prefixMatch);
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
Version(src+FlatTrieBase::getSizeInBytes(), typename Version::NoConsume()),
m_values(src+(1+FlatTrieBase::getSizeInBytes()))
{
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
	out << "total data size=" << FlatTrieBase::getSizeInBytes() + m_values.getSizeInBytes() << std::endl;
	FlatTrieBase::printStats(out);
	m_values.printStats(out);
	out << "sserialize::Static::UnicodeTrie::FlatTrie::stats--END" << std::endl;
	return out;
}

}}}//end namespace

#endif
