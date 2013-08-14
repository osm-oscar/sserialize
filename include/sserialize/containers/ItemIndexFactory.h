#ifndef ITEM_INDEX_FACTORY_H
#define ITEM_INDEX_FACTORY_H
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <google/sparse_hash_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/pack_unpack_functions.h>


namespace sserialize {

/** This class is a storage for multiple ItemIndex. It can create a file suitable for the Static::IndexStore */

class ItemIndexFactory {
public:
	typedef std::forward_list<OffsetType> DataOffsetContainer;
	typedef std::unordered_map< uint64_t, std::forward_list<OffsetType> > DataHashType;
	typedef std::unordered_map< uint64_t, uint32_t > OffsetToIdHashType;
private:
	uint32_t m_indexIdCounter;
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_indexStore;
	DataHashType m_hash;
	OffsetToIdHashType m_offsetsToId;
	uint32_t m_hitCount;
	bool m_checkIndex;
	int8_t m_bitWidth;
	bool m_useRegLine;
	ItemIndex::Types m_type;
	Static::ItemIndexStore::IndexCompressionType m_compressionType;
	
	uint64_t hashFunc(const UByteArrayAdapter & v);
	uint64_t hashFunc(const std::vector< uint8_t >& v);
	int64_t getIndex(const std::vector< uint8_t >& v, uint64_t & hv);
	bool indexInStore(const std::vector< uint8_t >& v, uint64_t offset);
	
public:
	ItemIndexFactory(bool memoryBase = false);
	~ItemIndexFactory();
	uint32_t size() { return m_indexIdCounter;}
	ItemIndex::Types type() const { return m_type; }
	UByteArrayAdapter at(OffsetType offset) const;
	///Sets the type. should not be called after having added indices
	void setType(ItemIndex::Types type) { m_type = type;}
	///create the index Store at the beginning of data
	void setIndexFile(UByteArrayAdapter data);
	///from IndexStore set after setIndexFile
	void fromIndexStore(const sserialize::Static::ItemIndexStore & store);
	
	void setCheckIndex(bool checkIndex) { m_checkIndex = checkIndex;}
	void setBitWith(int8_t bitWidth) { m_bitWidth = bitWidth; }
	void setRegline(bool useRegLine) { m_useRegLine = useRegLine; }
	
	inline ItemIndex getIndex(OffsetType offSet) const { return ItemIndex(m_indexStore+offSet, m_type); }
	
	uint32_t addIndex(const std::vector<uint8_t> & idx, OffsetType * indexOffset = 0);

	template<class TSortedContainer>
	uint32_t addIndex(const TSortedContainer & idx, bool * ok = 0, OffsetType * indexOffset = 0) {
		bool mok = false;
		std::vector<uint8_t> s;
		if (m_type == ItemIndex::T_REGLINE)
			mok = ItemIndexPrivateRegLine::create(idx, s, m_bitWidth, m_useRegLine);
		else if (m_type == ItemIndex::T_WAH) {
			mok = true;
			std::vector<uint8_t> mys;
			UByteArrayAdapter dest(&mys);
			ItemIndexPrivateWAH::create(idx, dest);
			if (m_compressionType == Static::ItemIndexStore::IC_VARUINT32) {
				UByteArrayAdapter nd(&s);
				for(std::vector<uint8_t>::const_iterator it(mys.begin()), end(mys.end()); it != end; it += 4) {
					uint32_t v = unPack_uint32_t(&(*it));
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
		if (ok)
			*ok = mok;
#if defined(DEBUG_CHECK_SERIALIZED_INDEX) || defined(DEBUG_CHECK_ALL)
		if (mok) {
			uint64_t idxOf;
			uint32_t idxId = addIndex(s, &idxOf);
			sserialize::ItemIndex sIdx = getIndex(idxOf);
			if (sIdx != idx) {
				std::cerr << "Broken index detected in ItemIndexFactory" << std::endl;
			}
			if (indexOffset)
				*indexOffset = idxOf;
			return idxId;
		}
#else
		if (mok) {
			return addIndex(s, indexOffset);
		}
#endif
		else {
			return 0;
		}
	}
	
	uint32_t addIndex(const std::unordered_set<uint32_t> & idx, bool * ok = 0, OffsetType * indexOffset = 0);
	
	
	///This only works in conjunction with the IC_NONE
	template<typename T_RANDOM_ACCESS_CONTAINER_ITERATOR>
	uint32_t addMergedIndex(T_RANDOM_ACCESS_CONTAINER_ITERATOR begin, const T_RANDOM_ACCESS_CONTAINER_ITERATOR & end, OffsetType & indexOffset) {
		std::vector<ItemIndex> indices;
		indices.reserve(end-begin);
		for(; begin != end; ++begin) {
			ItemIndex idx = getIndex(*begin);
			if (idx.size())
				indices.push_back( idx );
		}
		std::vector<uint8_t> d;
		UByteArrayAdapter dAdap(&d, false);
		if (sserialize::ItemIndex::uniteSameResult(type()) && indices.size() < 32) {
			std::vector<uint32_t> midx;
			ItemIndex::unite(indices).insertInto(std::back_inserter(midx));
			return addIndex(midx, 0, &indexOffset);
		}
		else {
			sserialize::DynamicBitSet dbs(dAdap);
			for(std::vector<ItemIndex>::const_iterator it(indices.begin()), end(indices.end()); it != end; ++it) {
				it->putInto(dbs);
			}
			std::vector<uint32_t> midx;
			dbs.putInto(std::back_inserter(midx));
			return addIndex(midx, 0, &indexOffset);
		}
	}
	
	inline UByteArrayAdapter & getIndexStore() { return m_indexStore;}
	inline uint32_t hitCount() { return m_hitCount; }
	
	UByteArrayAdapter getFlushedData();
	
	///Flushes the data, don't add indices afterwards
	///@return number of bytes from the beginning og the indexFile
	OffsetType flush();
	
	static UByteArrayAdapter::OffsetType compressWithHuffman(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	static UByteArrayAdapter::OffsetType compressWithVarUint(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);
	static UByteArrayAdapter::OffsetType compressWithLZO(sserialize::Static::ItemIndexStore & store, UByteArrayAdapter & dest);

};

}//end namespace

#endif
