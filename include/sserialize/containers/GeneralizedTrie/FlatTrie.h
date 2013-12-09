#ifndef SSERIALIZE_GENERALIZED_TRIE_FLAT_TRIE_H
#define SSERIALIZE_GENERALIZED_TRIE_FLAT_TRIE_H
#include "BaseTrie.h"

namespace sserialize {
namespace GeneralizedTrie {

struct FlatGSTConfig {
	FlatGSTConfig(UByteArrayAdapter & destination, sserialize::ItemIndexFactory & indexFactory) :
		destination(destination), indexFactory(indexFactory),
		deleteTrie(false), mergeIndex(true)  {}
	FlatGSTConfig(UByteArrayAdapter & destination, sserialize::ItemIndexFactory & indexFactory, bool deleteTrie, bool mergeIndex) : 
		destination(destination), indexFactory(indexFactory),
		deleteTrie(deleteTrie), mergeIndex(mergeIndex)  {}
	UByteArrayAdapter & destination;
	sserialize::ItemIndexFactory & indexFactory;
	bool deleteTrie;
	bool mergeIndex;
};

class FlatTrie: public BaseTrie< std::vector<uint32_t> > {
public:
	
	typedef BaseTrie< std::vector<uint32_t> > MyBaseClass;
	
	struct StringEntry {
		uint32_t stringId;
		uint16_t strBegin;
		uint16_t strLen;
	};
	struct IndexEntry {
		IndexEntry() : mergeIndex(true), itemIdIndex(true), exactValues(0), prefixValues(0), suffixValues(0), suffixPrefixValues(0)  {}
		bool mergeIndex;
		bool itemIdIndex;
		uint32_t exactValues;
		uint32_t prefixValues;
		uint32_t suffixValues;
		uint32_t suffixPrefixValues;
		
		uint32_t minId;
		uint32_t maxId;
	};

	class FlatTrieEntryConfig {
	private:
// 		FlatTrieEntryConfig & operator=(const FlatTrieEntry & f);
	public:
		FlatTrieEntryConfig(ItemIndexFactory & indexFactory) : indexFactory(indexFactory) {}
		 ~FlatTrieEntryConfig() {}
		std::vector<std::string> flatTrieStrings;
		std::unordered_map<Node*, StringEntry> stringEntries;
		ItemIndexFactory & indexFactory;
		uint32_t curStrId;
		std::string::const_iterator strIt;
		std::string::const_iterator strBegin;
		std::string::const_iterator strEnd;
	};
protected:
	class FlatGST_TPNS: public Node::TemporalPrivateStorage {
	public:
		typedef std::vector<ItemIdType> ItemIdContainerType;
	private:
		ItemIdContainerType m_prefixIndex;
		ItemIdContainerType m_suffixPrefixIndex;
	public:
		FlatGST_TPNS() {}
		virtual ~FlatGST_TPNS() {}
		template<class TSortedContainer>
		void prefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(m_prefixIndex, m_prefixIndex, c);
		}
		template<class TSortedContainer>
		void suffixPrefixIndexInsert(const TSortedContainer & c) {
			mergeSortedContainer(m_suffixPrefixIndex, m_suffixPrefixIndex, c);
		}
		
		const ItemIdContainerType & prefixIndex() const { return m_prefixIndex; }
		const ItemIdContainerType & suffixPrefixIndex() const { return m_suffixPrefixIndex; }
	};

	class FlatGSTIndexEntry_TPNS: public Node::TemporalPrivateStorage {
	public:
		FlatGSTIndexEntry_TPNS(const IndexEntry & e) : e(e) {}
		virtual ~FlatGSTIndexEntry_TPNS() {}
		IndexEntry e;
	};
	
	struct FlatGSTIndexEntryMapper {
		sserialize::Static::DequeCreator<IndexEntry> & dc;
		FlatGSTIndexEntryMapper(sserialize::Static::DequeCreator<IndexEntry> & dc) : dc(dc) {}
		void operator()(Node * node) {
			FlatGSTIndexEntry_TPNS * p = dynamic_cast<FlatGSTIndexEntry_TPNS*>(node->temporalPrivateStorage);
			dc.put(p->e);
			node->deleteTemporalPrivateStorage();
		}
	};
private: //flat trie creation functions
	uint32_t createFlatTrieEntry(FlatTrieEntryConfig & flatTrieConfig);
	void fillFlatTrieIndexEntries(FlatTrieEntryConfig & flatTrieConfig, const FlatGSTConfig& config);
private:
	bool checkFlatTrieEquality(Node* node, std::string prefix, uint32_t& posInFTrie, const sserialize::Static::FlatGST& trie, bool checkIndex);
private:
	FlatTrie(const FlatTrie & other);
	FlatTrie & operator=(const FlatTrie & other);
public:
	FlatTrie();
	FlatTrie(bool caseSensitive, bool suffixTrie);
	virtual ~FlatTrie();
	void swap( MyBaseClass & baseTrie );
	void createStaticFlatTrie(FlatGSTConfig & config);
	bool checkFlatTrieEquality(const sserialize::Static::FlatGST & trie, bool checkIndex = false);
};

}}//end namespace

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::GeneralizedTrie::FlatTrie::StringEntry & source);
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::GeneralizedTrie::FlatTrie::IndexEntry & source);

#endif