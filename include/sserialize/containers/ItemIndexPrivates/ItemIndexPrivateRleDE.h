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
  
class ItemIndexPrivateRleDECreator final {
public:
	UByteArrayAdapter & m_dest;
	UByteArrayAdapter m_data;
	sserialize::UByteArrayAdapter::OffsetType m_beginning;
	uint32_t m_rle;
	uint32_t m_lastDiff;
	uint32_t m_count;
	uint32_t m_prev;
private:
	void flushRle();
	//flushes the data and header to m_dest
	void flushData();
public:
	ItemIndexPrivateRleDECreator(UByteArrayAdapter & data);
	~ItemIndexPrivateRleDECreator();
	uint32_t size() const;
	///current id
	uint32_t cId() const;
	///current delta
	uint32_t cDelta() const;
	
	///push only in ascending order (id need to be unique and larger than the one before! otherwise this will eat your kitten!
	void push_back(uint32_t id);
	
	///push a run-length starting from nextId with the given delta and length
	void push_rle(uint32_t nextId, uint32_t delta, uint32_t length);
	
	///Does a flush and appends the data which has countInData elements (mainly used by set functions)
	void flushWithData(const UByteArrayAdapter & appendData, uint32_t countInData);
	///you need to call this prior to using toIndex() or using the data
	///you should not push after calling this function
	void flush();

	ItemIndex getIndex();
	
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
		virtual bool eq(const ItemIndexPrivate::const_iterator_base_type * other) const override;
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
	
	virtual uint32_t find(uint32_t id) const override;

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


	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;
	
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

	auto getNext = [](UByteArrayAdapter & data, uint32_t & val, uint32_t & rle) {
		val = data.getVlPackedUint32();
		if (val & 0x1) {
			rle = val >> 1;
			val = data.getVlPackedUint32();
		}
		val >>= 1;
	};
	
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
			if (aRle > 4 && aId + 3*aVal < bId) {
				//the + 1 is needed since aId consumed one rle, which was not subtracted yet
				uint32_t myRle = (bId - aId)/aVal - ((bId - aId)%aVal > 0 ? 0 : 1) + 1;
				
				if (TFunc::pushFirstSmaller) {
					creator.push_rle(aId, aVal, myRle-1);
				}
				
				aRle -= myRle;
				aIndexIt += myRle;
				aId += aVal*(myRle-1);
				
				if (!aRle) {
					getNext(aData, aVal, aRle);
					aId += aVal;
				}
				else {
					aId += aVal;
				}
			}
			else {
				if (TFunc::pushFirstSmaller) {
					creator.push_back(aId);
				}
				
				if (aRle) {
					--aRle;
				}
				
				if (!aRle) {
					getNext(aData, aVal, aRle);
				}
				aId += aVal;
				++aIndexIt;
			}
		}
		else if (bId  < aId) {
			if (bRle > 4 && bId + 3*bVal < aId) {
				//the + 1 is needed since aId consumed one rle, which was not subtracted yet
				uint32_t myRle = (aId-bId)/bVal - ((aId-bId)%bVal > 0 ? 0 : 1) + 1;
				
				if (TFunc::pushFirstSmaller) {
					creator.push_rle(bId, bVal, myRle-1);
				}
				
				bRle -= myRle;
				bIndexIt += myRle;
				bId += bVal*(myRle-1);
				
				if (!bRle) {
					getNext(bData, bVal, bRle);
					bId += bVal;
				}
				else {
					bId += bVal;
				}
			}
			else {
				if (TFunc::pushSecondSmaller) {
					creator.push_back(bId);
				}
			
				if (bRle) {
					--bRle;
				}
				
				if (!bRle) {
					getNext(bData, bVal, bRle);
				}
				bId += bVal;
				++bIndexIt;
			}
		}
		else {
			if (aRle > 4 && bRle > 4 && aVal == bVal) { // && aId == bId
				uint32_t myRle = std::min(aRle, bRle);
				
				if (TFunc::pushEqual) {
					creator.push_rle(aId, aVal, myRle-1);
				}
				
				//adjust variables accordingly
				aRle -= myRle;
				aIndexIt += myRle;
				aId += aVal*(myRle-1);
				
				bRle -= myRle;
				bIndexIt += myRle;
				bId += bVal*(myRle-1);
				
				if (!aRle) {
					getNext(aData, aVal, aRle);
					aId += aVal;
				}
				else {
					aId += aVal;
				}

				if (!bRle) {
					getNext(bData, bVal, bRle);
					bId += bVal;
				}
				else {
					bId += bVal;
				}
			}
			else {
				if (TFunc::pushEqual) {
					creator.push_back(aId);
				}
				
				if (aRle) {
					--aRle;
				}
				
				if (!aRle) {
					getNext(aData, aVal, aRle);
				}
				aId += aVal;
				++aIndexIt;

				if (bRle) {
					--bRle;
				}
					
				if (!bRle) {
					getNext(bData, bVal, bRle);
				}
				bId += bVal;
				++bIndexIt;
			}
		}
	}

	if (TFunc::pushFirstRemainder && aIndexIt < aSize) {
		if (aRle) {
			creator.push_rle(aId, aVal, aRle-1);
			SSERIALIZE_CHEAP_ASSERT_EQUAL(creator.cId(), aId+aVal*(aRle-1));
			aIndexIt += aRle;
			aId += aVal*aRle;
			aRle = 0;
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
			creator.push_rle(bId, bVal, bRle-1);
			SSERIALIZE_CHEAP_ASSERT_EQUAL(creator.cId(), bId+bVal*(bRle-1));
			bIndexIt += bRle;
			bId += bVal*bRle;
			bRle = 0;
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

}//end namespace
#endif