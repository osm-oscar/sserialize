#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_WAH_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_WAH_H
#include "ItemIndexPrivate.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/iterator/UDWConstrainedIterator.h>
#include <sserialize/utility/checks.h>

namespace sserialize {

/** This implements a BitVector-Based index
  *
  *--------------------------------------------------------------
  * SIZE |COUNT |DATA
  *--------------------------------------------------------------
  *uint32|uint32|*
  *--------------------------------------------------------------
  *
  *SIZE=number of DWORDS*4 (equals number of bytes in uncompressed format)
  *COUNT=number of elements
  *RLE: uint32_t, least significant bit tells if this dword is rle, if yes, then the second least signifant bit tells if zeros or ones
  *
  */

class ItemIndexPrivateWAH: public ItemIndexPrivate {
private:
	mutable UDWConstrainedIterator m_curData;
	UDWConstrainedIterator m_fullData;
	uint32_t m_size;
	mutable uint32_t m_curId;
	mutable UByteArrayAdapter m_cache;
private:
	///@return returns the data iterator DO NOT call reset() on them!
	const UDWConstrainedIterator & dataIterator() const;
	
public:
	ItemIndexPrivateWAH();
	ItemIndexPrivateWAH(sserialize::UDWIterator data);
	ItemIndexPrivateWAH(const UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateWAH();
	virtual ItemIndex::Types type() const override;
	virtual uint32_t find(uint32_t id) const override;

public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;


	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;

	virtual void putInto(DynamicBitSet & bitSet) const override;
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	
	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex fusedIntersectDifference(const std::vector< ItemIndexPrivateWAH* > & intersect, const std::vector< ItemIndexPrivateWAH* >& substract, uint32_t count, ItemIndex::ItemFilter * filter = 0);

	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex constrainedIntersect(const std::vector< ItemIndexPrivateWAH* > & intersect, uint32_t count, ItemIndex::ItemFilter * filter = 0);

	
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
public:

	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		UByteArrayAdapter::OffsetType beginning = dest.tellPutPtr();
		dest.putUint32((uint32_t)0);
		dest.putUint32(narrow_check<uint32_t>(src.size()));
		if (!src.size())
			return true;
		if (src.size() == 1) {
			uint32_t val = *src.begin();
			if (val > 30) {
				uint32_t leadingZeros = *src.begin()/31;
				uint32_t valueBit = (static_cast<uint32_t>(0x1) << (*src.begin()%31));
				leadingZeros <<= 2;//indicator bit and rle bit
				leadingZeros |= 0x1;
				dest.putUint32(leadingZeros);
				dest.putUint32(valueBit<<1);
				dest.putUint32(beginning, 8);
			}
			else {
				dest.putUint32(static_cast<uint32_t>(1) << (val+1) );
				dest.putUint32(beginning, 4);
			}
			return true;
		}
		uint32_t prev = *src.begin();
		uint32_t curRleEncWord = 0;
		uint32_t curEncWord = 0;
		uint8_t curEncWordPos = 0;

		if (prev > 30) {
			//put the rle zeros
			uint32_t leadingZeros = *src.begin()/31;
			leadingZeros <<= 2;//indicator bit and rle bit
			leadingZeros |= 0x1;
			dest.putUint32(leadingZeros);
			curEncWordPos = *src.begin()%31;
			curEncWord |= static_cast<uint32_t>(1) << curEncWordPos;
			++curEncWordPos;
		}
		else {//it's in the first encWord
			curEncWordPos += prev;
			curEncWord |= static_cast<uint32_t>(1) << curEncWordPos;
			++curEncWordPos;
		}
		if (curEncWordPos > 30) {
			dest.putUint32(curEncWord << 1);
			curEncWordPos = 0;
			curEncWord = 0;
		}
		typename TCONTAINER::const_iterator it(src.begin());
		typename TCONTAINER::const_iterator srcEnd(src.end()); 
		for(++it; it != srcEnd; ++it) {
			uint32_t bitOffsetToPrev = *it-prev;
			prev = *it;
			if (bitOffsetToPrev >  static_cast<uint32_t>(31-curEncWordPos)) {
				bitOffsetToPrev--;
				if (curRleEncWord) { //we also have a leading ones rle dword
					curRleEncWord <<= 2;
					curRleEncWord |= 0x3;
					dest.putUint32(curRleEncWord);
					curRleEncWord = 0;
				}
				if (curEncWordPos) {
					dest.putUint32(curEncWord << 1);
					curEncWord = 0;
					bitOffsetToPrev -= (31-curEncWordPos);
					curEncWordPos = 0;
				}
				if (bitOffsetToPrev >= 31) {
					//create the rle encword if necessary
					uint32_t rleCount = bitOffsetToPrev / 31;
					bitOffsetToPrev -= rleCount*31;
					rleCount <<= 2;
					rleCount |= 0x1;
					dest.putUint32(rleCount);
				}
				curEncWordPos = bitOffsetToPrev % 31;
				curEncWord |= static_cast<uint32_t>(1) << curEncWordPos;
				++curEncWordPos;
			}
			else {// fits into current one
				curEncWordPos += bitOffsetToPrev-1;
				curEncWord |= static_cast<uint32_t>(1) << curEncWordPos;
				++curEncWordPos;
			}
			
			if (curEncWordPos > 30) {
				if (curEncWord == 0x7FFFFFFF) { //all ones, then do rle
					curRleEncWord += 1;
				}
				else {//zeros within, don't do rle, but check if rle is before
					if (curRleEncWord) {
						curRleEncWord <<= 2;
						curRleEncWord |= 0x3;
						dest.putUint32(curRleEncWord);
						curRleEncWord = 0;
					}
					dest.putUint32(curEncWord<<1);
				}
				curEncWord = 0;
				curEncWordPos = 0;
			}
		}
		if (curEncWord == 0x7FFFFFFF) { //all ones, then do rle
			curRleEncWord += 1;
			curEncWord = 0;
		}
		if (curRleEncWord) {
			curRleEncWord <<= 2;
			curRleEncWord |= 0x3;
			dest.putUint32(curRleEncWord);
			curRleEncWord = 0;
		}
		if (curEncWord)
			dest.putUint32(curEncWord << 1);
		
		UByteArrayAdapter::OffsetType dataSize =  dest.tellPutPtr() - beginning - 8;
		dest.putUint32(beginning, narrow_check<uint32_t>(dataSize));
		return true;
	}
};


}//end namespace
#endif
