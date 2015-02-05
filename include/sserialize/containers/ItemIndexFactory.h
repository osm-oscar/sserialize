#ifndef SSERIALIZE_ITEM_INDEX_FACTORY_H
#define SSERIALIZE_ITEM_INDEX_FACTORY_H
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/utilcontainerfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/MultiReaderSingleWriterLock.h>

namespace sserialize {

/** This class is a storage for multiple ItemIndex. It can create a file suitable for the Static::IndexStore.
	Live-Compression is currenty only available for VARUINT.
	If you want to create a store with HUFFMAN and/or LZO you have to do it afterwards.
	
	Compile-time debug options:
	DEBUG_CHECK_SERIALIZED_INDEX, DEBUG_CHECK_ALL
*/

class ItemIndexFactory {
public:
	typedef std::forward_list<OffsetType> DataOffsetContainer;
	typedef std::unordered_map< uint64_t, std::forward_list<OffsetType> > DataHashType;
	typedef std::unordered_map< uint64_t, uint32_t > OffsetToIdHashType;
	typedef std::vector<uint64_t > IdToOffsetsType;
	typedef std::vector<uint32_t> ItemIndexSizesContainer;
private:
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_indexStore;
	DataHashType m_hash;
	OffsetToIdHashType m_offsetsToId;
	IdToOffsetsType m_idToOffsets;
	ItemIndexSizesContainer m_idxSizes;
	std::atomic<uint32_t> m_hitCount;
	bool m_checkIndex;
	int8_t m_bitWidth;
	bool m_useRegLine;
	ItemIndex::Types m_type;
	Static::ItemIndexStore::IndexCompressionType m_compressionType;
	MultiReaderSingleWriterLock m_mapLock;
	MultiReaderSingleWriterLock m_dataLock;
	
	uint64_t hashFunc(const UByteArrayAdapter & v);
	uint64_t hashFunc(const std::vector< uint8_t >& v);
	///returns the position of the index or -1 if none was found @thread-safety: yes
	int64_t getIndex(const std::vector< uint8_t >& v, uint64_t & hv);
	bool indexInStore(const std::vector< uint8_t >& v, uint64_t offset);
	///adds the data of an index to store, @thread-safety: true
	uint32_t addIndex(const std::vector<uint8_t> & idx, OffsetType * indexOffset = 0, uint32_t idxSize = 0);
private://deleted functions
	ItemIndexFactory(const ItemIndexFactory & other);
	ItemIndexFactory & operator=(const ItemIndexFactory & other);
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
	
	inline ItemIndex indexByOffset(OffsetType offSet) const { return ItemIndex(m_indexStore+offSet, m_type); }
	
	inline ItemIndex indexById(uint32_t id) const { return indexByOffset(m_idToOffsets.at(id));}
	inline uint32_t idxSize(uint32_t id) const { return m_idxSizes.at(id); }

	template<class TSortedContainer>
	uint32_t addIndex(const TSortedContainer & idx, bool * ok = 0, OffsetType * indexOffset = 0);
	
	uint32_t addIndex(const ItemIndex & idx, bool * ok = 0, OffsetType * indexOffset = 0);
	
	inline UByteArrayAdapter & getIndexStore() { return m_indexStore;}
	inline uint32_t hitCount() { return m_hitCount; }
	
	UByteArrayAdapter getFlushedData();
	
	///Flushes the data, don't add indices afterwards
	///@return number of bytes from the beginning og the indexFile
	OffsetType flush();
	
	static UByteArrayAdapter::OffsetType compressWithHuffman(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	static UByteArrayAdapter::OffsetType compressWithVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	static UByteArrayAdapter::OffsetType compressWithLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	
	template<typename TSortedContainer>
	static bool create(const TSortedContainer& idx, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndex::Types type);
	
	template<typename TSortedContainer>
	static ItemIndex create(const TSortedContainer& idx, sserialize::ItemIndex::Types type);

};

template<class TSortedContainer>
uint32_t ItemIndexFactory::addIndex(const TSortedContainer & idx, bool * ok, OffsetType * indexOffset) {
	#if defined(DEBUG_CHECK_SERIALIZED_INDEX) || defined(DEBUG_CHECK_ALL)
	if (!std::is_sorted(idx.cbegin(), idx.cend()) || !sserialize::is_strong_monotone_ascending(idx.cbegin(), idx.cend())) {
		throw sserialize::CreationException("ItemIndexFactory: trying to add unsorted/and or non-strong-monotone index");
	}
	#endif
	bool mok = false;
	std::vector<uint8_t> s;
	if (m_type == ItemIndex::T_REGLINE) {
		mok = ItemIndexPrivateRegLine::create(idx, s, m_bitWidth, m_useRegLine);
	}
	else if (m_type == ItemIndex::T_WAH) {
		mok = true;
		std::vector<uint8_t> mys;
		UByteArrayAdapter dest(&mys);
		ItemIndexPrivateWAH::create(idx, dest);
		if (m_compressionType == Static::ItemIndexStore::IndexCompressionType::IC_VARUINT32) {
			UByteArrayAdapter nd(&s);
			for(std::vector<uint8_t>::const_iterator it(mys.begin()), end(mys.end()); it != end; it += 4) {
				uint32_t v = up_u32(&(*it));
				nd.putVlPackedUint32(v);
			}
		}
		else {
			s.swap(mys);
		}
	}
	else if (m_type == ItemIndex::T_DE) {
		mok = true;
		UByteArrayAdapter dest(&s);
		ItemIndexPrivateDE::create(idx, dest);
	}
	else if (m_type == ItemIndex::T_RLE_DE) {
		mok = true;
		UByteArrayAdapter dest(&s);
		ItemIndexPrivateRleDE::create(idx, dest);
	}
	else if (m_type == ItemIndex::T_SIMPLE) {
		mok = true;
		UByteArrayAdapter dest(&s);
		ItemIndexPrivateSimple::create(idx, dest);
	}
	else if (m_type == ItemIndex::T_NATIVE) {
		mok = true;
		UByteArrayAdapter dest(&s);
		detail::ItemIndexPrivate::ItemIndexPrivateNative::create(idx.cbegin(), idx.cend(), dest);
	}
	if (ok)
		*ok = mok;
#if defined(DEBUG_CHECK_SERIALIZED_INDEX) || defined(DEBUG_CHECK_ALL)
	if (mok) {
		uint64_t idxOf;
		uint32_t idxId = addIndex(s, &idxOf, idx.size());
		sserialize::ItemIndex sIdx = indexByOffset(idxOf);
		if (sIdx != idx) {
			std::cerr << "Broken index detected in ItemIndexFactory" << std::endl;
		}
		if (indexOffset)
			*indexOffset = idxOf;
		return idxId;
	}
#else
	if (mok) {
		return addIndex(s, indexOffset, idx.size());
	}
#endif
	else {
		return 0;
	}
}

template<typename TSortedContainer>
bool ItemIndexFactory::create(const TSortedContainer & idx, UByteArrayAdapter & dest, ItemIndex::Types type) {
	switch(type) {
	case ItemIndex::T_NATIVE:
		return sserialize::detail::ItemIndexPrivate::ItemIndexPrivateNative::create(idx, dest);
	case ItemIndex::T_SIMPLE:
		return ItemIndexPrivateSimple::create(idx, dest);
	case ItemIndex::T_REGLINE:
		return ItemIndexPrivateRegLine::create(idx, dest, -1, true);
	case ItemIndex::T_WAH:
		return ItemIndexPrivateWAH::create(idx, dest);
	case ItemIndex::T_DE:
		return ItemIndexPrivateDE::create(idx, dest);
	case ItemIndex::T_RLE_DE:
		return ItemIndexPrivateRleDE::create(idx ,dest);
	default:
		return false;
	}
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
