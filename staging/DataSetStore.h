#ifndef SSERIALIZE_STATIC_DATA_SET_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include "HuffmanDecoder.h"
#include <unordered_set>
#define SSERIALIZE_STATIC_DATA_SET_STORE_VERSION 1

/*Version 1
 *
 *
 *-----------------------------------------------------------------------------------------------------------------------------------------
 *VERSION|DATA              |DecompressionSizeTable
 *-----------------------------------------------------------------------------------------------------------------------------------------
 *   1   |Static::Array<UBA>|BoundedCompactUintArray
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
class CompressedArray: public RefCountObject {
private:
	class LZODecompressor: public RefCountObject {
		BoundedCompactUintArray m_data;
	public:
		LZODecompressor();
		LZODecompressor(const sserialize::UByteArrayAdapter & data);
		virtual ~LZODecompressor();
		OffsetType getSizeInBytes() const;
		UByteArrayAdapter decompress(uint32_t id, const sserialize::UByteArrayAdapter & src) const;
	};
private:
	uint8_t m_version;
	UByteArrayAdapter m_data;
	SortedOffsetIndex m_index;
	RCPtrWrapper<LZODecompressor> m_lzod;
public:
	DataSetStore();
	DataSetStore(sserialize::UByteArrayAdapter data);
	~DataSetStore();
	OffsetType getSizeInBytes() const;
	uint32_t size() const;
	UByteArrayAdapter at(uint32_t pos) const;
	std::ostream& printStats(std::ostream& out) const;
	SortedOffsetIndex & getIndex() { return m_index;}
	const UByteArrayAdapter & getData() const { return m_data; }
};

}

class DataSetStore {
private:
	RCPtrWrapper<detail::DataSetStore> m_priv;
protected:
	const RCPtrWrapper<detail::DataSetStore> & priv() const { return m_priv; }
	RCPtrWrapper<detail::DataSetStore> & priv() { return m_priv; }
public:
	DataSetStore() : m_priv(new detail::DataSetStore()) {}
	DataSetStore(sserialize::UByteArrayAdapter data) : m_priv(new detail::DataSetStore(data)) {}
	~DataSetStore() {}
	inline OffsetType getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline uint32_t size() const { return priv()->size(); }
	inline UByteArrayAdapter at(uint32_t pos) const { return priv()->at(pos); }
	inline std::ostream& printStats(std::ostream& out) const { return priv()->printStats(out); }
	inline std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const { return priv()->printStats(out, indexIds);}
	inline SortedOffsetIndex & getIndex() { return priv()->getIndex();}
	inline const UByteArrayAdapter & getData() const { return priv()->getData(); }
};

}}//end namespace

#endif