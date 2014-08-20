#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_H
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_BASE_VERSION 1
#define SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_VERSION 1

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

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
private:
	struct StaticString {
		uint32_t m_off;
		uint32_t m_size;
		inline uint32_t size() const { return m_size; }
		inline uint32_t off() const { return m_off; }
	};
private:
	sserialize::UByteArrayAdapter m_strData;
	sserialize::MultiVarBitArray m_trie;
protected:
	StaticString sstr(uint32_t pos) const { return StaticString{ .m_off = m_trie.at(pos, TA_STR_OFFSET), .m_size = m_trie.at(pos, TA_STR_LEN)}; }
public:
	FlatTrieBase();
	FlatTrieBase(const sserialize::UByteArrayAdapter & src);
	virtual ~FlatTrieBase() {}
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t size() const { return m_trie.size();}
	inline UByteArrayAdapter strData(uint32_t pos) const {
		StaticString tmp(sstr(pos));
		return UByteArrayAdapter(m_strData, tmp.off(), tmp.size());
	}
	inline std::string strAt(uint32_t pos) const {
		UByteArrayAdapter::MemoryView mem(strData(pos).asMemView());
		return std::string(mem.begin(), mem.end());
	}
	uint32_t find(const std::string & str, bool prefixMatch) const;
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
FlatTrie<TValue>::FlatTrie(const sserialize::UByteArrayAdapter & src) :
FlatTrieBase(src),
m_values(src+(1+FlatTrieBase::getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_UNICODE_TRIE_FLAT_TRIE_VERSION, src.at(1+FlatTrieBase::getSizeInBytes()), "sserialize::Static::UnicodeTrie::FlatTrie");
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


}}}//end namespace

#endif