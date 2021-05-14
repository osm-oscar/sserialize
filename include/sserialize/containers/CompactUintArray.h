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
public:
	using SizeType = sserialize::SizeType;
	using Bits = uint32_t;
	using value_type = uint64_t;
public:
	CompactUintArrayPrivate();
	CompactUintArrayPrivate(const UByteArrayAdapter & adap);
	virtual ~CompactUintArrayPrivate();
	UByteArrayAdapter & data() { return m_data; }
	
	virtual Bits bpn() const;

	virtual value_type at(SizeType pos) const = 0;
	/** @param: returns the value set (i.e. if value is to large then it sets masked */
	virtual value_type set(const SizeType pos, value_type value) = 0;
protected:
	UByteArrayAdapter m_data;
protected:
	void calcBegin(const SizeType pos, sserialize::UByteArrayAdapter::OffsetType& posStart, uint8_t& initShift, Bits bpn) const;
};

class CompactUintArrayPrivateEmpty: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateEmpty();
	virtual ~CompactUintArrayPrivateEmpty();
	virtual Bits bpn() const override;
	virtual value_type at(SizeType pos) const override;
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateVarBits: public CompactUintArrayPrivate {
private:
	uint32_t m_mask;
	Bits m_bpn;
public:
	CompactUintArrayPrivateVarBits(const UByteArrayAdapter & adap, Bits bpn);
	virtual ~CompactUintArrayPrivateVarBits();
	virtual Bits bpn() const override;
	
	virtual value_type at(const SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateVarBits64: public CompactUintArrayPrivate {
private:
	uint64_t m_mask;
	Bits m_bpn;
public:
	CompactUintArrayPrivateVarBits64(const UByteArrayAdapter & adap, Bits bpn);
	virtual ~CompactUintArrayPrivateVarBits64();
	virtual Bits bpn() const override;
	
	virtual value_type at(const SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateU8: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU8(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU8();
	virtual Bits bpn() const override;
	virtual value_type at(SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateU16: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU16(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU16();
	virtual Bits bpn() const override;
	virtual value_type at(SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateU24: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU24(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU24();
	virtual Bits bpn() const override;
	virtual value_type at(SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};

class CompactUintArrayPrivateU32: public CompactUintArrayPrivate {
public:
	CompactUintArrayPrivateU32(const UByteArrayAdapter& adap);
	virtual ~CompactUintArrayPrivateU32();
	virtual Bits bpn() const override;
	virtual value_type at(SizeType pos) const override;
	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	virtual value_type set(const SizeType pos, value_type value) override;
};


/** This class implements a variable sized uint32_t store. 
 *  You can set the amount of bits used per number.
 *  Numbers are encoded in Little-Endian (smaller-valued bits first)
 *
 *
 */
class CompactUintArray: public RCWrapper<CompactUintArrayPrivate> {
public:
	using value_type = CompactUintArrayPrivate::value_type;
	using SizeType = CompactUintArrayPrivate::SizeType;
	using Bits = CompactUintArrayPrivate::Bits;
	static constexpr SizeType npos = std::numeric_limits<SizeType>::max();

	struct CompactUintArrayIteratorDerefer {
		inline CompactUintArray::value_type operator()(const CompactUintArray * c, SizeType pos) const { return c->at64(pos); }
	};

	typedef RCWrapper< sserialize::CompactUintArrayPrivate > MyBaseClass;
	typedef ReadOnlyAtStlIterator<const CompactUintArray *, value_type, SizeType, CompactUintArrayIteratorDerefer> const_iterator;
private:
	SizeType m_maxCount;
protected:
	void setPrivate(const UByteArrayAdapter & array, Bits bitsPerNumber);
	void setMaxCount(SizeType maxCount);
	CompactUintArray(CompactUintArrayPrivate * priv);
public:
	CompactUintArray();
	CompactUintArray(const UByteArrayAdapter & array, Bits bitsPerNumber);
	CompactUintArray(const UByteArrayAdapter & array, Bits bitsPerNumber, SizeType max_size);
	CompactUintArray(const CompactUintArray & cua);
	~CompactUintArray();
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	CompactUintArray & operator=(const CompactUintArray& carr);
	Bits bpn() const;
	
	SizeType findSorted(value_type key, SizeType len) const;
	
	value_type at(SizeType pos) const;
	value_type at64(SizeType pos) const;
	inline SizeType maxCount() const { return m_maxCount; }
	bool reserve(SizeType newMaxCount);
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;

	/** @param: returns the value set (i.e. if value is too large then it gets masked */
	value_type set(const SizeType pos, const value_type value);
	value_type set64(const SizeType pos, const value_type value);
	std::ostream& dump(std::ostream& out, SizeType len);
	void dump();
	
	///takes shared-ownership
	const_iterator begin() const;
	
	///takes shared-ownership
	const_iterator cbegin() const;

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	template<typename T_SOURCE_CONTAINER>
	static Bits create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, Bits bits);

	template<typename T_IT>
	static Bits create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, Bits bits);

	///Creates a new CompactUintArray beginning at dest.tellPutPtr()
	///@return returns the needed bits, 0 on failure
	template<typename T_SOURCE_CONTAINER>
	static Bits create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest);

	static Bits minStorageBits(value_type number);
	static Bits minStorageBitsFullBytes(value_type number);
	static UByteArrayAdapter::OffsetType minStorageBytes(Bits bpn, SizeType count);
	bool operator!=(const CompactUintArray & other) const;
	bool operator==(const CompactUintArray & other) const;
};

template<typename T_SOURCE_CONTAINER>
CompactUintArray::Bits CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		Bits bits = minStorageBits(maxElem);
		return create(src, dest, bits);
	}
	return 0;
}

template<typename T_SOURCE_CONTAINER>
CompactUintArray::Bits CompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest, Bits bits) {
	UByteArrayAdapter::OffsetType spaceNeed = minStorageBytes(bits, narrow_check<SizeType>(src.size()));
	if (!dest.reserveFromPutPtr(spaceNeed)) {
		return 0;
	}
	UByteArrayAdapter data(dest);
	data.shrinkToPutPtr();
	CompactUintArray carr(data, bits);
	SizeType pos = 0;
	for(typename T_SOURCE_CONTAINER::const_iterator it(src.begin()), end(src.end()); it != end; ++it, ++pos) {
		carr.set64(pos, *it);
	}
	dest.incPutPtr(spaceNeed);
	return bits;
}

template<typename T_IT>
CompactUintArray::Bits CompactUintArray::create(T_IT begin, const T_IT & end, UByteArrayAdapter & dest, Bits bits) {
	UByteArrayAdapter::OffsetType spaceNeed;
	try {
		using std::distance;
		SizeType myDist = narrow_check<SizeType>( distance(begin, end) );
		spaceNeed = minStorageBytes(bits, myDist);
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
	SizeType pos = 0;
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
	value_type set(const SizeType pos, const value_type value) = delete;
	value_type set64(const SizeType pos, const value_type value) = delete;
	bool reserve(SizeType newMaxCount);
public:
	BoundedCompactUintArray() : m_size(0) {}
	BoundedCompactUintArray(const sserialize::UByteArrayAdapter & d);
	BoundedCompactUintArray(sserialize::UByteArrayAdapter & d, sserialize::UByteArrayAdapter::ConsumeTag);
	BoundedCompactUintArray(sserialize::UByteArrayAdapter const & d, sserialize::UByteArrayAdapter::NoConsumeTag);
	BoundedCompactUintArray(const BoundedCompactUintArray & other);
	virtual ~BoundedCompactUintArray();
	BoundedCompactUintArray & operator=(const BoundedCompactUintArray & other);
	inline SizeType size() const { return m_size; }
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	///Creates a new BoundedCompactUintArray beginning at dest.tellPutPtr()
	template<typename T_SOURCE_CONTAINER>
	static Bits create(const T_SOURCE_CONTAINER& src, sserialize::UByteArrayAdapter& dest);
	///Creates a new BoundedCompactUintArray beginning at dest.tellPutPtr()
	template<typename T_ITERATOR>
	static void create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter& dest);
	inline const_iterator end() const { return const_iterator(m_size, this);}
	inline const_iterator cend() const { return const_iterator(m_size, this);}
	template<typename T_OUTPUT_ITERATOR>
	void copyInto(T_OUTPUT_ITERATOR out);
};

template<typename T_SOURCE_CONTAINER>
BoundedCompactUintArray::Bits BoundedCompactUintArray::create(const T_SOURCE_CONTAINER & src, UByteArrayAdapter & dest) {
	if (src.size()) {
		typename T_SOURCE_CONTAINER::value_type maxElem = *std::max_element(src.begin(), src.end());
		Bits bits = minStorageBits(maxElem);
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
		Bits bits = minStorageBits(maxElem);
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
		for(SizeType i(0), s(size()); i < s; ++i, ++out) {
			*out = at(i);
		}
	}
	else {
		for(SizeType i(0), s(size()); i < s; ++i, ++out) {
			*out = at64(i);
		}
	}
}

std::ostream & operator<<(std::ostream & out, const BoundedCompactUintArray & src);

}

#endif
