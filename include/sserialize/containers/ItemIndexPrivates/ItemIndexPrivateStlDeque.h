#ifndef ITEM_INDEX_PRIVATE_STL_DEQUE_H
#define ITEM_INDEX_PRIVATE_STL_DEQUE_H
#include "ItemIndexPrivate.h"
#include <deque>

namespace sserialize {

class ItemIndexPrivateStlDeque: public ItemIndexPrivate {
private:
	std::deque<uint32_t> m_data;
public:
	ItemIndexPrivateStlDeque();
	ItemIndexPrivateStlDeque(const std::deque<uint32_t> & data);
	virtual ~ItemIndexPrivateStlDeque();
	virtual ItemIndex::Types type() const;
public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;

	virtual uint32_t size() const;

	virtual uint8_t bpn() const { return 32; }
	virtual uint32_t slopenom() const { return 0; }
	virtual int32_t yintercept() const { return 0; }
	virtual uint32_t idOffSet() const { return 0; }


	virtual sserialize::UByteArrayAdapter::SizeType rawIdAt(const uint32_t pos) const { return at(pos); }
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const { return size()*4;}
	virtual sserialize::UByteArrayAdapter::SizeType getHeaderbytes() const { return 0; }
	virtual sserialize::UByteArrayAdapter::SizeType getRegressionLineBytes() const { return 0; }
	virtual sserialize::UByteArrayAdapter::SizeType getIdBytes() const { return size()*4;}

	
};

}//end namespace

#endif