#ifndef ITEM_INDEX_PRIVATE_NATIVE_H
#define ITEM_INDEX_PRIVATE_NATIVE_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <string.h>

namespace sserialize {
namespace detail {
namespace ItemIndexPrivate {

class ItemIndexPrivateNative: public sserialize::ItemIndexPrivate {
private:
	UByteArrayAdapter::MemoryView m_dataMem;
	uint32_t m_size;
private:
	inline uint32_t uncheckedAt(uint32_t pos, const uint8_t * data) const {
		data += sizeof(uint32_t)*pos;
		uint32_t res;
		::memmove(&res, data, sizeof(uint32_t));
		return res;
	}
public:
	ItemIndexPrivateNative();
	ItemIndexPrivateNative(const sserialize::UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateNative();
	virtual ItemIndex::Types type() const override;
	
public:
	///uses at(pos) by default
	virtual uint32_t uncheckedAt(uint32_t pos) const override;
	///checked at
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual uint32_t size() const override;

	///return the mean bit per number including header
	virtual uint8_t bpn() const override;
	
	virtual uint32_t getSizeInBytes() const override;
	
	virtual sserialize::ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	
	template<typename T_UINT32_ITERATOR>
	static bool create(T_UINT32_ITERATOR begin, const T_UINT32_ITERATOR & end, UByteArrayAdapter & dest);
	
	template<typename T_SORTED_CONTAINER>
	static bool create(const T_SORTED_CONTAINER & src, UByteArrayAdapter & dest) {
		return create(src.cbegin(), src.cend(), dest);
	}
};

template<typename T_UINT32_ITERATOR>
bool ItemIndexPrivateNative::create(T_UINT32_ITERATOR begin, const T_UINT32_ITERATOR & end, UByteArrayAdapter & dest) {
	uint32_t size = std::distance(begin, end);
	uint8_t * tmp = new uint8_t[sizeof(uint32_t)*size];
	uint8_t * tmpPtr = tmp;
	for(; begin != end; ++begin, tmpPtr += sizeof(uint32_t)) {
		uint32_t src = *begin;
		memmove(tmpPtr, &src, sizeof(uint32_t));
	}
	dest.putUint32(size);
	dest.put((uint8_t*)tmp, sizeof(uint32_t)*size);
	delete[] tmp;
	return true;
}


typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateNative> ItemIndexPrivateNativeIndirect;

}}}//end namespace

#endif