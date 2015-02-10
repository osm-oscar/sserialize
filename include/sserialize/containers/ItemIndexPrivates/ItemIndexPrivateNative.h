#ifndef ITEM_INDEX_PRIVATE_NATIVE_H
#define ITEM_INDEX_PRIVATE_NATIVE_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/utility/SerializationInfo.h>
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
	
	template<typename TFunc>
	sserialize::ItemIndexPrivate * genericSetOp(const sserialize::ItemIndexPrivate * other) const;
	
public:
	ItemIndexPrivateNative();
	ItemIndexPrivateNative(const sserialize::UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateNative();
	virtual sserialize::ItemIndex::Types type() const override;
	
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
	virtual sserialize::ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual sserialize::ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual sserialize::ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
	
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



template<typename TFunc>
sserialize::ItemIndexPrivate * ItemIndexPrivateNative::genericSetOp(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	uint32_t maxResultSize = TFunc::maxSize(this, other);
	void * tmpResultRaw = malloc(maxResultSize*sizeof(uint32_t));
	uint32_t * tmpResult = (uint32_t*) tmpResultRaw;
	uint32_t tmpResultSize = 0;
	
	const uint8_t * myD = m_dataMem.get();
	const uint8_t * oD = cother->m_dataMem.get();
	uint32_t myI(0), oI(0);
	for( ;myI < m_size && oI < cother->m_size; ) {
		uint32_t myId = this->ItemIndexPrivateNative::uncheckedAt(myI, myD);
		uint32_t oId = cother->ItemIndexPrivateNative::uncheckedAt(oI, oD);
		if (myId < oId) {
			if (TFunc::pushFirstSmaller) {
				tmpResult[tmpResultSize] = myId;
				++tmpResultSize;
			}
			++myI;
		}
		else if (oId < myId) {
			if (TFunc::pushSecondSmaller) {
				tmpResult[tmpResultSize] = oId;
				++tmpResultSize;
			}
			++oI;
		}
		else {
			if (TFunc::pushEqual) {
				tmpResult[tmpResultSize] = myId;
				++tmpResultSize;
			}
			++oI;
			++myI;
		}
	}
	if (TFunc::pushFirstRemainder) {
		uint32_t remainderSize = m_size-myI;
		::memmove(tmpResult+tmpResultSize, myD+sizeof(uint32_t)*myI, sizeof(uint32_t)*remainderSize);
		tmpResultSize += remainderSize;
	}
	if (TFunc::pushSecondRemainder) {
		uint32_t remainderSize = cother->m_size-oI;
		::memmove(tmpResult+tmpResultSize, oD+sizeof(uint32_t)*oI, sizeof(uint32_t)*remainderSize);
		tmpResultSize += remainderSize;
	}
	
	UByteArrayAdapter tmpD(UByteArrayAdapter::createCache(sserialize::SerializationInfo<uint32_t>::length+sizeof(uint32_t)*tmpResultSize, sserialize::MM_PROGRAM_MEMORY));
	tmpD.putUint32(tmpResultSize);
	tmpD.put(static_cast<uint8_t*>(tmpResultRaw), sizeof(uint32_t)*tmpResultSize);//can we do this? aliasing, cahe updates?
	free(tmpResultRaw);
	return new ItemIndexPrivateNative(tmpD);
}

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateNative> ItemIndexPrivateNativeIndirect;

}}}//end namespace

#endif