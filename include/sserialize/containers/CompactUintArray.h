#ifndef CompactUintArray_H
#define CompactUintArray_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/iterator/AtStlInputIterator.h>
#include <set>
#include <deque>
#include <ostream>
#include <algorithm>

//TODO: implement random-access-iterator
//Sub-classes only have to implement the 64 bit functions if needed

namespace sserialize {

class CompactUintArrayPrivate: public RefCountObject {
protected:
	UByteArrayAdapter m_data;
protected:
	void calcBegin(const uint32_t pos, UByteArrayAdapter::OffsetType & posStart, uint8_t & initShift, uint8_t bpn) const;
public:
	CompactUintArrayPrivate();
	CompactUintArrayPrivate(const UByteArrayAdapter & adap);
	virtual ~CompactUintArrayPrivate();
	UByteArrayAdapter & data() { return m_data; }
	
	virtual uint8_t bpn() const;

	virtual uint32_t at(uint32_t pos) const = 0;
	virtual uint64_t at64(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large then it sets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) = 0;
	virtual uint64_t set64(const uint32_t pos, uint64_t value);
};

class CompactUintArrayPrivateEmpty: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateEmpty();
	virtual ~CompactUintArrayPrivateEmpty() {}
	virtual uint8_t bpn() const;
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};

class CompactUintArrayPrivateVarBits: public CompactUintArrayPrivate {
private:
	uint8_t m_bpn;
	uint32_t m_mask;
public:
	CompactUintArrayPrivateVarBits(const UByteArrayAdapter & adap, uint8_t bpn);
	virtual ~CompactUintArrayPrivateVarBits() {}
	virtual uint8_t bpn() const;
	
	virtual uint32_t at(const uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};

class CompactUintArrayPrivateVarBits64: public CompactUintArrayPrivate {
private:
	uint8_t m_bpn;
	uint64_t m_mask;
public:
	CompactUintArrayPrivateVarBits64(const UByteArrayAdapter & adap, uint8_t bpn);
	virtual ~CompactUintArrayPrivateVarBits64() {}
	virtual uint8_t bpn() const;
	
	virtual uint32_t at(const uint32_t pos) const;
	virtual uint64_t at64(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
	virtual uint64_t set64(const uint32_t pos, uint64_t value);
};

class CompactUintArrayPrivateU8: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU8(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU8() {}
	virtual uint8_t bpn() const;
	virtual uint32_t at(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};

class CompactUintArrayPrivateU16: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU16(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU16() {}
	virtual uint8_t bpn() const;
	virtual uint32_t at(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};

class CompactUintArrayPrivateU24: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU24(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU24() {}
	virtual uint8_t bpn() const;
	virtual uint32_t at(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};

class CompactUintArrayPrivateU32: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU32(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU32() {}
	virtual uint8_t bpn() const;
	virtual uint32_t at(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value);
};


/** This class implements a variable sized uint32_t store. 
 *  You can set the amount of bits used per number.
 *  Numbers are encoded in Little-Endian (smaller-valued bits first)
 *
 *
 */
class CompactUintArray: public RCWrapper<CompactUintArrayPrivate> {
public:
	typedef uint64_t value_type;
	static constexpr uint32_t npos = 0xFFFFFFFF;

	struct CompactUintArrayIteratorDerefer {
		inline CompactUintArray::value_type operator()(const CompactUintArray & c, uint32_t pos) const { return c.at64(pos); }
	};

	typedef RCWrapper< sserialize::CompactUintArrayPrivate > MyBaseClass;
	typedef ReadOnlyAtStlIterator<CompactUintArray, value_type, int64_t, CompactUintArrayIteratorDerefer> const_iterator;
private:
	uint32_t m_maxCount;
protected:
	void setPrivate(const UByteArrayAdapter & array, uint8_t bitsPerNumber);
	CompactUintArray(CompactUintArrayPrivate * priv);
public:
	CompactUintArray();
	CompactUintArray(const UByteArrayAdapter & array, uint8_t bitsPerNumber);
	CompactUintArray(const UByteArrayAdapter & array, uint8_t bitsPerNumber, uint32_t max_size);
	CompactUintArray(const CompactUintArray & cua);
	~CompactUintArray();
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	CompactUintArray & operator=(const CompactUintArray& carr);
	uint8_t bpn() const;
	
	int32_t findSorted(uint32_t key, int32_t len) const;
	
	uint32_t at(uint32_t pos) const;
	uint64_t at64(uint32_t pos) const;
	inline uint32_t maxCount() const { return m_maxCount; }
	bool reserve(uint32_t newMaxCount);
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;

	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	uint32_t set(const uint32_t pos, const uint32_t value);
	uint64_t set64(const uint32_t pos, const uint64_t value);
	std::ostream& dump(std::ostream& out, uint32_t len);
	void dump();
	
	inline const_iterator begin() const { return const_iterator(0, *this); }
	inline const_iterator cbegin() const { return const_iterator(0, *this); }

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	template<typename T_SOURCE_CONTAINER>
	static uint8_t create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, uint8_t bits);

	template<typename T_IT>
	static uint8_t create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, uint8_t bits);

// 	template<typename T_SOURCE_CONTAINER>
// 	static bool createFromSet(const T_SOURCE_CONTAINER & src, std::deque<uint8_t> & dest, uint8_t bits);

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	///@return returns the needed bits, 0 on failure
	template<typename T_SOURCE_CONTAINER>
	static uint8_t create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest);

// 	template<typename T_SOURCE_CONTAINER>
// 	static uint8_t create(const T_SOURCE_CONTAINER & src, std::deque< uint8_t >& dest);

	static uint8_t minStorageBits(const uint32_t number);
	static uint8_t minStorageBitsFullBytes(uint32_t number);
	static uint8_t minStorageBits64(const uint64_t number);
	static uint8_t minStorageBitsFullBytes64(const uint64_t number);
	static UByteArrayAdapter::OffsetType minStorageBytes(uint8_t bpn, uint32_t count);
	inline bool operator!=(const CompactUintArray & other) const { return data() != other.data(); }
	inline bool operator==(const CompactUintArray & other) const { return priv() == other.priv() || data() == other.data(); }
};

template<typename T_SOURCE_CONTAINER>
uint8_t CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		uint8_t bits = minStorageBits64(maxElem);
		return create(src, dest, bits);
	}
	return 0;
}

template<typename T_SOURCE_CONTAINER>
uint8_t CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, uint8_t bits) {
	UByteArrayAdapter::OffsetType spaceNeed = minStorageBytes(bits, src.size());
	if (!dest.reserveFromPutPtr(spaceNeed))
		return 0;
	UByteArrayAdapter data(dest);
	data.shrinkToPutPtr();
	CompactUintArray carr(data, bits);
	uint32_t pos = 0;
	for(typename T_SOURCE_CONTAINER::const_iterator it(src.begin()), end(src.end()); it != end; ++it, ++pos) {
		carr.set64(pos, *it);
	}
	dest.incPutPtr(spaceNeed);
	return bits;
}

template<typename T_IT>
uint8_t CompactUintArray::create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, uint8_t bits) {
	UByteArrayAdapter::OffsetType spaceNeed = minStorageBytes(bits, std::distance(begin, end));
	if (!dest.reserveFromPutPtr(spaceNeed))
		return 0;
	UByteArrayAdapter data(dest);
	data.shrinkToPutPtr();
	CompactUintArray carr(data, bits);
	uint32_t pos = 0;
	for(; begin != end; ++begin, ++pos) {
		carr.set64(pos, *begin);
	}
	dest.incPutPtr(spaceNeed);
	return bits;
}

// template<typename T_SOURCE_CONTAINER>
// bool CompactUintArray::createFromSet(const T_SOURCE_CONTAINER & src, std::deque<uint8_t> & dest, uint8_t bpn) {
// 	std::deque<uint8_t> tmpDest;
// 	UByteArrayAdapter adap(&tmpDest);
// 	bool ok = bpn == create(src, adap, bpn);
// 	if (ok)
// 		dest.insert(dest.end(), tmpDest.begin(), tmpDest.end());
// 	return ok;
// }

// template<typename T_SOURCE_CONTAINER>
// uint8_t CompactUintArray::create(const T_SOURCE_CONTAINER & src, std::deque< uint8_t >& dest) {
// 	std::deque<uint8_t> tmpDest;
// 	UByteArrayAdapter adap(&tmpDest);
// 	uint8_t bpn = create(src, adap);
// 	if (bpn > 0)
// 		dest.insert(dest.end(), tmpDest.begin(), tmpDest.end());
// 	return bpn;
// }

/** Storage Layout of BoundedCompactUintArray
  *------------------------------------------
  *Size/Bits|CompactUintArray
  *------------------------------------------
  *   vu64  |
  * 
  */

class BoundedCompactUintArray: public CompactUintArray {
private:
	SizeType m_size;
private:
	uint32_t set(const uint32_t pos, const uint32_t value);
	uint64_t set64(const uint32_t pos, const uint64_t value);
	bool reserve(uint32_t newMaxCount);
public:
	BoundedCompactUintArray() : m_size(0) {}
	BoundedCompactUintArray(const sserialize::UByteArrayAdapter & d);
	BoundedCompactUintArray(const BoundedCompactUintArray & other);
	virtual ~BoundedCompactUintArray();
	BoundedCompactUintArray & operator=(const BoundedCompactUintArray & other);
	inline SizeType size() const { return m_size; }
	   UByteArrayAdapter::OffsetType getSizeInBytes() const;
	///Creates a new BoundedCompactUintArray beginning at dest.tellPutPtr()
	template<typename T_SOURCE_CONTAINER>
	static uint8_t create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest);
	inline const_iterator end() const { return const_iterator(m_size, *this);}
	inline const_iterator cend() const { return const_iterator(m_size, *this);}
	template<typename T_OUTPUT_ITERATOR>
	void copyInto(T_OUTPUT_ITERATOR out);
};

template<typename T_SOURCE_CONTAINER>
uint8_t BoundedCompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		uint8_t bits = minStorageBits64(maxElem);
		dest.putVlPackedUint64(src.size() << 6 | (bits-1));
		return CompactUintArray::create(src, dest, bits);
	}
	else {
		dest.putVlPackedUint64(0);
		return 1;
	}
}

template<typename T_OUTPUT_ITERATOR>
void BoundedCompactUintArray::copyInto(T_OUTPUT_ITERATOR out) {
	for(uint32_t i(0), s(size()); i < s; ++i, ++out) {
		*out = at64(i);
	}
}

}

#endif