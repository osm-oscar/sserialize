#ifndef SSERIALIZE_SORTED_OFFSET_INDEX_PRIVATE_H
#define SSERIALIZE_SORTED_OFFSET_INDEX_PRIVATE_H
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/utility/LinearRegressionFunctions.h>
#include <sserialize/utility/utilfuncs.h>
#include <iostream>

namespace sserialize {
namespace Static {

/* Data Layout
 * 
 * v4:
 * -------------------------------------------------------
 * CONTIDBITS|Y-INTCEPT|SLOPENOM|IDOFFSET|IDS
 * -------------------------------------------------------
 *  1-9 byte |1-9 bytes|1-9 byte|1-9 byte|COMPACTARRAY
 * -------------------------------------------------------
 * 
 * IDBITS: 6 bits, define the bits per number for IDS: bpn = IDBITS+1;
 * COUNT is encoded with the var_uint64_t function
 * Y-INTERCEPT: int64_t 
 * SLOPENOM: uint64_t, the last y value of the linear regression minus Y-INTERCEPT (comes from f(count-1)-f(0)/(count-1-0) * (count-1) = SLOPENOM
 * Calulating ids:
 * if (COUNT == 1) => only Y-INTERCEPT present = id, but encoded as uint64
 * else
 *    ID = IDS(POS) + (SLOPENOM)/(COUNT-1)*POS + ((SLOPENOM % (COUNT-1))*POS)/(COUNT-1) + Y-INTERCEPT - IDOFFSET
 */

class SortedOffsetIndexPrivate: public sserialize::RefCountObject {
private:
	CompactUintArray m_idStore;
	uint32_t m_size;
	uint64_t m_slopenom;
	int64_t m_yintercept; 
	uint64_t m_idOffset;//negative id-offset
	
private:
	template<class TSortedContainer>
	static uint8_t getNeededBitsForLinearRegression(const TSortedContainer & ids, uint64_t slopenom, int64_t yintercept, uint64_t & idOffset) {
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
		return CompactUintArray::minStorageBits64(range);
	}

	template<class TSortedContainer>
	static bool getLinearRegressionParamsInteger(const TSortedContainer & ids, uint64_t & slopenom, int64_t & yintercept, uint8_t & bpn, uint64_t & idOffset) {
		double sloped, yinterceptd;
		getLinearRegressionParams(ids, sloped, yinterceptd);
	#ifndef __ANDROID__
		if (std::isfinite(sloped) && std::isfinite(yinterceptd)) {
			yintercept = floor(yinterceptd);
			double slopenomd = floor(sloped) * (ids.size()-1);
			slopenom = slopenomd;
			if (slopenomd >= floor((double)std::numeric_limits<uint64_t>::max()) || yinterceptd >= floor((double)std::numeric_limits<uint64_t>::max())) {
				return false;
			}
		}
		else {
			slopenom = 0;
			yintercept = 0;
		}
	#endif
		bpn = getNeededBitsForLinearRegression(ids, slopenom, yintercept, idOffset);
		return true;
	}
	
private:
	bool init(UByteArrayAdapter data);
	
	static inline uint64_t getRegLineSlopeCorrectionValue(const uint64_t slopenom, const uint64_t size, const uint64_t pos) {
		return (slopenom/(size-1))*pos + ((slopenom % (size-1))*pos)/(size-1);
	}
public:
	SortedOffsetIndexPrivate();
	SortedOffsetIndexPrivate(const UByteArrayAdapter & data);
	virtual ~SortedOffsetIndexPrivate();
	virtual UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t size() const;
	UByteArrayAdapter::OffsetType at(uint32_t pos) const;
	
	///Append a SortedOffsetIndexPrivate at dest
	template<typename TSortedContainer>
	static bool create(const TSortedContainer & src, sserialize::UByteArrayAdapter & destination) {
	
		if (src.size() > 1) {
			uint64_t slopenom = 0;
			int64_t yintercept = 0;
			uint8_t bitsForIds = 32;
			uint64_t idOffset = 0;
			if (!getLinearRegressionParamsInteger(src, slopenom, yintercept, bitsForIds, idOffset)) {
				bitsForIds = 0xFF;
				std::cerr << "Failed to create regressionline params" << std::endl;
			}
			
			if (bitsForIds >= CompactUintArray::minStorageBits64(*src.rbegin())) {
				bitsForIds = CompactUintArray::minStorageBits64(*src.rbegin());
				slopenom = 0;
				yintercept = 0;
				idOffset = 0;
			}

			uint64_t countTypeHeader = static_cast<uint64_t>(src.size()) << 6;
			countTypeHeader |= (bitsForIds-1);

			if (destination.putVlPackedUint64(countTypeHeader) < 0)
				return false;
			
			if (destination.putVlPackedInt64(yintercept) < 0)
				return false;
			
			if (destination.putVlPackedUint64(slopenom) < 0)
				return false;

			if (destination.putVlPackedUint64(idOffset) < 0)
				return false;

			//push the remaining elements
			int64_t offSetCorrectedId;
			int64_t curOffSetCorrection = yintercept;
			uint32_t count = 0;
			uint32_t idStorageNeed = CompactUintArray::minStorageBytes(bitsForIds, src.size());
			destination.growStorage(idStorageNeed);
			CompactUintArray carr(destination+destination.tellPutPtr(), bitsForIds);
			destination.incPutPtr(idStorageNeed);
			typename TSortedContainer::const_iterator end (src.end() );
			for(typename TSortedContainer::const_iterator i = src.begin(); i != end; ++i) {
				curOffSetCorrection = yintercept + static_cast<int64_t>(getRegLineSlopeCorrectionValue(slopenom, src.size(), count));
				offSetCorrectedId = static_cast<int64_t>(*i)  - curOffSetCorrection + idOffset;
				carr.set64(count, offSetCorrectedId);
				++count;
			}
		}
		else if (src.size() == 1) { //a single element
			destination.putUint8(0x40);
			destination.putVlPackedUint64(*(src.begin()));
		}
		else {
			destination.putUint8(0);
		}
		return true;
	}
};

class SortedOffsetIndexPrivateEmpty: public SortedOffsetIndexPrivate {
public:
	SortedOffsetIndexPrivateEmpty();
	virtual ~SortedOffsetIndexPrivateEmpty();
	virtual UByteArrayAdapter::OffsetType getSizeInBytes() const;
};


}}//end namespace


#endif