#ifndef SSERIALIZE_STATIC_ITEM_INDEX_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include "HuffmanDecoder.h"
#include <unordered_set>
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION 5

/*Version 5
 *
 *
 *------------------------------------------------------------------------------------------------------------------------------------------------------
 *VERSION|IndexTypes|IndexCompressionType|datalength| Data     |      Offsets    |Index sizes|HuffmanDecodeTable|DSTEntryLength|DecompressionSizeTable
 *------------------------------------------------------------------------------------------------------------------------------------------------------
 *   1   |    1     |        1           |OffsetType|datalength|SortedOffsetIndex|Array<u32> |HuffmanDecoder    |     u8       |CompactUintArray
 *
 * struct ItemIndexStore {
 *   uint<8> version;
 *   uint<16> indexTypes;
 *   uint<8> indexCompressionType;
 *   OffsetType dataLength;
 *   Array<Data, dataLength> data;
 *   SortedOffsetIndex offsets;
 *   Array< uint<32>, offsets.size> indexSizes;
 *   HuffmanDecoder huffmanDecodeTable;
 *   uint<8> decompressedSizeTableEntryLength;
 *   CompactUintArray<decompressedSizeTableEntryLength> decompressionSizeTable;
 *   CompactUintArray<log2(indexTypes)> indexTypeInfo;
 * };
 * 
 * 
 * There are 3 different compression modes which can be partialy mixed wit the following decompression order
 * [LZO][VARUINT32|HUFFMAN]
 * 
 * If indexTypes has more than one index active (popCount(indexTypes) > 1)
 * then the indexTypeInfo array is present and indicates for each entry the type
 *
 *
 */

namespace sserialize {
namespace Static {
namespace interfaces {

class ItemIndexStore: public RefCountObject {
public:
	ItemIndexStore() {}
	virtual ~ItemIndexStore() {}
	virtual OffsetType getSizeInBytes() const = 0;
	virtual uint32_t size() const = 0;
	///returns the types this store has, possibly multiple!
	virtual int indexTypes() const = 0;
	virtual ItemIndex::Types indexType(uint32_t pos) const = 0;
	virtual uint32_t compressionType() const = 0;
	virtual UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const = 0;
	virtual UByteArrayAdapter rawDataAt(uint32_t pos) const = 0;
	virtual ItemIndex at(uint32_t pos) const = 0;
	virtual uint32_t idxSize(uint32_t pos) const = 0;
	virtual std::ostream& printStats(std::ostream& out) const = 0;
	virtual std::ostream& printStats(std::ostream& out, std::function<bool(uint32_t)> filter) const = 0;
	virtual SortedOffsetIndex & getIndex() = 0;
	virtual const UByteArrayAdapter & getData() const = 0;
	virtual RCPtrWrapper<HuffmanDecoder> getHuffmanTree() const = 0;
	virtual UByteArrayAdapter getHuffmanTreeData() const = 0;
};

}//end namespace interfaces

class ItemIndexStoreId final {
public:
	typedef enum {
		ST_FIXED_LENGTH=0, ST_VARIABLE_LENGTH=1,
		ST_FL=ST_FIXED_LENGTH, ST_VL=ST_VARIABLE_LENGTH
	} SerializationType;
	
	struct FixedLengthTag {};
	struct VariableLengthTag {};
	
public:
	ItemIndexStoreId() : m_d(0xFFFFFFFFFF) {}
	ItemIndexStoreId(uint64_t id) : m_d(id) {}
	template<typename T_ST>
	ItemIndexStoreId(const sserialize::UByteArrayAdapter & d, T_ST dummy = T_ST());
	ItemIndexStoreId(const sserialize::UByteArrayAdapter & d, SerializationType t);
	~ItemIndexStoreId() {}
	template<typename T_ST>
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes(T_ST dummy = T_ST()) const;
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes(SerializationType t) const;
	operator uint32_t() const { return sserialize::narrow_check<uint32_t>( m_d ); }
private:
	uint64_t m_d;
};

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::FixedLengthTag);

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::VariableLengthTag);

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::FixedLengthTag) const;

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::VariableLengthTag) const;

class ItemIndexStore {
public:
	typedef enum {IC_NONE=0, IC_VARUINT32=1, IC_HUFFMAN=2, IC_LZO=4} IndexCompressionType;
	typedef uint32_t SizeType;
	typedef SizeType IdType;
	static constexpr IdType npos = std::numeric_limits<IdType>::max();
private:
	RCPtrWrapper<interfaces::ItemIndexStore> m_priv;
protected:
	const RCPtrWrapper<interfaces::ItemIndexStore> & priv() const { return m_priv; }
	RCPtrWrapper<interfaces::ItemIndexStore> & priv() { return m_priv; }
public:
	ItemIndexStore();
	ItemIndexStore(const sserialize::UByteArrayAdapter & data);
	explicit ItemIndexStore(interfaces::ItemIndexStore * base);
	~ItemIndexStore() {}
	inline OffsetType getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline uint32_t size() const { return priv()->size(); }
	inline int indexTypes() const { return priv()->indexTypes(); }
	inline ItemIndex::Types indexType(uint32_t pos) const { return priv()->indexType(pos); }
	inline IndexCompressionType compressionType() const { return (IndexCompressionType) priv()->compressionType(); }
	inline UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const { return priv()->dataSize(pos); }
	inline UByteArrayAdapter rawDataAt(uint32_t pos) const { return priv()->rawDataAt(pos); }
	inline ItemIndex at(uint32_t pos) const { return priv()->at(pos);}
	inline uint32_t idxSize(uint32_t pos) const { return priv()->idxSize(pos); }
	inline std::ostream& printStats(std::ostream& out) const { return priv()->printStats(out); }
	inline std::ostream& printStats(std::ostream& out, std::function<bool(uint32_t)> filter) const { return priv()->printStats(out, filter);}
	inline SortedOffsetIndex & getIndex() { return priv()->getIndex();}
	inline const UByteArrayAdapter & getData() const { return priv()->getData(); }
	inline RCPtrWrapper<HuffmanDecoder> getHuffmanTree() const { return priv()->getHuffmanTree(); }
	inline UByteArrayAdapter getHuffmanTreeData() const { return priv()->getHuffmanTreeData();}
};

namespace detail {

/** The first index id is ALWAYS the empty index*/
class ItemIndexStore: public interfaces::ItemIndexStore {
private:
	typedef sserialize::Static::ItemIndexStore::IndexCompressionType IndexCompressionType;
private:
	class LZODecompressor: public RefCountObject {
		CompactUintArray m_data;
	public:
		LZODecompressor();
		LZODecompressor(const sserialize::UByteArrayAdapter & data);
		virtual ~LZODecompressor();
		OffsetType getSizeInBytes() const;
		UByteArrayAdapter decompress(uint32_t id, const sserialize::UByteArrayAdapter & src) const;
	};
private:
	uint8_t m_version;
	int m_type;
	IndexCompressionType m_compression;
	UByteArrayAdapter m_data;
	SortedOffsetIndex m_index;
	Static::Array<uint32_t> m_idxSizes;
	RCPtrWrapper<HuffmanDecoder> m_hd;
	RCPtrWrapper<LZODecompressor> m_lzod;
	CompactUintArray m_idxTypeInfo;
public:
	ItemIndexStore();
	ItemIndexStore(sserialize::UByteArrayAdapter data);
	virtual ~ItemIndexStore();
	virtual OffsetType getSizeInBytes() const override;
	virtual uint32_t size() const override;
	virtual int indexTypes() const override;
	virtual ItemIndex::Types indexType(uint32_t pos) const override;
	virtual uint32_t compressionType() const  override { return m_compression; }
	virtual UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const override;
	virtual UByteArrayAdapter rawDataAt(uint32_t pos) const override;
	virtual ItemIndex at(uint32_t pos) const override;
	virtual inline uint32_t idxSize(uint32_t pos) const override { return m_idxSizes.at(pos); }
	virtual std::ostream& printStats(std::ostream& out) const override;
	virtual std::ostream& printStats(std::ostream& out, std::function<bool(uint32_t)> filter) const override;
	virtual inline SortedOffsetIndex & getIndex() override { return m_index;}
	virtual inline const UByteArrayAdapter & getData() const override { return m_data; }
	virtual inline RCPtrWrapper<HuffmanDecoder> getHuffmanTree() const override { return m_hd; }
	virtual UByteArrayAdapter getHuffmanTreeData() const override;
};

}//end namespace detail

}}//end namespace

#endif
