#ifndef SSERIALIZE_COMPACT_UINT_ARRAY_H
#define SSERIALIZE_COMPACT_UINT_ARRAY_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/algorithm/utilcontainerfuncs.h>
#include <sserialize/iterator/AtStlInputIterator.h>
#include <sserialize/utility/checks.h>
#include <algorithm>
#include <type_traits>
#include <iterator>

//Sub-classes only have to implement the 64 bit functions if needed

namespace sserialize {

class CompactUintArrayPrivate: public RefCountObject {
protected:
	UByteArrayAdapter m_data;
protected:
	void calcBegin(const uint32_t pos, sserialize::UByteArrayAdapter::OffsetType& posStart, uint8_t& initShift, uint32_t bpn) const;
public:
	CompactUintArrayPrivate();
	CompactUintArrayPrivate(const UByteArrayAdapter & adap);
	virtual ~CompactUintArrayPrivate();
	UByteArrayAdapter & data() { return m_data; }
	
	virtual uint32_t bpn() const;

	virtual uint32_t at(uint32_t pos) const = 0;
	virtual uint64_t at64(uint32_t pos) const;
	/** @param: returns the value set (i.e. if value is to large then it sets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) = 0;
	virtual uint64_t set64(const uint32_t pos, uint64_t value);
};

class CompactUintArrayPrivateEmpty: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateEmpty();
	virtual ~CompactUintArrayPrivateEmpty();
	virtual uint32_t bpn() const override;
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
};

class CompactUintArrayPrivateVarBits: public CompactUintArrayPrivate {
private:
	uint32_t m_mask;
	uint32_t m_bpn;
public:
	CompactUintArrayPrivateVarBits(const UByteArrayAdapter & adap, uint32_t bpn);
	virtual ~CompactUintArrayPrivateVarBits();
	virtual uint32_t bpn() const override;
	
	virtual uint32_t at(const uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
};

class CompactUintArrayPrivateVarBits64: public CompactUintArrayPrivate {
private:
	uint64_t m_mask;
	uint32_t m_bpn;
public:
	CompactUintArrayPrivateVarBits64(const UByteArrayAdapter & adap, uint32_t bpn);
	virtual ~CompactUintArrayPrivateVarBits64();
	virtual uint32_t bpn() const override;
	
	virtual uint32_t at(const uint32_t pos) const override;
	virtual uint64_t at64(uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
	virtual uint64_t set64(const uint32_t pos, uint64_t value) override;
};

class CompactUintArrayPrivateU8: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU8(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU8();
	virtual uint32_t bpn() const override;
	virtual uint32_t at(uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
};

class CompactUintArrayPrivateU16: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU16(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU16();
	virtual uint32_t bpn() const override;
	virtual uint32_t at(uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
};

class CompactUintArrayPrivateU24: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU24(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU24();
	virtual uint32_t bpn() const override;
	virtual uint32_t at(uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
};

class CompactUintArrayPrivateU32: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU32(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU32();
	virtual uint32_t bpn() const override;
	virtual uint32_t at(uint32_t pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual uint32_t set(const uint32_t pos, uint32_t value) override;
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
	typedef uint32_t SizeType;
	static constexpr uint32_t npos = 0xFFFFFFFF;

	struct CompactUintArrayIteratorDerefer {
		inline CompactUintArray::value_type operator()(const CompactUintArray * c, SizeType pos) const { return c->at64(pos); }
	};

	typedef RCWrapper< sserialize::CompactUintArrayPrivate > MyBaseClass;
	typedef ReadOnlyAtStlIterator<const CompactUintArray *, value_type, SizeType, CompactUintArrayIteratorDerefer> const_iterator;
private:
	SizeType m_maxCount;
protected:
	void setPrivate(const UByteArrayAdapter & array, uint32_t bitsPerNumber);
	void setMaxCount(SizeType maxCount);
	CompactUintArray(CompactUintArrayPrivate * priv);
public:
	CompactUintArray();
	CompactUintArray(const UByteArrayAdapter & array, uint32_t bitsPerNumber);
	CompactUintArray(const UByteArrayAdapter & array, uint32_t bitsPerNumber, uint32_t max_size);
	CompactUintArray(const CompactUintArray & cua);
	~CompactUintArray();
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	CompactUintArray & operator=(const CompactUintArray& carr);
	uint8_t bpn() const;
	
	uint32_t findSorted(uint32_t key, int32_t len) const;
	
	uint32_t at(SizeType pos) const;
	uint64_t at64(SizeType pos) const;
	inline uint32_t maxCount() const { return m_maxCount; }
	bool reserve(uint32_t newMaxCount);
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;

	/** @param: returns the value set (i.e. if value is too large then it gets masked */
	uint32_t set(const uint32_t pos, const uint32_t value);
	uint64_t set64(const uint32_t pos, const uint64_t value);
	std::ostream& dump(std::ostream& out, uint32_t len);
	void dump();
	
	///takes shared-ownership
	const_iterator begin() const;
	
	///takes shared-ownership
	const_iterator cbegin() const;

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	template<typename T_SOURCE_CONTAINER>
	static uint32_t create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, uint32_t bits);

	template<typename T_IT>
	static uint32_t create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, uint32_t bits);

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	///@return returns the needed bits, 0 on failure
	template<typename T_SOURCE_CONTAINER>
	static uint32_t create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest);

	static uint32_t minStorageBits(const uint32_t number);
	static uint32_t minStorageBitsFullBytes(uint32_t number);
	static uint32_t minStorageBits64(const uint64_t number);
	static uint32_t minStorageBits(const uint64_t number) { return minStorageBits64(number); }
	static uint32_t minStorageBitsFullBytes64(const uint64_t number);
	static UByteArrayAdapter::OffsetType minStorageBytes(uint32_t bpn, sserialize::UByteArrayAdapter::OffsetType count);
	bool operator!=(const CompactUintArray & other) const;
	bool operator==(const CompactUintArray & other) const;
};

template<typename T_SOURCE_CONTAINER>
uint32_t CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		uint32_t bits = minStorageBits64(maxElem);
		return create(src, dest, bits);
	}
	return 0;
}

template<typename T_SOURCE_CONTAINER>
uint32_t CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, uint32_t bits) {
	UByteArrayAdapter::OffsetType spaceNeed = minStorageBytes(bits, narrow_check<SizeType>(src.size()));
	if (!dest.reserveFromPutPtr(spaceNeed)) {
		return 0;
	}
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
uint32_t CompactUintArray::create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, uint32_t bits) {
	UByteArrayAdapter::OffsetType spaceNeed;
	try {
		using std::distance;
		uint32_t myDist = narrow_check<uint32_t>( distance(begin, end) );
		spaceNeed = minStorageBytes(bits, (uint32_t)myDist);
	}
	catch (const sserialize::TypeOverflowException &) {
		throw sserialize::TypeOverflowException("CompactUintArray::create");
	}
	if (!dest.reserveFromPutPtr(spaceNeed)) {
		throw sserialize::IOException("CompactUintArray::create could not allocate memory");
	}
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
	static uint32_t create(const T_SOURCE_CONTAINER& src, sserialize::UByteArrayAdapter& dest);
	///Creates a new BoundedCompactUintArray beginning at dest.tellPutPtr()
	template<typename T_ITERATOR>
	static void create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter& dest);
	inline const_iterator end() const { return const_iterator(m_size, this);}
	inline const_iterator cend() const { return const_iterator(m_size, this);}
	template<typename T_OUTPUT_ITERATOR>
	void copyInto(T_OUTPUT_ITERATOR out);
};

template<typename T_SOURCE_CONTAINER>
uint32_t BoundedCompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		uint32_t bits = minStorageBits64(maxElem);
		dest.putVlPackedUint64( static_cast<uint64_t>(src.size()) << 6 | (bits-1));
		return CompactUintArray::create(src, dest, bits);
	}
	else {
		dest.putVlPackedUint64(0);
		return 1;
	}
}

template<typename T_ITERATOR>
void BoundedCompactUintArray::create(T_ITERATOR begin, T_ITERATOR end, UByteArrayAdapter & dest) {
	using std::distance;
	auto size = distance(begin, end);
	if (size) {
		auto maxElem = *std::max_element(begin, end);
		uint32_t bits = minStorageBits64(maxElem);
		dest.putVlPackedUint64( static_cast<uint64_t>(size) << 6 | (bits-1));
		CompactUintArray::create(begin, end, dest, bits);
	}
	else {
		dest.putVlPackedUint64(0);
	}
}

template<typename T_OUTPUT_ITERATOR>
void BoundedCompactUintArray::copyInto(T_OUTPUT_ITERATOR out) {
	if (std::numeric_limits<typename std::iterator_traits<T_OUTPUT_ITERATOR>::value_type>::digits <= 32) {
		for(uint32_t i(0), s(size()); i < s; ++i, ++out) {
			*out = at(i);
		}
	}
	else {
		for(uint32_t i(0), s(size()); i < s; ++i, ++out) {
			*out = at64(i);
		}
	}
}

std::ostream & operator<<(std::ostream & out, const BoundedCompactUintArray & src);

}

#endif
