#ifndef SSERIALIZE_STATIC_FLAT_TRIE_STR_IDS_H
#define SSERIALIZE_STATIC_FLAT_TRIE_STR_IDS_H
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/containers/ItemIndexIteratorDB.h>

namespace sserialize {
namespace Static {

template<class DataBaseType>
class FlatGSTStrIds: public sserialize::Static::FlatGST {
private:
	typedef sserialize::Static::FlatGST MyBaseClass;
private:
	DataBaseType m_sdb;
public:
	FlatGSTStrIds() : FlatGST() {}
	FlatGSTStrIds(const UByteArrayAdapter & data, const ItemIndexStore & indexStore, const DataBaseType & sdb) :
	MyBaseClass(data, indexStore), m_sdb(sdb) {}
	virtual ~FlatGSTStrIds() {}

	virtual ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
		int pos = getStringEntryPos(str, qtype);
		if (pos > 0) {
			return indexFromPosition(pos, qtype);
		}
		else {
			return ItemIndex();
		}
	}
	
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
		int pos = getStringEntryPos(str, qtype);
		if (pos > 0) {
			return indexIteratorFromPosition(pos, qtype);
		}
		else {
			return ItemIndexIterator();
		}
	}
	
	virtual ItemIndex select(const std::unordered_set<uint32_t> & strIds) {
		return m_sdb.select(strIds);
	}
	
	virtual ItemIndex indexFromEntry(const IndexEntry & e, sserialize::StringCompleter::QuerryType qtype) const {
		if (e.indexType() & MyBaseClass::IndexEntry::IT_ITEM_ID_INDEX) {
			return MyBaseClass::indexFromEntry(e, qtype);
		}
		else {
			return m_sdb.select(MyBaseClass::indexFromEntry(e, qtype));
		}
	}
	
	virtual ItemIndex indexFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const {
		if (pos < size()) {
			return indexFromEntry(indexEntryAt(pos), qtype);
		}
		return ItemIndex();
	}

	virtual ItemIndexIterator indexIteratorFromPosition(uint32_t pos, sserialize::StringCompleter::QuerryType qtype) const {
		MyBaseClass::IndexEntry e( indexEntries().at(pos) );
		if (e.indexType() & MyBaseClass::IndexEntry::IT_ITEM_ID_INDEX) {
			return MyBaseClass::indexIteratorFromEntry(e, qtype);
		}
		else {
			return ItemIndexIterator( new ItemIndexIteratorDB<DataBaseType, ItemIndex>(m_sdb, MyBaseClass::indexFromEntry(e, qtype), e.minId(), e.maxId()+1) );
		}
	}
	
	virtual std::ostream& printStats(std::ostream& out) const {
		out << "FlatGSTStrIds::Stats::Begin" << std::endl;
		MyBaseClass::printStats(out);
		out << "FlatGSTStrIds::Stats::End" << std::endl;
		return out;
	}
	
	virtual std::string getName() const {
		return std::string("Static::FlatTrieStrIds");
	}
	
	DataBaseType & getDB() {
		return m_sdb;
	}
};

}}//end namespace

#endif