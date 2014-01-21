#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_REGLINE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_REGLINE_H
#include "ItemIndexPrivate.h"
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/utility/LinearRegressionFunctions.h>
#include <sserialize/utility/exceptions.h>
#include <cstdlib>

//TODO: Möglichkeit, delta-enkodierung zu nutzen
/* Data Layout
 * 
 * v4:
 * -------------------------------------------------------
 * CONTIDBITS|Y-INTCEPT|SLOPENOM|IDOFFSET|IDS
 * -------------------------------------------------------
 *  1-5 byte |1-5 bytes|1-5 byte|1-5 byte|COMPACTARRAY
 * -------------------------------------------------------
 * 
 * IDBITS: 5 bits, define the bits per number for IDS: bpn = IDBITS+1;
 * COUNT is encoded with the var_uint32_t function
 * Y-INTERCEPT: int32_t  encoded as uint32_t
 * SLOPENOM: uint32_t, the last y value of the linear regression minus Y-INTERCEPT (comes from f(count-1)-f(0)/(count-1-0) * (count-1) = SLOPENOM
 * Calulating ids:
 * if (COUNT == 1) => no Y-INTERCEPT/SLOPE present. Only one ID
 * else
 *    ID = IDS(POS) + (SLOPENOM)/(COUNT-1)*POS + ((SLOPENOM % (COUNT-1))*POS)/(COUNT-1) + Y-INTERCEPT - IDOFFSET
 */

namespace sserialize {

class ItemIndexPrivateRegLine: public ItemIndexPrivate {
private:
	CompactUintArray m_idStore;
	uint32_t m_size;
	uint8_t m_bpn;
	uint32_t m_slopenom;
	int32_t m_yintercept; 
	uint32_t m_idOffset;//negative id-offset

private:
	bool init(UByteArrayAdapter index);
	
	static inline uint32_t getRegLineSlopeCorrectionValue(const uint32_t slopenom, const uint32_t size, const uint32_t pos) {
		return (slopenom/(size-1))*pos + ((slopenom % (size-1))*pos)/(size-1);
	}

private: //creation functions
	
	template<class TSortedContainer>
	static uint8_t getNeededBitsForLinearRegression(const TSortedContainer & ids, uint32_t slopenom, int32_t yintercept, uint32_t & idOffset) {
		int64_t curOffSetCorrection;
		int64_t maxPositive = 0;
		int64_t minNegative = 0;
		int64_t curDiff;
		uint64_t count = 0;
		typename TSortedContainer::const_iterator end( ids.end() );
		for(typename TSortedContainer::const_iterator it = ids.begin(); it != end; ++it) {
			curOffSetCorrection = static_cast<int64_t>(getRegLineSlopeCorrectionValue(slopenom, ids.size(), count)) + yintercept; 
			curDiff = *it - curOffSetCorrection;
			if (curDiff > maxPositive)
				maxPositive = curDiff;
			if (curDiff < minNegative)
				minNegative = curDiff;
			count++;
		}
		uint64_t range = maxPositive - minNegative + 1;
#ifdef __ANDROID__
		idOffset = std::abs( (long int) minNegative);
#else
		idOffset = std::abs(minNegative);
#endif
		return CompactUintArray::minStorageBits(range);
	}

	template<class TSortedContainer>
	static bool getLinearRegressionParamsInteger(const TSortedContainer & ids, uint32_t & slopenom, int32_t & yintercept, uint8_t & bpn, uint32_t & idOffset) {
		double sloped, yinterceptd;
		getLinearRegressionParams(ids, sloped, yinterceptd);
	#ifndef __ANDROID__
		if (std::isfinite(sloped) && std::isfinite(yinterceptd)) {
			yintercept = floor(yinterceptd);
			slopenom = floor(sloped) * (ids.size()-1);
		}
		else {
			slopenom = 0;
			yintercept = 0;
		}
	#endif
		bpn = getNeededBitsForLinearRegression(ids, slopenom, yintercept, idOffset);
		return true;
	/*
		//now test all 4 cases of floor/ceiling combinations:
		double slopenomd = sloped*(ids.size()-1);
		uint32_t slopenoms[4] = {
			uint32_t(floor(slopenomd)),
			uint32_t(floor(slopenomd)),
			uint32_t(ceil(slopenomd)),
			uint32_t(ceil(slopenomd))
		};
		int32_t yintercepts[4] = {
			int32_t(floor(yinterceptd)),
			int32_t(ceil(yinterceptd)),
			int32_t(floor(yinterceptd)),
			int32_t(ceil(yinterceptd))
		};

		bpn =  33;
		uint8_t bestMatch = 0;
		idOffset = 0;
		for(size_t i = 0; i < 4; i++) {
			uint32_t tmpIdOffset;
			uint8_t tmp = getNeededBitsForLinearRegression(ids, slopenoms[i], yintercepts[i], tmpIdOffset);
			if (bpn > tmp) {
				bpn = tmp;
				idOffset = tmpIdOffset;
				bestMatch = i;
			}
		}
		slopenom = slopenoms[bestMatch];
		yintercept = yintercepts[bestMatch];
		return true;
	*/
	}

public:
	ItemIndexPrivateRegLine();
	ItemIndexPrivateRegLine(const ItemIndexPrivateRegLine& idx);
	ItemIndexPrivateRegLine(const UByteArrayAdapter & index);
	ItemIndexPrivateRegLine(uint32_t size, const CompactUintArray& idStore, uint8_t bpn, uint32_t slopenom, int32_t yintercept, uint32_t idOffSet);
	virtual ~ItemIndexPrivateRegLine() {}
	virtual ItemIndex::Types type() const;

	virtual int binarySearchIdInIndex(uint32_t id) const;

public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;
	
	virtual uint32_t size() const;

	virtual uint8_t bpn() const { return m_bpn;}
	virtual uint32_t slopenom() const { return m_slopenom; }
	virtual int32_t yintercept() const { return m_yintercept; }
	virtual uint32_t idOffSet() const { return m_idOffset;}

	virtual uint32_t rawIdAt(const uint32_t pos) const;
	virtual uint32_t getSizeInBytes() const;
	virtual uint32_t getHeaderbytes() const;
	virtual uint32_t getRegressionLineBytes() const;
	virtual uint32_t getIdBytes() const;
public:

	template<class TSortedContainer>
	static bool create(const TSortedContainer & ids, std::deque<uint8_t> & indexList, int8_t fixedBitWith = -1, bool regLine = false) {
		UByteArrayAdapter tmp(&indexList);
		tmp.setPutPtr(indexList.size());
		return create(ids, tmp, fixedBitWith, regLine);
	}

	template<class TSortedContainer>
	static bool create(const TSortedContainer & ids, std::vector<uint8_t> & indexList, int8_t fixedBitWith = -1, bool regLine = false) {
		UByteArrayAdapter tmp(&indexList);
		tmp.setPutPtr(indexList.size());
		return create(ids, tmp, fixedBitWith, regLine);
	}


	/** @param ids: ids to add
	  * @param destination: destination to put into starting at putPtr
	  * @param fixedBitWith: Sets a fixed bit with, -1 => auto
	  * @param regLine: Sets if a regression Line shall be used for interpolation
	  */
	template<class TSortedContainer>
	static bool create(const TSortedContainer & ids, UByteArrayAdapter & destination, int8_t fixedBitWith = -1, bool regLine = false) {
		if (ids.size() > 0x7FFFFFF) {
			throw sserialize::OutOfBoundsException("sserialize::ItemIndexPrivateRegLine");
		}
		if (ids.size() > 1) {
			uint32_t slopenom = 0;
			int32_t yintercept = 0;
			uint8_t bitsForIds = 32;
			uint32_t idOffset = 0;
			if (regLine) {
				getLinearRegressionParamsInteger(ids, slopenom, yintercept, bitsForIds, idOffset);
			}
			else {
				slopenom = 0;
				yintercept = *(ids.begin());
				bitsForIds = CompactUintArray::minStorageBits(*(ids.rbegin()) - *(ids.begin()));
				idOffset = 0;
			}

			if (fixedBitWith > 0) {
				if (fixedBitWith > bitsForIds)
					bitsForIds = fixedBitWith;
				else
					return false;
			}

			uint32_t countTypeHeader = ids.size() << 5;
			countTypeHeader |= (bitsForIds-1);

			if (destination.putVlPackedUint32(countTypeHeader) < 0)
				return false;
			
			if (destination.putVlPackedInt32(yintercept) < 0)
				return false;
			
			if (destination.putVlPackedUint32(slopenom) < 0)
				return false;

			if (destination.putVlPackedUint32(idOffset) < 0)
				return false;

			//push the remaining elements
			int64_t offSetCorrectedId;
			int64_t curOffSetCorrection = yintercept;
			uint32_t count = 0;
			uint32_t idStorageNeed = CompactUintArray::minStorageBytes(bitsForIds, ids.size());
			destination.growStorage(idStorageNeed);
			CompactUintArray carr(destination+destination.tellPutPtr(), bitsForIds);
			destination.incPutPtr(idStorageNeed);
			typename TSortedContainer::const_iterator end (ids.end() );
			for(typename TSortedContainer::const_iterator i = ids.begin(); i != end; ++i) {
				curOffSetCorrection = yintercept + static_cast<int64_t>(getRegLineSlopeCorrectionValue(slopenom, ids.size(), count));
				offSetCorrectedId = static_cast<int64_t>(*i)  - curOffSetCorrection + idOffset;
				carr.set(count, offSetCorrectedId);
				count++;
			}
		}
		else if (ids.size() == 1) { //a single element
			uint32_t itemId = *(ids.begin());
			uint8_t bits = CompactUintArray::minStorageBits(itemId);
			if (fixedBitWith > 0) {
				if (fixedBitWith > bits) {
					bits = fixedBitWith;
				}
				else {
					return false;
				}
			}
			uint8_t countType = 0x20 | (bits-1);
			destination.putUint8(countType);
			std::deque<uint32_t> d;
			d.push_back(itemId);
			std::deque<uint8_t> tmp;
			CompactUintArray::createFromSet(d, tmp, bits);
			destination.put(tmp);
		}
		else {
			destination.putUint8(0);
		}
		return true;
	}

	static void addFixedSizeHeaderRegLine(uint32_t idsInSet, uint8_t storageBits, uint32_t lowestId, const CompactUintArray & carray, UByteArrayAdapter & adap);
};

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateRegLine> ItemIndexPrivateRegLineIndirect;

}//end namespace

#endif