#ifndef CompactUintArray_H
#define CompactUintArray_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <set>
#include <deque>
#include <ostream>

//TODO: implement random-access-iterator
//BUG:Overflowbugs for pos m_bpn*pos > 32bit
//Sub-classes only have to implement the 64 bit functions if needed

namespace sserialize {

class CompactUintArrayPrivate {
protected:
	UByteArrayAdapter m_data;
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
private:
	uint32_t m_maxCount;
public:
	CompactUintArray();
	CompactUintArray(const UByteArrayAdapter & array, uint8_t bitsPerNumber);
	CompactUintArray(const CompactUintArray & cua);
	~CompactUintArray();
	CompactUintArray & operator=(const CompactUintArray& carr);
	uint8_t bpn() const;
	
	int32_t findSorted(uint32_t key, int32_t len) const;
	
	uint32_t at(uint32_t pos) const;
	uint64_t at64(uint32_t pos) const;
	inline uint32_t maxCount() const { return m_maxCount; }
	bool reserve(uint32_t newMaxCount);

	/** @param: returns the value set (i.e. if value is to large the nit ets masked */
	uint32_t set(const uint32_t pos, const uint32_t value);
	uint64_t set64(const uint32_t pos, const uint64_t value);
	std::ostream& dump(std::ostream& out, uint32_t len);
	void dump();

	static bool createFromSet(const std::deque<uint32_t> & src, std::deque<uint8_t> & dest, uint8_t bpn);
	/** @return returns the needed bits, 0 on failure */
	static uint8_t createFromDeque(const std::deque< uint32_t >& src, std::deque< uint8_t >& dest);
	static uint8_t minStorageBits(const uint32_t number);
	static uint8_t minStorageBitsFullBytes(uint32_t number);
	static uint8_t minStorageBits64(const uint64_t number);
	static uint8_t minStorageBitsFullBytes64(const uint64_t number);
	static uint32_t minStorageBytes(uint8_t bpn, uint32_t count);
};

}

#endif