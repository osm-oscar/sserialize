#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_BOUNDED_COMPACT_UINT_ARRAY_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_BOUNDED_COMPACT_UINT_ARRAY_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/CompactUintArray.h>

namespace sserialize {

class ItemIndexPrivateBoundedCompactUintArray: public ItemIndexPrivate {
public:
	ItemIndexPrivateBoundedCompactUintArray();
	ItemIndexPrivateBoundedCompactUintArray(const sserialize::BoundedCompactUintArray & index);
	virtual ~ItemIndexPrivateBoundedCompactUintArray();
	virtual ItemIndex::Types type() const override;
public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;

	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;
private:
	sserialize::BoundedCompactUintArray m_data;
};

}//end namespace

#endif
