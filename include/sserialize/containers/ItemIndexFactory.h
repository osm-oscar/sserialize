#ifndef SSERIALIZE_ITEM_INDEX_FACTORY_H
#define SSERIALIZE_ITEM_INDEX_FACTORY_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/algorithm/hashspecializations.h>

namespace sserialize {

/** This class is a storage for multiple ItemIndex. It can create a file suitable for the Static::IndexStore.
	Live-Compression is currenty only available for VARUINT.
	If you want to create a store with HUFFMAN and/or LZO you have to do it afterwards.
*/

class ItemIndexFactory {
public:
	class IndexId {
	public:
		IndexId() = default;
		IndexId(uint32_t v) : m_v(v) {}
		IndexId(IndexId const & other) = default;
		IndexId & operator=(IndexId const & other) = default;
	public:
		inline operator uint32_t() const {
			SSERIALIZE_CHEAP_ASSERT(valid());
			return m_v;
		}
		inline bool valid() const { return m_v != INVALID;}
	private:
		enum Types {INVALID=0xFFFFFFFF};
	private:
		uint32_t m_v{INVALID};
	};
	typedef ShaHasherDigestData DataHashKey;
	typedef std::unordered_map<DataHashKey, IndexId> DataHashType; //Hash->id
	typedef sserialize::MMVector<uint64_t > IdToOffsetsType;
	typedef sserialize::MMVector<uint32_t> ItemIndexSizesContainer;
	typedef sserialize::MMVector<uint8_t> ItemIndexTypesContainer;
public://deleted functions
	ItemIndexFactory(const ItemIndexFactory & other) = delete;
	ItemIndexFactory & operator=(const ItemIndexFactory & other) = delete;
public:
	ItemIndexFactory(bool memoryBase = false);
	ItemIndexFactory(ItemIndexFactory && other);
	~ItemIndexFactory();
	ItemIndexFactory & operator=(ItemIndexFactory && other);
	uint32_t size() { return (uint32_t)m_idToOffsets.size();}
	int types() const;
	ItemIndex::Types type(uint32_t pos) const;
	Static::ItemIndexStore::IndexCompressionType compressionType() const { return m_compressionType; }
	UByteArrayAdapter at(OffsetType offset) const;
	///Sets the type of the indexes. If T_MULTIPLE is set, then the index with the smallest size is chosen to be stored
	void setType(int type);
	///create the ItemIndexStore at the beginning of data
	void setIndexFile(UByteArrayAdapter data);
	///insert IndexStore, threadCount > 1 change the order of index ids
	std::vector<uint32_t> insert(const sserialize::Static::ItemIndexStore & store, uint32_t threadCount = 1);
	
	void setCheckIndex(bool checkIndex) { m_checkIndex = checkIndex;}
	//default is on
	void setDeduplication(bool dedup) { m_useDeduplication  = dedup; }
	
	void recalculateDeduplicationData();
	
	inline UByteArrayAdapter::SizeType dataSizeById(uint32_t id) const {
		return (id+1 < m_idToOffsets.size() ? m_idToOffsets.at(id+1) : m_idToOffsets.size()) - m_idToOffsets.at(id);
	}
	inline UByteArrayAdapter indexDataById(uint32_t id) const { return sserialize::UByteArrayAdapter(m_indexStore, m_idToOffsets.at(id), dataSizeById(id)); }
	inline ItemIndex indexById(uint32_t id) const { return ItemIndex(indexDataById(id), type(id)); }
	inline uint32_t idxSize(uint32_t id) const { return m_idxSizes.at(id); }

	template<class TSortedContainer>
	uint32_t addIndex(const TSortedContainer & idx);
	
	uint32_t addIndex(const ItemIndex & idx);
	
	inline UByteArrayAdapter & getIndexStore() { return m_indexStore;}
	///Make sure that you do not read any indexes while adding an index to the factory
	///Adding an index may cause a resize in the storage back end which invalidates memory regions
	///This is only a problem for multi-threaded usage where one thread reads an index and another one adds an index which causes the resize
	sserialize::Static::ItemIndexStore asItemIndexStore();
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
	
	///@return the type created if type & ItemIndex::T_MULTIPLE, ItemIndex::T_NULL if creation failed
	template<typename TSortedContainer>
	static ItemIndex::Types create(const TSortedContainer& idx, sserialize::UByteArrayAdapter& dest, int type, ItemIndex::CompressionLevel cl = ItemIndex::CL_DEFAULT);
	
	template<typename TSortedContainer>
	static ItemIndex create(const TSortedContainer& idx, int type, ItemIndex::CompressionLevel cl = ItemIndex::CL_DEFAULT);
	
	static ItemIndex range(uint32_t begin, uint32_t end, uint32_t step, int type);
private:
	using MutexType = std::mutex;
	using WriteLock = std::unique_lock<MutexType>;
	using ReadLock = std::unique_lock<MutexType>;
private:
	DataHashKey hashFunc(const UByteArrayAdapter & v);
	DataHashKey hashFunc(const std::vector< uint8_t >& v);
	///returns the id of the index or -1 if none was found @thread-safety: yes
	int64_t getIndex(const std::vector< uint8_t >& v, DataHashKey & hv);
	bool indexInStore(const std::vector< uint8_t >& v, uint32_t id);
	///adds the data of an index to store @thread-safety: true
	uint32_t addIndex(const std::vector<uint8_t> & idx, uint32_t idxSize, ItemIndex::Types type);
private:
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_indexStore;
	DataHashType m_hash;
	IdToOffsetsType m_idToOffsets;
	ItemIndexSizesContainer m_idxSizes;
	ItemIndexTypesContainer m_idxTypes;
	std::atomic<uint64_t> m_hitCount;
	bool m_checkIndex;
	bool m_useDeduplication;
	int m_type;
	Static::ItemIndexStore::IndexCompressionType m_compressionType;
	MutexType m_mapLock;
	MutexType m_dataLock;
};

template<class TSortedContainer>
uint32_t ItemIndexFactory::addIndex(const TSortedContainer & idx) {
	std::vector<uint8_t> s;
	UByteArrayAdapter ds(&s, false);
	ItemIndex::Types type = create(idx, ds, m_type);
	if (type != ItemIndex::T_NULL) {
		if (m_checkIndex) {
			sserialize::ItemIndex sidx(ds, type);
			if (idx != sidx) {
				throw sserialize::CreationException("Created index does not match source");
			}
		}
		return addIndex(s, narrow_check<uint32_t>(idx.size()), type);
	}
	else {
		throw sserialize::CreationException("Failed to create index");
	}
}

template<typename TSortedContainer>
ItemIndex::Types ItemIndexFactory::create(const TSortedContainer & idx, UByteArrayAdapter & dest, int type, ItemIndex::CompressionLevel cl) {
	#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	if (!std::is_sorted(idx.cbegin(), idx.cend())) {
		throw sserialize::CreationException("ItemIndexFactory: trying to add unsorted index");	
	}
	if (!sserialize::is_strong_monotone_ascending(idx.cbegin(), idx.cend())) {
			throw sserialize::CreationException("ItemIndexFactory: trying to add non-strong-monotone index");
	}
	UByteArrayAdapter::OffsetType destBegin = dest.tellPutPtr();
	#endif

	auto c = [&idx, cl](UByteArrayAdapter & dest, int type) -> bool {
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
			ok = ItemIndexPrivateRleDE::create(idx, dest);
			break;
		case ItemIndex::T_ELIAS_FANO:
			ok = ItemIndexPrivateEliasFano::create(idx, dest);
			break;
		case ItemIndex::T_PFOR:
			ok = ItemIndexPrivatePFoR::create(idx, dest, cl);
			break;
		case ItemIndex::T_FOR:
			ok = ItemIndexPrivateFoR::create(idx, dest, cl);
			break;
		default:
			break;
		}
		return ok;
	};
	if (type & ItemIndex::T_MULTIPLE) {
		type &= ~ItemIndex::T_MULTIPLE;
		int ct = 1; //T_SIMPLE
		int bestType = ItemIndex::T_NULL;
		sserialize::UByteArrayAdapter::SizeType bestSize = std::numeric_limits<sserialize::UByteArrayAdapter::SizeType>::max();
		sserialize::UByteArrayAdapter tmp(new std::vector<uint8_t>(), true);
		while(type) {
			if (type & 0x1) {
				bool ok = c(tmp, ItemIndex::Types(ct));
				if (ok && tmp.size() < bestSize) {
					bestSize = tmp.size();
					bestType = ct;
				}
				tmp.resize(0);
			}
			type >>= 1;
			ct <<= 1;
		}
		type = bestType;
	}
	bool ok = c(dest, type);
	type = ok ? type : ItemIndex::T_NULL;
#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	if (type != ItemIndex::T_NULL) {
		sserialize::ItemIndex sIdx(dest+destBegin, ItemIndex::Types(type));
		ok = (sIdx == idx);
	}
#endif
	return ItemIndex::Types(type);
}

template<typename TSortedContainer>
ItemIndex ItemIndexFactory::create(const TSortedContainer & idx, int type, ItemIndex::CompressionLevel cl) {
	switch (type) {
	case ItemIndex::T_EMPTY:
		return ItemIndex();
	case ItemIndex::T_STL_DEQUE:
		return ItemIndex( std::deque<uint32_t>(idx.begin(), idx.end()) );
	case ItemIndex::T_STL_VECTOR:
		return ItemIndex( std::vector<uint32_t>(idx.begin(), idx.end()) );
	default:
	{
		UByteArrayAdapter tmp(UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
		ItemIndex::Types rt = create(idx, tmp, type, cl);
		if (rt != ItemIndex::T_NULL) {
			tmp.resetPtrs();
			return ItemIndex(tmp, rt);
		}
	}
	}
	throw sserialize::CreationException("Could not create index with type " + std::to_string(type));
	return ItemIndex();
}

namespace detail {

class ItemIndexStoreFromFactory: public sserialize::Static::interfaces::ItemIndexStore {
private:
	sserialize::ItemIndexFactory * m_idxFactory;
	sserialize::Static::SortedOffsetIndex m_dummyOffsets;
public:
	ItemIndexStoreFromFactory() : m_idxFactory(0) {}
	ItemIndexStoreFromFactory(sserialize::ItemIndexFactory * idxFactory) : m_idxFactory(idxFactory) {}
	virtual ~ItemIndexStoreFromFactory() {}
	virtual OffsetType getSizeInBytes() const override;
	virtual uint32_t size() const override;
	virtual int indexTypes() const override;
	virtual ItemIndex::Types indexType(uint32_t pos) const override;
	virtual uint32_t compressionType() const override;
	virtual UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const override;
	virtual UByteArrayAdapter rawDataAt(uint32_t pos) const override;
	virtual ItemIndex at(uint32_t pos) const override;
	virtual uint32_t idxSize(uint32_t pos) const override;
	virtual std::ostream& printStats(std::ostream& out) const override;
	virtual std::ostream& printStats(std::ostream& out, std::function<bool(uint32_t)> filter) const override;
	virtual sserialize::Static::SortedOffsetIndex & getIndex() override;
	virtual const UByteArrayAdapter & getData() const override;
	virtual RCPtrWrapper<Static::HuffmanDecoder> getHuffmanTree() const override;
	virtual UByteArrayAdapter getHuffmanTreeData() const override;
};

};

}//end namespace

#endif
