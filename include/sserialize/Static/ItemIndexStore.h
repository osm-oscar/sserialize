#ifndef SSERIALIZE_STATIC_ITEM_INDEX_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include "HuffmanDecoder.h"
#include <unordered_set>
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION 4

/*Version 4
 *
 *
 *------------------------------------------------------------------------------------------------------------------------------------------------------
 *VERSION|IndexTypes|IndexCompressionType|datalength| Data     |      Offsets    |Index sizes|HuffmanDecodeTable|DSTEntryLength|DecompressionSizeTable
 *------------------------------------------------------------------------------------------------------------------------------------------------------
 *   1   |    1     |        1           |OffsetType|datalength|SortedOffsetIndex|Array<u32> |HuffmanDecoder    |     u8       |CompactUintArray
 *
 * 
 * There are 3 different compression modes which can be partialy mixed wit the following decompression order
 * [LZO][VARUINT32|HUFFMAN]
 *
 *
 */

namespace sserialize {
namespace Static {
namespace detail {

/** The first index id is ALWAYS the empty index*/
class ItemIndexStore: public RefCountObject {
public:
	typedef enum {IC_NONE=0, IC_VARUINT32=1, IC_HUFFMAN=2, IC_LZO=4} IndexCompressionType;
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
	ItemIndex::Types m_type;
	IndexCompressionType m_compression;
	UByteArrayAdapter m_data;
	SortedOffsetIndex m_index;
	Static::Array<uint32_t> m_idxSizes;
	RCPtrWrapper<HuffmanDecoder> m_hd;
	RCPtrWrapper<LZODecompressor> m_lzod;
public:
	ItemIndexStore();
	ItemIndexStore(sserialize::UByteArrayAdapter data);
	~ItemIndexStore();
	OffsetType getSizeInBytes() const;
	uint32_t size() const;
	ItemIndex::Types indexType() const { return m_type; }
	IndexCompressionType compressionType() const { return m_compression; }
	UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const;
	UByteArrayAdapter rawDataAt(uint32_t pos) const;
	ItemIndex at(uint32_t pos) const;
	ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const;
	ItemIndex hierachy(const std::deque< uint32_t >& offsets) const;
	inline uint32_t idxSize(uint32_t pos) const { return m_idxSizes.at(pos); }
	std::ostream& printStats(std::ostream& out) const;
	std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const;
	SortedOffsetIndex & getIndex() { return m_index;}
	const UByteArrayAdapter & getData() const { return m_data; }
	RCPtrWrapper<HuffmanDecoder> getHuffmanTree() const { return m_hd; }
	UByteArrayAdapter getHuffmanTreeData() const;
};
}

class ItemIndexStore {
public:
	typedef detail::ItemIndexStore::IndexCompressionType IndexCompressionType;
private:
	RCPtrWrapper<detail::ItemIndexStore> m_priv;
protected:
	const RCPtrWrapper<detail::ItemIndexStore> & priv() const { return m_priv; }
	RCPtrWrapper<detail::ItemIndexStore> & priv() { return m_priv; }
public:
	ItemIndexStore() : m_priv(new detail::ItemIndexStore()) {}
	ItemIndexStore(sserialize::UByteArrayAdapter data) : m_priv(new detail::ItemIndexStore(data)) {}
	~ItemIndexStore() {}
	inline OffsetType getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline uint32_t size() const { return priv()->size(); }
	inline ItemIndex::Types indexType() const { return priv()->indexType(); }
	inline IndexCompressionType compressionType() const { return priv()->compressionType(); }
	inline UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const { return priv()->dataSize(pos); }
	inline UByteArrayAdapter rawDataAt(uint32_t pos) const { return priv()->rawDataAt(pos); }
	inline ItemIndex at(uint32_t pos) const { return priv()->at(pos);}
	inline ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const { return priv()->at(pos, realIdIndex);}
	inline ItemIndex hierachy(const std::deque< uint32_t >& offsets) const { return priv()->hierachy(offsets); }
	inline uint32_t idxSize(uint32_t pos) const { return priv()->idxSize(pos); }
	inline std::ostream& printStats(std::ostream& out) const { return priv()->printStats(out); }
	inline std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const { return priv()->printStats(out, indexIds);}
	inline SortedOffsetIndex & getIndex() { return priv()->getIndex();}
	inline const UByteArrayAdapter & getData() const { return priv()->getData(); }
	inline RCPtrWrapper<HuffmanDecoder> getHuffmanTree() const { return priv()->getHuffmanTree(); }
	inline UByteArrayAdapter getHuffmanTreeData() const { return priv()->getHuffmanTreeData();}
};

}}//end namespace

#endif