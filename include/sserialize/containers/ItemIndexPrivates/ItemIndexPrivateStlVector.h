#ifndef ITEM_INDEX_PRIVATE_STL_VECTOR_H
#define ITEM_INDEX_PRIVATE_STL_VECTOR_H
#include "ItemIndexPrivate.h"
#include <vector>

namespace sserialize {

class ItemIndexPrivateStlVector: public ItemIndexPrivate {
private:
	std::vector<uint32_t> m_data;
public:
	ItemIndexPrivateStlVector();
	ItemIndexPrivateStlVector(std::vector<uint32_t> && data);
	ItemIndexPrivateStlVector(const std::vector<uint32_t> & data);
	virtual ~ItemIndexPrivateStlVector();
	virtual ItemIndex::Types type() const;
	void absorb(std::vector<uint32_t> & data);
public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;

	virtual uint32_t size() const;

	virtual uint8_t bpn() const { return 32; }
	virtual uint32_t slopenom() const { return 0; }
	virtual int32_t yintercept() const { return 0; }
	virtual uint32_t idOffSet() const { return 0; }


	virtual uint32_t rawIdAt(const uint32_t pos) const { return at(pos); }
	virtual uint32_t getSizeInBytes() const { return size()*4;}
	virtual uint32_t getHeaderbytes() const { return 0; }
	virtual uint32_t getRegressionLineBytes() const { return 0; }
	virtual uint32_t getIdBytes() const { return size()*4;}

	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
};

}//end namespace

#endif