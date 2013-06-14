#ifndef SSERIALIZE_STATIC_ITEM_INDEX_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include "HuffmanDecoder.h"
#include <unordered_set>
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION 2

/*Version 2
 *
 *
 *------------------------------------------------------------
 *VERSION|IndexCompressionType|IndexTypes|datalength| Data     |      Offsets    |
 *------------------------------------------------------------
 *   1   |        1           |    1     |OffsetType|datalength|SortedOffsetIndex
 *
 * 
 *
 *
 */

namespace sserialize {
namespace Static {

/** The first index id is ALWAY the empty index*/
class ItemIndexStore {
public:
	typedef enum {IC_NONE=0, IC_VARUINT32=1, IC_HUFFMAN=2, IC_ILLEGAL=0xFF} IndexCompressionType;
private:
	uint8_t m_version;
	ItemIndex::Types m_type;
	IndexCompressionType m_compression;
	HuffmanDecoder m_hd;
	SortedOffsetIndex m_index;
	UByteArrayAdapter m_data;
public:
	ItemIndexStore();
	ItemIndexStore(sserialize::UByteArrayAdapter data);
	~ItemIndexStore();
	uint32_t size() const;
	ItemIndex::Types indexType() const { return m_type; }
	UByteArrayAdapter dataAt(uint32_t pos) const;
	ItemIndex at(uint32_t pos) const;
	ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const;
	uint32_t getSizeInBytes() const;
	ItemIndex hierachy(const std::deque< uint32_t >& offsets) const;
	std::ostream& printStats(std::ostream& out) const;
	std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const;
	SortedOffsetIndex & getIndex() { return m_index;}
	const UByteArrayAdapter & getData() const { return m_data; }
};

}}//end namespace

#endif