#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_RLE_DE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_RLE_DE_H
#include "ItemIndexPrivate.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/iterator/UDWConstrainedIterator.h>

namespace sserialize {

/** This implements a difference run-length encoded index
  *
  *--------------------------------------------------------------------------------------
  * COUNT|SIZE|DATA
  *--------------------------------------------------------------------------------------
  * vu32|vu32 |*
  *--------------------------------------------------------------------------------------
  * SIZE is the size of the DATA section
  * COUNT is the number of elements
  * Data is delta encoded with rle. Type of word is selected by the least significant bit.
  * If data[i] & 0x1 then data is a rle and the next varuint32 tells the difference between the (data[i] >> 1) elements
  *
  *
  * TODO:  make sure that for COUNT==1 the size can be dropped
  *
  */
  
class ItemIndexPrivateRleDE;
  
class ItemIndexPrivateRleDECreator {
public:
	UByteArrayAdapter & m_dest;
	UByteArrayAdapter m_data;
	sserialize::UByteArrayAdapter::OffsetType m_beginning;
	uint32_t m_rle;
	uint32_t m_lastDiff;
	uint32_t m_count;
	uint32_t m_prev;
private:
	void flushRle() {
		if (m_rle) {
			if (m_rle == 1) { //no rle
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
			else {
				m_data.putVlPackedUint32((m_rle << 1) | 0x1);
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
		}
		m_rle = 0;
	}
	//flushes the data and header to m_dest
	void flushData() {
		m_dest.setPutPtr(m_beginning);
		m_dest.putVlPackedUint32(m_count);
		m_dest.putVlPackedUint32(m_data.size());
		m_dest.putData(m_data);
	}
public:
	ItemIndexPrivateRleDECreator(UByteArrayAdapter & data) :
	m_dest(data),
	m_data(new std::vector<uint8_t>(), true),
	m_beginning(data.tellPutPtr()),
	m_rle(0),
	m_lastDiff(0),
	m_count(0),
	m_prev(0)
	{}
	virtual ~ItemIndexPrivateRleDECreator() {}
	inline uint32_t size() const { return m_count; }
	///push only in ascending order (id need to be unique and larger than the one before! otherwise this will eat your kitten!
	void push_back(uint32_t id) {
		uint32_t diff = id - m_prev;
		if (diff == m_lastDiff) {
			++m_rle;
		}
		else {
			if (m_rle) {
				if (m_rle == 1) { //no rle
					m_data.putVlPackedUint32(m_lastDiff << 1);
				}
				else {
					m_data.putVlPackedUint32((m_rle << 1) | 0x1);
					m_data.putVlPackedUint32(m_lastDiff << 1);
				}
			}
			m_rle = 1;
			m_lastDiff = diff;
		}
		m_prev = id;
		++m_count;
	}
	
	///Does a flush and appends the data which has countInData elements (mainly used by set functions)
	void flushWithData(const UByteArrayAdapter & appendData, uint32_t countInData) {
		flushRle();
		m_data.putData(appendData);
		m_count += countInData;
		flushData();
	}
	///you need to call this prior to using toIndex() or using the data
	///you should not push after calling this function
	void flush() {
		flushRle();
		flushData();
	}

	ItemIndex getIndex() {
		return ItemIndex(UByteArrayAdapter(m_dest, m_beginning), ItemIndex::T_RLE_DE);
	}
	
	ItemIndexPrivate * getPrivateIndex();
};

class ItemIndexPrivateRleDE: public ItemIndexPrivate {
public:
	typedef ItemIndexPrivate MyBaseClass;
private:
	class MyIterator: public ItemIndexPrivate::const_iterator_base_type {
	private:
		const ItemIndexPrivateRleDE * m_parent;
		UByteArrayAdapter::OffsetType m_dataOffset;
		uint32_t m_curRleCount;
		uint32_t m_curRleDiff;
		uint32_t m_curId;
	private:
		MyIterator(const MyIterator & other);
		void fetchNext();
	public:
		MyIterator(const ItemIndexPrivateRleDE * parent, UByteArrayAdapter::OffsetType dataOffset);
		virtual ~MyIterator();
		virtual uint32_t get() const override;
		virtual void next() override;
		virtual bool notEq(const ItemIndexPrivate::const_iterator_base_type * other) const override;
		virtual ItemIndexPrivate::const_iterator_base_type * copy() const override;
	};
private:
	UByteArrayAdapter m_data;
	uint32_t m_size;
	mutable uint32_t m_dataOffset;
	mutable uint32_t m_curId;
	mutable std::vector<uint32_t> m_cache;
private:
	template<typename TFunc>
	sserialize::ItemIndexPrivate* genericOp(const sserialize::ItemIndexPrivateRleDE * cother) const;
public:
	ItemIndexPrivateRleDE();
	ItemIndexPrivateRleDE(const UByteArrayAdapter & data);
	ItemIndexPrivateRleDE(const UDWIterator & data);
	virtual ~ItemIndexPrivateRleDE();
	virtual ItemIndex::Types type() const override;
	
	virtual int find(uint32_t id) const override;

public:
	virtual void loadIntoMemory() override;

	virtual UByteArrayAdapter data() const override;

	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual MyBaseClass::const_iterator cbegin() const override;
	virtual MyBaseClass::const_iterator cend() const override;

	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;


	virtual uint32_t getSizeInBytes() const override;
	
	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t * dest) const override;
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);


	virtual ItemIndexPrivate * uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const override;
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;

	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex fusedIntersectDifference(const std::vector< ItemIndexPrivateRleDE* > & intersect, const std::vector< ItemIndexPrivateRleDE* >& subtract, uint32_t count);


	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex constrainedIntersect(const std::vector< ItemIndexPrivateRleDE* > & intersect, uint32_t count);


public:
	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		ItemIndexPrivateRleDECreator creator(dest);
		typename TCONTAINER::const_iterator srcIt = src.begin();
		typename TCONTAINER::const_iterator srcEnd = src.end();
		for(; srcIt != srcEnd; ++srcIt) {
			creator.push_back(*srcIt);
		}
		creator.flush();
		return true;
	}
};

template<typename TFunc>
sserialize::ItemIndexPrivate* ItemIndexPrivateRleDE::genericOp(const sserialize::ItemIndexPrivateRleDE * cother) const {
	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	aData.resetGetPtr();
	bData.resetGetPtr();
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
	ItemIndexPrivateRleDECreator creator(dest);

	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aRle = 0;
	uint32_t bRle = 0;
	uint32_t aId = 0;
	uint32_t bId = 0;
	uint32_t aVal = aData.getVlPackedUint32();
	uint32_t bVal = bData.getVlPackedUint32();
	if (aVal & 0x1) {
		aRle = aVal >> 1;
		aVal = aData.getVlPackedUint32();
	}
	if (bVal & 0x1) {
		bRle = bVal >> 1;
		bVal = bData.getVlPackedUint32();
	}
	aVal >>= 1;
	bVal >>= 1;

	aId = aVal;
	bId = bVal;
	
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aId < bId) {
			if (TFunc::pushFirstSmaller) {
				creator.push_back(aId);
			}
		
			if (aRle) {
				--aRle;
			}
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;
		}
		else if (bId  < aId) {
			if (TFunc::pushSecondSmaller) {
				creator.push_back(bId);
			}
		
			if (bRle) {
				--bRle;
			}
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
		else {
			if (TFunc::pushEqual) {
				creator.push_back(aId);
			}
			
			if (aRle) {
				--aRle;
			}
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;

			if (bRle) {
				--bRle;
			}
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
	}

	if (TFunc::pushFirstRemainder && aIndexIt < aSize) {
		if (aRle) {
			while (aRle) {
				creator.push_back(aId);
				aId += aVal;
				++aIndexIt;
				--aRle;
			}
		}
		else {
			creator.push_back(aId);
			++aIndexIt;
		}
		
		//from here on,  the differences are equal to the ones in aData
		aData.shrinkToGetPtr();
		creator.flushWithData(aData, aSize - aIndexIt);
	}
	else if (TFunc::pushSecondRemainder && bIndexIt < bSize) {
		if (bRle) {
			while (bRle) {
				creator.push_back(bId);
				bId += bVal;
				++bIndexIt;
				--bRle;
			}
		}
		else {
			creator.push_back(bId);
			++bIndexIt;
		}
		//from here on,  the differences are equal to the ones in aData
		bData.shrinkToGetPtr();
		creator.flushWithData(bData, bSize - bIndexIt);
	}
	else {
		creator.flush();
	}
	
	dest.resetPtrs();
	
	return new ItemIndexPrivateRleDE(dest);
}

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateRleDE> ItemIndexPrivateRleDEIndirect;

}//end namespace
#endif