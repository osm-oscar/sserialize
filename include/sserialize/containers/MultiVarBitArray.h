#ifndef SSERIALIZE_MULTI_VAR_BIN_ARRAY_H
#define SSERIALIZE_MULTI_VAR_BIN_ARRAY_H
#include <vector>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/debug.h>
#include <sserialize/storage/Size.h>
#include <sserialize/Static/Version.h>

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
  * 
  * struct MultiVarBitArray {
  *   SimpleVersion<1> version;
  *   Size size;
  *   uint8_t numberOfFields;
  *   CompactUintArray bitconfig(5, numberOfFields);
  *   UByteArrayAdapter data;
  * }
  * 
  */
	
class MultiVarBitArrayPrivate: public RefCountObject {
public:
	using value_type = uint64_t;
	using SizeType = Size;
	using Version = Static::SimpleVersion<1, MultiVarBitArrayPrivate>;
	static constexpr UByteArrayAdapter::SizeType HEADER_SIZE = SerializationInfo<Version>::length + SerializationInfo<SizeType>::length + SerializationInfo<uint8_t>::length;
private:
	UByteArrayAdapter m_data;
	std::vector<uint16_t> m_bitSums;
	SizeType m_size;
public:
	MultiVarBitArrayPrivate();
	MultiVarBitArrayPrivate(const UByteArrayAdapter & data);
	MultiVarBitArrayPrivate(const std::vector< uint8_t >& bitConfig, const sserialize::UByteArrayAdapter& data);
	virtual ~MultiVarBitArrayPrivate();
	SizeType size() const;
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	value_type at(SizeType pos, uint32_t subPos) const;
	value_type set(SizeType pos, uint32_t subPos, value_type value);

	const UByteArrayAdapter & data() const;
	UByteArrayAdapter & data();
	void setSize(SizeType newSize);

	uint16_t bitsPerEntry() const;
	uint32_t bitConfigCount() const;
	uint8_t bitCount(uint32_t pos) const;
};

class MultiVarBitArray: public RCWrapper<MultiVarBitArrayPrivate> {
public:
	using value_type = MultiVarBitArrayPrivate::value_type;
	using SizeType = MultiVarBitArrayPrivate::SizeType;
protected:
	typedef RCWrapper<MultiVarBitArrayPrivate> MyParentClass;
public:
	MultiVarBitArray();
	MultiVarBitArray(const UByteArrayAdapter & data);
	MultiVarBitArray(UByteArrayAdapter & data, UByteArrayAdapter::ConsumeTag);
	MultiVarBitArray(UByteArrayAdapter const & data, UByteArrayAdapter::NoConsumeTag);
	MultiVarBitArray(const MultiVarBitArray & other);
	virtual ~MultiVarBitArray();
	MultiVarBitArray & operator=(const MultiVarBitArray & other);
	
	SizeType size() const;
	///This is not valid if you used the second constructor with a given bitConfig
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	value_type at(SizeType pos, uint32_t subPos) const;
	value_type set(SizeType pos, uint32_t subPos, value_type value);

	///return the data without the header
	const UByteArrayAdapter & data() const;
	///return the data without the header
	UByteArrayAdapter & data();
	void setSize(SizeType newSize);

	uint16_t totalBitSum() const;
	uint32_t bitConfigCount() const;
	uint8_t bitCount(uint32_t pos) const;

	std::ostream & printStats(std::ostream & out) const;

// 	static MultiVarBitArray create(const std::vector< uint8_t >& bitConfig, sserialize::UByteArrayAdapter& destination, uint32_t initCount);
	static UByteArrayAdapter::OffsetType minStorageBytes(const std::vector<uint8_t> & bitConfig, const SizeType count);
	static UByteArrayAdapter::OffsetType minStorageBytes(const uint32_t sum, const sserialize::UByteArrayAdapter::OffsetType count);
	static constexpr UByteArrayAdapter::SizeType HEADER_SIZE=MultiVarBitArrayPrivate::HEADER_SIZE;
};

class MultiVarBitArrayCreator {
public:
	using value_type = MultiVarBitArray::value_type;
	using SizeType = MultiVarBitArray::SizeType;
private:
	UByteArrayAdapter & m_data; //this holds the original dat ref
	UByteArrayAdapter m_header;
	MultiVarBitArray m_arr;
	uint32_t m_headerSize;
public:
	/** This will create MultiVarBitArray at the beginning of data.tellPutPtr() */
	MultiVarBitArrayCreator(const std::vector< uint8_t >& bitConfig, sserialize::UByteArrayAdapter& data);
	virtual ~MultiVarBitArrayCreator();
	bool reserve(SizeType count);
	/** This will increase the Array to fit pos */
	bool set(SizeType pos, uint32_t subPos, value_type value);
	value_type at(SizeType pos, uint32_t subPos) const;
	
	/** Flushes everthing to that, adjusting the length of the data and returns the data-block associated with the created MultiVarBitArray */
	UByteArrayAdapter flush();
};


}//end namespace

#endif
