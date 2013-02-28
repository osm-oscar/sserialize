#ifndef SSERIALIZE_STATIC_ITEM_INDEX_STORE
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/containers/ItemIndex.h>
#include <unordered_set>
#define SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION 1

/*Version 1
 *
 *
 *------------------------------------------------------------
 *VERSION|IndexTypes|datalength| Data     |      Offsets    |
 *------------------------------------------------------------
 *   1   |    1     |OffsetType|datalength|SortedOffsetIndex
 *
 * 
 *
 *
 */

namespace sserialize {
namespace Static {

/** The first index id is ALWAY the empty index*/
class ItemIndexStore {
private:
	ItemIndex::Types m_type;
	SortedOffsetIndex m_index;
	UByteArrayAdapter m_data;
public:
	ItemIndexStore();
	ItemIndexStore(sserialize::UByteArrayAdapter data);
	~ItemIndexStore();
	uint32_t size() const;
	ItemIndex::Types indexType() const { return m_type; }
	
	ItemIndex at(uint32_t pos) const;
	ItemIndex at(uint32_t pos, const ItemIndex & realIdIndex) const;
	uint32_t getSizeInBytes() const;
	ItemIndex hierachy(const std::deque< uint32_t >& offsets) const;
	std::ostream& printStats(std::ostream& out) const;
	std::ostream& printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const;
	SortedOffsetIndex & getIndex() { return m_index;}
};

}}//end namespace

#endif