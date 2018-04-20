#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_STL_VECTOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_STL_VECTOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <vector>

namespace sserialize {

class ItemIndexPrivateStlVector: public ItemIndexPrivate {
public:
	ItemIndexPrivateStlVector();
	ItemIndexPrivateStlVector(std::vector<uint32_t> && data);
	ItemIndexPrivateStlVector(const std::vector<uint32_t> & data);
	virtual ~ItemIndexPrivateStlVector();
	virtual ItemIndex::Types type() const override;
	void absorb(std::vector<uint32_t> & data);
public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual uint32_t size() const override;

	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const  override;
	virtual uint8_t bpn() const override;

	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
private:
	std::vector<uint32_t> m_data;
};

}//end namespace

#endif
