#ifndef STATIC_FLAT_GENERALIZED_TRIE_H
#define STATIC_FLAT_GENERALIZED_TRIE_H
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/containers/MultiVarBitArray.h>

namespace sserialize {
namespace Static {

/** Fileformat: v1
  *-------------------------------------------------------------------
  *VERSION|SQ|STRINGTABLE|  StringEntries  |DEQUE<FlatGST::IndexEntry>
  *-------------------------------------------------------------------
  *   1   |1 |     *     |MultiVarBitArray |          *
  *
  *
  *SQ = SupportedQuerries from sserialize::StringCompleter
  *
  * IndexEntry File format:
  * ------------------------
  * MEETAAAA|Entries
  * ------------------------
  * 1 byte |CompateUintArray
  *------------------------
  * M = defines if indices need to be merged, i.e. if suffixIndex needs to be merged with exactIndex
  * EE = defines the size of  the entries in bytes (1 to 4)
  * T = if true, then the index holds item ids, others wise in holds string ids
  * A = Select the indices present
  */

class FlatGST: public Static::StringCompleterPrivate {
public:
	typedef Static::StringCompleterPrivate MyBaseClass;
	class StringEntry {
		uint32_t m_strId;
		uint16_t m_strBegin;
		uint16_t m_strLen;
	public:
		StringEntry();
		StringEntry(const MultiVarBitArray & data, uint32_t pos);
		~StringEntry();
		uint32_t strId() const;
		uint16_t strBegin() const;
		uint16_t strLen() const;
	};
	
	class IndexEntry {
	public:
		typedef enum {IT_NONE=0, IT_EXACT=0x1, IT_PREFIX=0x2, IT_SUFFIX=0x4, IT_SUFFIX_PREFIX=0x8, IT_ALL=0xF, IT_ITEM_ID_INDEX=0x10, IT_MERGE_INDEX=0x80} IndexTypes;
	private:
		uint8_t m_header;
		CompactUintArray m_data;
		uint8_t entrySize() const;
	public:
		IndexEntry();
		IndexEntry(const UByteArrayAdapter & data);
		~IndexEntry();
		bool mergeIndex() const;
		bool itemIdIndex() const;
		uint8_t indexType() const;
		uint32_t exactPtr() const;
		uint32_t suffixPtr() const;
		uint32_t prefixPtr() const;
		uint32_t suffixPrefixPtr() const;
		
		//only usefull if IT_ITEM_ID_INDEX is set
		uint32_t minId() const;
		uint32_t maxId() const;
	};
	
	class ForwardIterator: public MyBaseClass::ForwardIterator {
	private:
		std::string m_string;
		const FlatGST * m_cmp;
	public:
		ForwardIterator(const FlatGST * cmp);
		ForwardIterator(const FlatGST::ForwardIterator & other);
		virtual ~ForwardIterator();
		virtual std::set<uint32_t> getNext() const;
		virtual bool hasNext(uint32_t codepoint) const;
		virtual bool next(uint32_t codepoint);
		virtual MyBaseClass::ForwardIterator * copy() const;
	};
	
private:
	sserialize::StringCompleter::SupportedQuerries m_sq;
	ItemIndexStore m_idxStore;
	StringTable m_stable;
	MultiVarBitArray m_strEntries;
	Static::Array<IndexEntry> m_indexEntries;
protected:
	const Static::Array<IndexEntry> & indexEntries() const { return m_indexEntries; }
	int32_t lowerBound(const std::string& str, sserialize::StringCompleter::QuerryType qt) const;
	int32_t getStringEntryPos(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
public:
    FlatGST();
	FlatGST(const UByteArrayAdapter & data, const ItemIndexStore & idxStore);
	virtual ~FlatGST();
	virtual OffsetType getSizeInBytes() const;
	virtual ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual ItemIndex indexFromId(uint32_t idxId) const;
	ItemIndexIterator indexIteratorFromId(uint32_t idxId) const;
	
	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
	virtual sserialize::StringCompleterPrivate::ForwardIterator * forwardIterator() const;
	
	/** @return nodecount */
	uint32_t size() const;
	
	//Access functions
	
	UByteArrayAdapter fgstStringAt(uint32_t pos) const;
	StringEntry stringEntryAt(uint32_t pos) const;
	IndexEntry indexEntryAt(uint32_t pos) const;
	
	virtual ItemIndex indexFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const;
	   ItemIndexIterator indexIteratorFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual ItemIndex indexFromEntry(const IndexEntry & e, sserialize::StringCompleter::QuerryType qtype) const;
	virtual ItemIndexIterator indexIteratorFromEntry(const IndexEntry & e, sserialize::StringCompleter::QuerryType qtype) const;
	
	
	void dump();
	std::ostream & dump(std::ostream& out) const;
};

}}//end namespace

#endif