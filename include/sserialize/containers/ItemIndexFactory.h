#ifndef SSERIALIZE_ITEM_INDEX_FACTORY_H
#define SSERIALIZE_ITEM_INDEX_FACTORY_H
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/utilcontainerfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/mt/MultiReaderSingleWriterLock.h>
#include <sserialize/templated/MMVector.h>

namespace sserialize {

/** This class is a storage for multiple ItemIndex. It can create a file suitable for the Static::IndexStore.
	Live-Compression is currenty only available for VARUINT.
	If you want to create a store with HUFFMAN and/or LZO you have to do it afterwards.
	
	Compile-time debug options:
	DEBUG_CHECK_SERIALIZED_INDEX, DEBUG_CHECK_ALL
*/

class ItemIndexFactory {
public:
	struct DataOffsetEntry {
		uint64_t prev;
		uint32_t id;
	} __attribute__ ((packed));
	typedef std::unordered_map< uint64_t, uint64_t > DataHashType;
	typedef sserialize::MMVector<DataOffsetEntry> DataOffsetContainer;
	typedef sserialize::MMVector<uint64_t > IdToOffsetsType;
	typedef sserialize::MMVector<uint32_t> ItemIndexSizesContainer;
private:
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_indexStore;
	DataHashType m_hash;
	DataOffsetContainer m_dataOffsets;
	IdToOffsetsType m_idToOffsets;
	ItemIndexSizesContainer m_idxSizes;
	std::atomic<uint32_t> m_hitCount;
	bool m_checkIndex;
	int8_t m_bitWidth;
	bool m_useRegLine;
	bool m_useDeduplication;
	ItemIndex::Types m_type;
	Static::ItemIndexStore::IndexCompressionType m_compressionType;
	MultiReaderSingleWriterLock m_mapLock;
	MultiReaderSingleWriterLock m_dataLock;
	
	uint64_t hashFunc(const UByteArrayAdapter & v);
	uint64_t hashFunc(const std::vector< uint8_t >& v);
	///returns the id of the index or -1 if none was found @thread-safety: yes
	int64_t getIndex(const std::vector< uint8_t >& v, uint64_t & hv);
	bool indexInStore(const std::vector< uint8_t >& v, uint32_t id);
	///adds the data of an index to store, @thread-safety: true
	uint32_t addIndex(const std::vector<uint8_t> & idx, uint32_t idxSize = 0);
	inline ItemIndex indexByOffset(OffsetType offSet) const { return ItemIndex(m_indexStore+offSet, m_type); }
public://deleted functions
	ItemIndexFactory(const ItemIndexFactory & other) = delete;
	ItemIndexFactory & operator=(const ItemIndexFactory & other) = delete;
public:
	ItemIndexFactory(bool memoryBase = false);
	ItemIndexFactory(ItemIndexFactory && other);
	~ItemIndexFactory();
	ItemIndexFactory & operator=(ItemIndexFactory && other);
	uint32_t size() { return m_idToOffsets.size();}
	ItemIndex::Types type() const { return m_type; }
	UByteArrayAdapter at(OffsetType offset) const;
	///Sets the type. should not be called after having added indices
	void setType(ItemIndex::Types type) { m_type = type;}
	///create the index Store at the beginning of data
	void setIndexFile(UByteArrayAdapter data);
	///insert IndexStore
	std::vector<uint32_t> insert(const sserialize::Static::ItemIndexStore & store);
	
	void setCheckIndex(bool checkIndex) { m_checkIndex = checkIndex;}
	void setBitWith(int8_t bitWidth) { m_bitWidth = bitWidth; }
	void setRegline(bool useRegLine) { m_useRegLine = useRegLine; }
	//default is on
	void setDeduplication(bool dedup) { m_useDeduplication  = dedup; }
		
	inline ItemIndex indexById(uint32_t id) const { return indexByOffset(m_idToOffsets.at(id));}
	inline uint32_t idxSize(uint32_t id) const { return m_idxSizes.at(id); }

	template<class TSortedContainer>
	uint32_t addIndex(const TSortedContainer & idx);
	
	uint32_t addIndex(const ItemIndex & idx);
	
	inline UByteArrayAdapter & getIndexStore() { return m_indexStore;}
	inline uint32_t hitCount() { return m_hitCount; }
	
	UByteArrayAdapter getFlushedData();
	
	///Flushes the data, don't add indices afterwards
	///@return number of bytes from the beginning og the indexFile
	OffsetType flush();
	
	///BROKEN
	static UByteArrayAdapter::OffsetType compressWithHuffman(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	///BROKEN
	static UByteArrayAdapter::OffsetType compressWithVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	static UByteArrayAdapter::OffsetType compressWithLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	
	template<typename TSortedContainer>
	static bool create(const TSortedContainer& idx, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndex::Types type);
	
	template<typename TSortedContainer>
	static ItemIndex create(const TSortedContainer& idx, sserialize::ItemIndex::Types type);

};

template<class TSortedContainer>
uint32_t ItemIndexFactory::addIndex(const TSortedContainer & idx) {
	std::vector<uint8_t> s;
	UByteArrayAdapter ds(&s, false);
	bool mok = create(idx, ds, m_type);
	assert(mok);
	if (mok) {
		return addIndex(s, idx.size());
	}
	else {
		return 0;
	}
}

template<typename TSortedContainer>
bool ItemIndexFactory::create(const TSortedContainer & idx, UByteArrayAdapter & dest, ItemIndex::Types type) {
	#if defined(DEBUG_CHECK_INDEX_SERIALIZATION) || defined(DEBUG_CHECK_ALL)
	if (!std::is_sorted(idx.cbegin(), idx.cend()) || !sserialize::is_strong_monotone_ascending(idx.cbegin(), idx.cend())) {
		throw sserialize::CreationException("ItemIndexFactory: trying to add unsorted/and or non-strong-monotone index");
	}
	UByteArrayAdapter::OffsetType destBegin = dest.tellPutPtr();
	#endif
	bool ok = false;
	switch(type) {
	case ItemIndex::T_NATIVE:
		ok = sserialize::detail::ItemIndexPrivate::ItemIndexPrivateNative::create(idx, dest);
		break;
	case ItemIndex::T_SIMPLE:
		ok = ItemIndexPrivateSimple::create(idx, dest);
		break;
	case ItemIndex::T_REGLINE:
		ok = ItemIndexPrivateRegLine::create(idx, dest, -1, true);
		break;
	case ItemIndex::T_WAH:
		ok = ItemIndexPrivateWAH::create(idx, dest);
		break;
	case ItemIndex::T_DE:
		ok = ItemIndexPrivateDE::create(idx, dest);
		break;
	case ItemIndex::T_RLE_DE:
		ok = ItemIndexPrivateRleDE::create(idx ,dest);
		break;
	default:
		break;
	}
#if defined(DEBUG_CHECK_INDEX_SERIALIZATION) || defined(DEBUG_CHECK_ALL)
	if (ok) {
		sserialize::ItemIndex sIdx(dest+destBegin, type);
		ok = (sIdx == idx);
	}
#endif
	return ok;
}

template<typename TSortedContainer>
ItemIndex ItemIndexFactory::create(const TSortedContainer & idx, ItemIndex::Types type) {
	UByteArrayAdapter tmp(UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	if (create(idx, tmp, type)) {
		tmp.resetPtrs();
		return ItemIndex(tmp, type);
	}
	return ItemIndex();
}


}//end namespace

#endif
