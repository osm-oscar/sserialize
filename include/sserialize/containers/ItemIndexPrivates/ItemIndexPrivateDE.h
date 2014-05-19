#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_DE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_DE_H
#include "ItemIndexPrivate.h"
#include <sserialize/utility/utilfuncs.h>

namespace sserialize {

/** This implements a BitVector-Based index
  *
  *--------------------------------------------------------------
  * SIZE |COUNT |DATA
  *--------------------------------------------------------------
  *uint32|uint32|*
  *--------------------------------------------------------------
  *Data is delta encoded
  *
  *
  */

class ItemIndexPrivateDE: public ItemIndexPrivate {
private:
	UByteArrayAdapter m_data;
	uint32_t m_size;
	mutable uint32_t m_dataOffset;
	mutable uint32_t m_curId;
	mutable UByteArrayAdapter m_cache;
	mutable uint32_t m_cacheOffset;
public:
	ItemIndexPrivateDE();
	ItemIndexPrivateDE(const UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateDE();
	virtual ItemIndex::Types type() const;
	
	virtual int find(uint32_t id) const;

public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;

	virtual uint32_t size() const;

	virtual uint8_t bpn() const;


	virtual uint32_t getSizeInBytes() const;
	
	virtual void putInto(DynamicBitSet & bitSet) const;
	virtual void putInto(uint32_t * bitSet) const override;
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);


	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const;
// 	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const;
// 	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const;

public:
	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		uint32_t beginning = dest.tellPutPtr();
		dest.putUint32(0);
		dest.putUint32(src.size());
		if (!src.size())
			return true;
		if (src.size() == 1) {
			dest.putVlPackedUint32(*src.begin());
			dest.putUint32(beginning, dest.tellPutPtr()-(beginning+8));
			return true;
		}
		uint32_t prevDest = *src.begin();
		dest.putVlPackedUint32(prevDest);
		
		typename TCONTAINER::const_iterator it(src.begin());
		typename TCONTAINER::const_iterator srcEnd(src.end()); 
		for(++it; it != srcEnd; ++it) {
			dest.putVlPackedUint32(*it - prevDest);
			prevDest = *it;
		}
		uint32_t dataSize =  dest.tellPutPtr() - beginning - 8;
		dest.putUint32(beginning, dataSize);
		return true;
	}
};

class ItemIndexPrivateDECreator {
public:
	UByteArrayAdapter & m_data;
	uint32_t m_beginning;
	uint32_t m_prev;
	uint32_t m_count;
public:
	ItemIndexPrivateDECreator(UByteArrayAdapter & data) : m_data(data), m_beginning(data.tellPutPtr()), m_prev(0), m_count(0) {
		m_data.putUint32(0);
		m_data.putUint32(0);
	}
	virtual ~ItemIndexPrivateDECreator() {}
	void push_back(uint32_t id) {
		m_data.putVlPackedUint32(id - m_prev);
		m_prev = id;
		++m_count;
	}
	void flush() {
		m_data.putUint32(m_beginning, m_data.tellPutPtr() - (m_beginning + 8));
		m_data.putUint32(m_beginning+4, m_count);
	}

	ItemIndexPrivate * getPrivateIndex() {
		return new ItemIndexPrivateDE(UByteArrayAdapter(m_data, m_beginning));
	}
	
	ItemIndex getIndex() {
		return ItemIndex(UByteArrayAdapter(m_data, m_beginning), ItemIndex::T_DE);
	}
};

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateDE> ItemIndexPrivateDEIndirect;

}//end namespace
#endif