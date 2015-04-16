#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_WAH_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_WAH_H
#include "ItemIndexPrivate.h"
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/UDWConstrainedIterator.h>

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
	///@return returns the dat iterator DO NOT call reset() on them!
	const UDWConstrainedIterator & dataIterator() const;
	
public:
	ItemIndexPrivateWAH();
	ItemIndexPrivateWAH(sserialize::UDWIterator data);
	ItemIndexPrivateWAH(const UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateWAH();
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
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	
	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex fusedIntersectDifference(const std::vector< ItemIndexPrivateWAH* > & intersect, const std::vector< ItemIndexPrivateWAH* >& substract, uint32_t count, ItemIndex::ItemFilter * filter = 0);

	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex constrainedIntersect(const std::vector< ItemIndexPrivateWAH* > & intersect, uint32_t count, ItemIndex::ItemFilter * filter = 0);

	
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const;
public:

	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		uint32_t beginning = dest.tellPutPtr();
		dest.putUint32(0);
		dest.putUint32(src.size());
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
		
		uint32_t dataSize =  dest.tellPutPtr() - beginning - 8;
		dest.putUint32(beginning, dataSize);
		return true;
	}
};

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateWAH> ItemIndexPrivateWAHIndirect;

}//end namespace
#endif