#ifndef SSERIALIZE_MULTI_VAR_BIN_ARRAY_H
#define SSERIALIZE_MULTI_VAR_BIN_ARRAY_H
#include <vector>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>

namespace sserialize {

/** This is a class a bit like the CompactUintArray.
  * The difference is, that here it's possible to define multiple bit-lengths.
  * Suppose you have a struct:
  * struct {
  *      uint32_t bits:3;
  *      uint32_t bits:7;
  *      uint32_t bits:17;
  * }
  * And now you want to have an array out of these structs.
  * 
  * Limits:
  * The maximum bitSum is 0xFFFF
  * File format:
  * ----------------------------------------------------------
  * VERSION|count|configcount|bitconfig(sums)|data
  * ----------------------------------------------------------
  *     1     4        1     |compactuintarra|multivarbitarr 
  */
	
class MultiVarBitArrayPrivate: public RefCountObject {
private:
	UByteArrayAdapter m_data;
	std::vector<uint16_t> m_bitSums;
	uint32_t m_size;
public:
	MultiVarBitArrayPrivate();
	MultiVarBitArrayPrivate(const UByteArrayAdapter & data);
	MultiVarBitArrayPrivate(const std::vector< uint8_t >& bitConfig, const sserialize::UByteArrayAdapter& data);
	virtual ~MultiVarBitArrayPrivate();
	uint32_t size() const;
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t at(uint32_t pos, uint32_t subPos) const;
	uint32_t set(uint32_t pos, uint32_t subPos, uint32_t value);

	const UByteArrayAdapter & data() const;
	UByteArrayAdapter & data();
	void setSize(uint32_t newSize);

	uint16_t bitsPerEntry() const;
	uint32_t bitConfigCount() const;
	uint8_t bitCount(uint32_t pos) const;
};

class MultiVarBitArray: public RCWrapper<MultiVarBitArrayPrivate> {
protected:
	typedef RCWrapper<MultiVarBitArrayPrivate> MyParentClass;
public:
	MultiVarBitArray();
	MultiVarBitArray(const UByteArrayAdapter & data);
	MultiVarBitArray(const MultiVarBitArray & other);
	virtual ~MultiVarBitArray();
	MultiVarBitArray & operator=(const MultiVarBitArray & other);
	
	uint32_t size() const;
	///This is not valid if you used the second constructor with a given bitConfig
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t at(uint32_t pos, uint32_t subPos) const;
	uint32_t set(uint32_t pos, uint32_t subPos, uint32_t value);

	///return the data without the header
	const UByteArrayAdapter & data() const;
	///return the data without the header
	UByteArrayAdapter & data();
	void setSize(uint32_t newSize);

	uint16_t totalBitSum() const;
	uint32_t bitConfigCount() const;
	uint8_t bitCount(uint32_t pos) const;

	std::ostream & printStats(std::ostream & out) const;

// 	static MultiVarBitArray create(const std::vector< uint8_t >& bitConfig, sserialize::UByteArrayAdapter& destination, uint32_t initCount);
	static UByteArrayAdapter::OffsetType minStorageBytes(const std::vector<uint8_t> & bitConfig, const uint32_t count);
	static UByteArrayAdapter::OffsetType minStorageBytes(const uint32_t sum, const sserialize::UByteArrayAdapter::OffsetType count);
	enum { HEADER_SIZE=6}; //use const expr later
};

class MultiVarBitArrayCreator {
private:
	UByteArrayAdapter & m_data; //this holds the original dat ref
	UByteArrayAdapter m_header;
	MultiVarBitArray m_arr;
	uint32_t m_headerSize;
public:
	/** This will create MultiVarBitArray at the beginning of data.tellPutPtr() */
	MultiVarBitArrayCreator(const std::vector< uint8_t >& bitConfig, sserialize::UByteArrayAdapter& data);
	virtual ~MultiVarBitArrayCreator();
	bool reserve(uint32_t count);
	/** This will increase the Array to fit pos */
	bool set(uint32_t pos, uint32_t subPos, uint32_t value);
	
	/** Flushes everthing to that, adjusting the length of the data and returns the data-block associated with the created MultiVarBitArray */
	UByteArrayAdapter flush();
};


}//end namespace

#endif