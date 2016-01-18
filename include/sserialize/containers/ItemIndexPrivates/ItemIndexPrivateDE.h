#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_DE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_DE_H
#include "ItemIndexPrivate.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/checks.h>

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
private:
	template<typename TFunc>
	sserialize::ItemIndexPrivate* genericOp(const sserialize::ItemIndexPrivateDE * cother) const;
public:
	ItemIndexPrivateDE();
	ItemIndexPrivateDE(const UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateDE();
	virtual ItemIndex::Types type() const override;
	
	virtual uint32_t find(uint32_t id) const override;

public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;


	virtual uint32_t getSizeInBytes() const override;
	
	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t * bitSet) const override;
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);


	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;

public:
	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		UByteArrayAdapter::OffsetType beginning = dest.tellPutPtr();
		dest.putUint32(0);
		dest.putUint32(narrow_check<uint32_t>(src.size()));
		if (!src.size())
			return true;
		if (src.size() == 1) {
			dest.putVlPackedUint32(*src.begin());
			dest.putUint32(beginning, narrow_check<uint32_t>(dest.tellPutPtr()-(beginning+8)));
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
		UByteArrayAdapter::OffsetType dataSize =  dest.tellPutPtr() - beginning - 8;
		dest.putUint32(beginning, narrow_check<uint32_t>(dataSize));
		return true;
	}
};

class ItemIndexPrivateDECreator {
public:
	UByteArrayAdapter & m_data;
	UByteArrayAdapter::OffsetType m_beginning;
	uint32_t m_prev;
	uint32_t m_count;
public:
	ItemIndexPrivateDECreator(UByteArrayAdapter & data) : m_data(data), m_beginning(data.tellPutPtr()), m_prev(0), m_count(0) {
		m_data.putUint32(0);
		m_data.putUint32(0);
	}
	virtual ~ItemIndexPrivateDECreator() {}
	inline void push_back(uint32_t id) {
		m_data.putVlPackedUint32(id - m_prev);
		m_prev = id;
		++m_count;
	}
	
	///don't push anything after calling this function
	inline void flushWithData(const UByteArrayAdapter & src, uint32_t itemcount) {
		m_data.putData(src);
		m_count += itemcount;
		flush();
	}
	
	inline void flush() {
		m_data.putUint32(m_beginning, (uint32_t)(m_data.tellPutPtr() - (m_beginning + 8)));
		m_data.putUint32(m_beginning+4, m_count);
	}

	inline ItemIndexPrivate * getPrivateIndex() {
		return new ItemIndexPrivateDE(UByteArrayAdapter(m_data, m_beginning));
	}
	
	inline ItemIndex getIndex() {
		return ItemIndex(UByteArrayAdapter(m_data, m_beginning), ItemIndex::T_DE);
	}
};

template<typename TFunc>
sserialize::ItemIndexPrivate* ItemIndexPrivateDE::genericOp(const sserialize::ItemIndexPrivateDE* cother) const {
	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
	ItemIndexPrivateDECreator creator(dest);
	
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aItemId = aData.getVlPackedUint32();
	uint32_t bItemId = bData.getVlPackedUint32();
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aItemId == bItemId) {
			if (TFunc::pushEqual) {
				creator.push_back(aItemId);
			}
			aIndexIt++;
			bIndexIt++;
			aItemId += aData.getVlPackedUint32();
			bItemId += bData.getVlPackedUint32();
		}
		else if (aItemId < bItemId) {
			if (TFunc::pushFirstSmaller) {
				creator.push_back(aItemId);
			}
			aIndexIt++;
			aItemId += aData.getVlPackedUint32();
		}
		else { //bItemId is smaller
			if (TFunc::pushSecondSmaller) {
				creator.push_back(bItemId);
			}
			bIndexIt++;
			bItemId += bData.getVlPackedUint32();
		}
	}

	if (TFunc::pushFirstRemainder && aIndexIt < aSize) {
		creator.push_back(aItemId);
		++aIndexIt;
		//from here on,  the differences are equal to the ones in aData
		aData.shrinkToGetPtr();
		uint32_t remainderSize = aSize - aIndexIt;
		creator.flushWithData(aData, remainderSize);
	}
	else if(TFunc::pushSecondRemainder && bIndexIt < bSize) {
		creator.push_back(bItemId);
		++bIndexIt;
		//from here on,  the differences are equal to the ones in aData
		bData.shrinkToGetPtr();
		uint32_t remainderSize = bSize - bIndexIt;
		creator.flushWithData(bData, remainderSize);
	}
	else {
		creator.flush();
	}
	return creator.getPrivateIndex();
}


typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateDE> ItemIndexPrivateDEIndirect;

}//end namespace
#endif