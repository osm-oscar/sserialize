#ifndef SSERIALIZE_STATIC_ITEM_INDEX_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include "HuffmanDecoder.h"
#include <unordered_set>
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION 3

/*Version 3
 *
 *
 *-----------------------------------------------------------------------------------------------------------------------------------------
 *VERSION|IndexTypes|IndexCompressionType|datalength| Data     |      Offsets    |HuffmanDecodeTable|DSTEntryLength|DecompressionSizeTable
 *-----------------------------------------------------------------------------------------------------------------------------------------
 *   1   |    1     |        1           |OffsetType|datalength|SortedOffsetIndex|HuffmanDecoder    |     u8       |CompactUintArray
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

/** The first index id is ALWAY the empty index*/
class ItemIndexStore {
public:
	typedef enum {IC_NONE=0, IC_VARUINT32=1, IC_HUFFMAN=2, IC_LZO=4} IndexCompressionType;
private:
	class LZODecompressor {
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
	std::shared_ptr<HuffmanDecoder> m_hd;
	std::shared_ptr<LZODecompressor> m_lzod;
public:
	ItemIndexStore();
	ItemIndexStore(sserialize::UByteArrayAdapter data);
	~ItemIndexStore();
	OffsetType getSizeInBytes() const;
	uint32_t size() const;
	ItemIndex::Types indexType() const { return m_type; }
	IndexCompressionType compressionType() const { return m_compression; }
	UByteArrayAdapter rawDataAt(uint32_t pos) const;
	ItemIndex at(uint32_t pos) const;
	ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const;
	ItemIndex hierachy(const std::deque< uint32_t >& offsets) const;
	std::ostream& printStats(std::ostream& out) const;
	std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const;
	SortedOffsetIndex & getIndex() { return m_index;}
	const UByteArrayAdapter & getData() const { return m_data; }
	std::shared_ptr<HuffmanDecoder> getHuffmanTree() const { return m_hd; }
	UByteArrayAdapter getHuffmanTreeData() const;
};
}

class ItemIndexStore {
public:
	typedef detail::ItemIndexStore::IndexCompressionType IndexCompressionType;
private:
	std::shared_ptr<detail::ItemIndexStore> m_priv;
protected:
	const std::shared_ptr<detail::ItemIndexStore> & priv() const { return m_priv; }
	std::shared_ptr<detail::ItemIndexStore> & priv() { return m_priv; }
public:
	ItemIndexStore() : m_priv(new detail::ItemIndexStore()) {}
	ItemIndexStore(sserialize::UByteArrayAdapter data) : m_priv(new detail::ItemIndexStore(data)) {}
	~ItemIndexStore() {}
	inline OffsetType getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline uint32_t size() const { return priv()->size(); }
	inline ItemIndex::Types indexType() const { return priv()->indexType(); }
	inline IndexCompressionType compressionType() const { return priv()->compressionType(); }
	inline UByteArrayAdapter rawDataAt(uint32_t pos) const { return priv()->rawDataAt(pos); }
	inline ItemIndex at(uint32_t pos) const { return priv()->at(pos);}
	inline ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const { return priv()->at(pos, realIdIndex);}
	inline ItemIndex hierachy(const std::deque< uint32_t >& offsets) const { return priv()->hierachy(offsets); }
	inline std::ostream& printStats(std::ostream& out) const { return priv()->printStats(out); }
	inline std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const { return priv()->printStats(out, indexIds);}
	inline SortedOffsetIndex & getIndex() { return priv()->getIndex();}
	inline const UByteArrayAdapter & getData() const { return priv()->getData(); }
	inline std::shared_ptr<HuffmanDecoder> getHuffmanTree() const { return priv()->getHuffmanTree(); }
	inline UByteArrayAdapter getHuffmanTreeData() const { return priv()->getHuffmanTreeData();}
};

}}//end namespace

#endif