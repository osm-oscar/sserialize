#ifndef SSERIALIZE_UBYTE_ARRAY_ADAPTER_H
#define SSERIALIZE_UBYTE_ARRAY_ADAPTER_H
#include <sserialize/utility/types.h>
#include <stdint.h>
#include <iterator>
#include <deque>
#include <vector>
#include <string>
#include <memory>

//TODO: prevent segfaults of UByteArrayAdapter is empty (m_priv is 0)

/** This is the main storage abstraction class.
  * It gives a unified view on an Array of uint8_t.
  * The underlying real storage can vary and is ref-counted but not cowed
  * 
  * Bytes sizes:
  * Bits translate to Bits/8 Bytes
  * (Negative)OffsetType: 5
  * vlpacked:
  * (u)int32: 1-5
  * (u)int64: 1-9
  * 
  */

namespace sserialize {

class MmappedFile;
class UByteArrayAdapterPrivate;
class ChunkedMmappedFile;
class CompressedMmappedFile;

class UByteArrayAdapter: public std::iterator<std::random_access_iterator_tag, uint8_t, int> {
public:
	typedef sserialize::OffsetType OffsetType;
	typedef sserialize::NegativeOffsetType NegativeOffsetType;
	typedef sserialize::OffsetType SizeType;
private:
	/** Data is at offset, not at base address **/
	std::shared_ptr<UByteArrayAdapterPrivate> m_priv;
	OffsetType m_offSet;
	OffsetType m_len;
	OffsetType m_getPtr;
	OffsetType m_putPtr;

	static std::string m_tempFilePrefix;
	
private:
	explicit UByteArrayAdapter(const std::shared_ptr<UByteArrayAdapterPrivate> & priv);
	explicit UByteArrayAdapter(const std::shared_ptr<UByteArrayAdapterPrivate> & priv, OffsetType offSet, OffsetType len);
	bool resizeForPush(OffsetType pos, OffsetType length);
// 	void moveAndResize(uint32_t offset, unsigned int smallerLen);

public:
	/**len has to be larger than 0! **/
	UByteArrayAdapter();
	UByteArrayAdapter(const UByteArrayAdapter & adapter);
	/** @param addOffset add offset to beginning of the array, put/get ptrs stay where they are or are clipped */
	UByteArrayAdapter(const UByteArrayAdapter & adapter, OffsetType addOffset);
	/** @param addOffset add offset to beginning of the array, put/get ptrs stay where they are or are clipped */
// 	UByteArrayAdapter(const UByteArrayAdapter & adapter, int addOffset);
	/** @param addOffset add offset to beginning of the array, put/get ptrs stay where they are or are clipped */
	UByteArrayAdapter(const UByteArrayAdapter & adapter, OffsetType addOffset, OffsetType smallerLen);
	UByteArrayAdapter(uint8_t * data, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(std::deque<uint8_t> * data, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(std::deque<uint8_t> * data);
	UByteArrayAdapter(std::deque<uint8_t> * data, bool deleteOnClose);
	UByteArrayAdapter(std::vector<uint8_t> * data, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(std::vector<uint8_t> * data);
	UByteArrayAdapter(std::vector<uint8_t> * data, bool deleteOnClose);
	
	UByteArrayAdapter(sserialize::MmappedFile file, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(sserialize::MmappedFile file);
	UByteArrayAdapter(const ChunkedMmappedFile & file);
	UByteArrayAdapter(const CompressedMmappedFile & file);
	~UByteArrayAdapter();
	UByteArrayAdapter & operator=(const UByteArrayAdapter & node);
	uint8_t & operator[](const OffsetType pos);
	const uint8_t & operator[](const OffsetType pos) const;
	uint8_t & operator*();
	const uint8_t & operator*() const;
	UByteArrayAdapter operator+(OffsetType offSet) const;
	UByteArrayAdapter operator++(int offSet);
	UByteArrayAdapter operator--(int offSet);
	UByteArrayAdapter& operator++();
	UByteArrayAdapter& operator--();
	UByteArrayAdapter& operator+=(OffsetType offSet);
// 	UByteArrayAdapter& operator-=(unsigned int offSet);
	UByteArrayAdapter begin() const;
	UByteArrayAdapter end() const;
	
	bool equal(const sserialize::UByteArrayAdapter& b) const;
	bool equalContent(const sserialize::UByteArrayAdapter & b) const;
	bool equalContent(const std::deque<uint8_t> & b) const;

	//ptrs
	OffsetType tellPutPtr() const;
	void incPutPtr(OffsetType num);
	void decPutPtr(OffsetType num);
	void setPutPtr(OffsetType pos);
	void resetPutPtr();
	/** Moves the offset to the putPtr */
	UByteArrayAdapter& shrinkToPutPtr();

	OffsetType tellGetPtr() const;
	bool getPtrHasNext() const;
	void incGetPtr(OffsetType num);
	void decGetPtr(OffsetType num);
	void setGetPtr(OffsetType pos);
	void resetGetPtr();
	/** Moves the offset to the getPtr */
	UByteArrayAdapter& shrinkToGetPtr();

	void resetPtrs();


	/** tries to shrink the underlying data source, use with caution, others Adapter are not notified of this change */
	bool shrinkStorage(OffsetType byte);
	/** tries to grow the storage of this adapter by byte bytes.
	  * If the underlying storage is already larege enough, then no additional storage will be allocates */
	bool growStorage(OffsetType byte);
	bool resize(OffsetType byte);

	void setDeleteOnClose(bool del);

	uint8_t at(OffsetType pos) const;
	inline OffsetType size() const { return m_len;};
	inline OffsetType offset() const { return m_offSet;}
	inline bool isEmpty() const { return (m_len == 0);}


	int64_t getInt64(const OffsetType pos) const;
	uint64_t getUint64(const OffsetType pos) const;
	int32_t getInt32(const OffsetType pos) const;
	uint32_t getUint32(const OffsetType pos) const;
	uint32_t getUint24(const OffsetType pos) const;
	uint16_t getUint16(const OffsetType pos) const;
	uint8_t getUint8(const OffsetType pos) const;

	uint64_t getVlPackedUint64(const OffsetType pos, int* length) const;
	int64_t getVlPackedInt64(const OffsetType pos, int* length) const;
	uint32_t getVlPackedUint32(const OffsetType pos, int* length) const;
	int32_t getVlPackedInt32(const OffsetType pos, int* length) const;
	
	//Offset storage
	OffsetType getOffset(const OffsetType pos) const;
	NegativeOffsetType getNegativeOffset(const OffsetType pos) const;
	
	//returns an empty string if length is invalid
	std::string getString(const OffsetType pos, int * length = 0) const;
	//returns an empty string if length is invalid
	UByteArrayAdapter getStringData(const OffsetType pos, int * length = 0) const;
	OffsetType get(const OffsetType pos, uint8_t * dest, OffsetType len) const;


	/** If the supplied memory is not writable then you're on your own! **/

	bool putUint64(const OffsetType pos, const uint64_t value);
	bool putInt64(const OffsetType pos, const int64_t value);
	bool putInt32(const OffsetType pos, const int32_t value);
	bool putUint32(const OffsetType pos, const uint32_t value);
	bool putUint24(const OffsetType pos, const uint32_t value);
	bool putUint16(const OffsetType pos, const uint16_t value);
	bool putUint8(const OffsetType pos, const uint8_t value);
	
	bool putOffset(const OffsetType pos, const OffsetType value);
	bool putNegativeOffset(const OffsetType pos, const NegativeOffsetType value);

	/** @return: Length of the number, -1 on failure **/
	int putVlPackedUint64(const OffsetType pos, const uint64_t value);
	int putVlPackedInt64(const OffsetType pos, const int64_t value);
	
	int putVlPackedUint32(const OffsetType pos, const uint32_t value);
	int putVlPackedPad4Uint32(const OffsetType pos, const uint32_t value);
	int putVlPackedInt32(const OffsetType pos, const int32_t value);
	int putVlPackedPad4Int32(const OffsetType pos, const int32_t value);

	//complex objects
	/** @return number of bytes added, -1 if failed */
	int put(const OffsetType pos, const std::string & str);
	bool put(OffsetType pos, const uint8_t * data, uint32_t len);
	bool put(const OffsetType pos, const std::deque<uint8_t> & data);
	bool put(const OffsetType pos, const std::vector<uint8_t> & data);
	bool put(const OffsetType pos, const UByteArrayAdapter & data);

//streaming api
	OffsetType getOffset();
	NegativeOffsetType getNegativeOffset();

	uint64_t getUint64();
	int64_t getInt64();
	int32_t getInt32();
	uint32_t getUint32();
	uint32_t getUint24();
	uint16_t getUint16();
	uint8_t getUint8();

	uint64_t getVlPackedUint64();
	int64_t getVlPackedInt64();
	uint32_t getVlPackedUint32();
	int32_t getVlPackedInt32();
	
	///@return number of uint8_t read, @param len: maxnumber of uint8_t to read
	OffsetType get(uint8_t * dest, OffsetType len);
	
	std::string getString();
	UByteArrayAdapter getStringData();

	bool putOffset(const OffsetType value);
	bool putNegativeOffset(const NegativeOffsetType value);
	
	bool putInt64(const int64_t value);
	bool putUint64(const uint64_t value);
	
	bool putInt32(const int32_t value);
	bool putUint32(const uint32_t value);
	bool putUint24(const uint32_t value);
	bool putUint16(const uint16_t value);
	bool putUint8(const uint8_t value);

	int putVlPackedUint64(const uint64_t value);
	int putVlPackedInt64(const int64_t value);
	
	int putVlPackedUint32(const uint32_t value);
	int putVlPackedPad4Uint32(const uint32_t value);
	int putVlPackedInt32(const int32_t value);
	int putVlPackedPad4Int32(const int32_t value);

	bool put(const std::string & str);
	bool put(const uint8_t * data, OffsetType len);
	bool put(const std::deque<uint8_t> & data);
	bool put(const std::vector<uint8_t> & data);
	bool put(const UByteArrayAdapter & data);
	
	std::string toString() const;
	
	void dump(uint32_t byteCount) const;
	void dumpAsString(uint32_t byteCount) const;

	UByteArrayAdapter writeToDisk(std::string fileName, bool deleteOnClose = true);
	static UByteArrayAdapter createCache(OffsetType size, bool forceFileBase);
	static UByteArrayAdapter createFile(OffsetType size, std::string fileName);
	static UByteArrayAdapter open(const std::string & fileName);
	static UByteArrayAdapter openRo(const std::string & fileName, bool compressed, OffsetType maxFullMapSize, uint8_t chunkSizeExponent);
	static std::string getTempFilePrefix();
	static void setTempFilePrefix(const std::string & path);
	
	static inline uint32_t OffsetTypeSerializedLength() { return 5; }
};

}//end namespace

//Streaming operators


// sserialize::UByteArrayAdapter& operator--(sserialize::UByteArrayAdapter& a);
// sserialize::UByteArrayAdapter& operator++(sserialize::UByteArrayAdapter& a);

/** Iterator equality comparisson, does not check if a contains the same as b */ 
bool operator==(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b);

/** Iterator equality comparisson, does not check if a contains the same as b */ 
bool operator!=(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b);

bool operator==(const std::deque<uint8_t> & a, const sserialize::UByteArrayAdapter & b);
bool operator!=(const std::deque<uint8_t> & a, const sserialize::UByteArrayAdapter & b);
bool operator==(const sserialize::UByteArrayAdapter& b, const std::deque<uint8_t> & a);
bool operator!=(const sserialize::UByteArrayAdapter& b, const std::deque<uint8_t> & a);


sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const int64_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint64_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const int32_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint32_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint16_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint8_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const std::string & value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const std::deque<uint8_t> & value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const std::vector<uint8_t> & value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const sserialize::UByteArrayAdapter & value);

sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, int64_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, uint64_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, int32_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, uint32_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, uint16_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, uint8_t & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, std::string & value);




#endif
