#ifndef SSERIALIZE_UBYTE_ARRAY_ADAPTER_H
#define SSERIALIZE_UBYTE_ARRAY_ADAPTER_H
#include <sserialize/utility/types.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/MmappedMemory.h>
#include <stdint.h>
#include <iterator>
#include <deque>
#include <vector>
#include <string>
#include <memory>

//TODO: prevent segfaults of UByteArrayAdapter is empty (m_priv is 0)
//TODO: split this class into UByteArrayAdapterIterator and UByteArrayAdapter to seperate concepts
//TODO: get rid of static std::string as that leads to double frees on incorrect linking
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
  * All functions that change the size of the underlying storage are NOT thread-safe.
  * 
  */

namespace sserialize {

class MmappedFile;
class UByteArrayAdapterPrivate;
class ChunkedMmappedFile;
class CompressedMmappedFile;

class UByteArrayAdapter: public std::iterator<std::random_access_iterator_tag, uint8_t, sserialize::SignedOffsetType> {
public:
	typedef sserialize::OffsetType OffsetType;
	typedef sserialize::SignedOffsetType NegativeOffsetType;
	typedef sserialize::OffsetType SizeType;
	
	class MemoryView final {
		friend class UByteArrayAdapter;
	public:
		typedef const uint8_t * const_iterator;
		typedef uint8_t * iterator;
	private:
		class MemoryViewImp final: public sserialize::RefCountObject {
			RCPtrWrapper<UByteArrayAdapterPrivate> m_dataBase;
			uint8_t * m_d;
			OffsetType m_off;
			OffsetType m_size;
			bool m_copy;
		public:
			MemoryViewImp(uint8_t * ptr, OffsetType off, OffsetType size, bool isCopy, UByteArrayAdapterPrivate * base);
			~MemoryViewImp();
			inline uint8_t * get() { return m_d; }
			inline const uint8_t * get() const { return m_d; }
			inline OffsetType size() const { return m_size; }
			inline bool isCopy() const { return m_copy; }
			bool flush(OffsetType len, OffsetType off);
		};
	private:
		RCPtrWrapper<MemoryViewImp> m_priv;
		///@param isCopy: if true, then ptr gets deleted by delete[]
		MemoryView(uint8_t * ptr, OffsetType off, OffsetType size, bool isCopy, UByteArrayAdapterPrivate * base) : m_priv(new MemoryViewImp(ptr, off, size, isCopy, base)) {}
	public:
		MemoryView() {}
		~MemoryView() {}
		uint8_t * get() { return m_priv->get();}
		const uint8_t * get() const { return m_priv->get();}
		uint8_t * begin() { return get(); }
		const uint8_t * cbegin() const { return get(); }
		const uint8_t * begin() const { return get(); }
		uint8_t * end() { return get()+size(); }
		const uint8_t * cend() const { return get()+size(); }
		const uint8_t * end() const { return get()+size(); }

		OffsetType size() const { return m_priv->size();}
		///If this is true, then writes are not passed through to the UBA, call flush() to do that
		bool isCopy() const { return m_priv->isCopy();}
		///flush up to len bytes starting from off
		bool flush(OffsetType len, OffsetType off = 0) { return m_priv->flush(len, off); }
		bool flush() { return flush(size()); }
	};
	
private:
	/** Data is at offset, not at base address **/
	RCPtrWrapper<UByteArrayAdapterPrivate> m_priv;
	OffsetType m_offSet;
	OffsetType m_len;
	OffsetType m_getPtr;
	OffsetType m_putPtr;

	static std::string m_tempFilePrefix;
	static std::string m_fastTempFilePrefix;
	static std::string m_logFilePrefix;
	
private:
	explicit UByteArrayAdapter(const RCPtrWrapper<UByteArrayAdapterPrivate> & priv);
	explicit UByteArrayAdapter(const RCPtrWrapper<UByteArrayAdapterPrivate> & priv, OffsetType offSet, OffsetType len);
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
	UByteArrayAdapter(const sserialize::MmappedMemory<uint8_t> & mem);
	~UByteArrayAdapter();
	UByteArrayAdapter & operator=(const UByteArrayAdapter & node);
	void zero();
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
	UByteArrayAdapter& operator-=(OffsetType offSet);
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
	UByteArrayAdapter& resetGetPtr();
	/** Moves the offset to the getPtr */
	UByteArrayAdapter& shrinkToGetPtr();

	UByteArrayAdapter & resetPtrs();


	/** tries to shrink the underlying data source, use with caution, others Adapter are not notified of this change */
	bool shrinkStorage(OffsetType byte);
	/** tries to grow the storage of this adapter by byte bytes.
	  * If the underlying storage is already larege enough, then no additional storage will be allocates */
	bool growStorage(OffsetType byte);
	
	///resize to @param byte bytes (grows but does not shrink the underlying storage)
	bool resize(OffsetType byte);

	void resetToStorage();

	///reserves @bytes bytes beginning at tellPutPtr()
	bool reserveFromPutPtr(OffsetType bytes);


	void setDeleteOnClose(bool del);

	uint8_t at(OffsetType pos) const;
	inline OffsetType size() const { return m_len;};
	inline OffsetType offset() const { return m_offSet;}
	inline bool isEmpty() const { return (m_len == 0);}
	
	///Returns a read/writable MemoryView. If you don't write to it, then this function behaves like a const function
	MemoryView getMemView(const OffsetType pos, OffsetType size);
	const MemoryView getMemView(const OffsetType pos, OffsetType size) const;
	inline MemoryView asMemView() { return getMemView(0, size());}
	const MemoryView asMemView() const { return getMemView(0, size());}
	
	int64_t getInt64(const OffsetType pos) const;
	uint64_t getUint64(const OffsetType pos) const;
	int32_t getInt32(const OffsetType pos) const;
	uint32_t getUint32(const OffsetType pos) const;
	uint32_t getUint24(const OffsetType pos) const;
	uint16_t getUint16(const OffsetType pos) const;
	uint8_t getUint8(const OffsetType pos) const;
	double getDouble(const OffsetType pos) const;
	float getFloat(const OffsetType pos) const;

	uint64_t getVlPackedUint64(const OffsetType pos, int* length) const;
	int64_t getVlPackedInt64(const OffsetType pos, int* length) const;
	uint32_t getVlPackedUint32(const OffsetType pos, int* length) const;
	int32_t getVlPackedInt32(const OffsetType pos, int* length) const;
	
	//Offset storage
	OffsetType getOffset(const OffsetType pos) const;
	NegativeOffsetType getNegativeOffset(const OffsetType pos) const;
	
	//returns an empty string if length is invalid
	std::string getString(const OffsetType pos, int * length = 0) const;
	inline uint32_t getStringLength(const OffsetType pos) { return getVlPackedUint32(pos);}
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
	bool putDouble(const OffsetType pos, const double value);
	bool putFloat(const OffsetType pos, const float value);
	
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
	bool put(const OffsetType pos, const uint8_t* data, OffsetType len);
	bool put(const OffsetType pos, const std::deque<uint8_t> & data);
	bool put(const OffsetType pos, const std::vector<uint8_t> & data);
	bool put(const OffsetType pos, const UByteArrayAdapter & data);
	bool put(const OffsetType pos, const MemoryView & data);

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
	double getDouble();
	float getFloat();

	uint64_t getVlPackedUint64();
	int64_t getVlPackedInt64();
	uint32_t getVlPackedUint32();
	int32_t getVlPackedInt32();
	
	///@return number of uint8_t read, @param len: maxnumber of uint8_t to read
	OffsetType get(uint8_t * dest, OffsetType len);
	
	inline uint32_t getStringLength() { return getVlPackedUint32();}
	UByteArrayAdapter getStringData();
	std::string getString();

	bool putOffset(const OffsetType value);
	bool putNegativeOffset(const NegativeOffsetType value);
	
	bool putInt64(const int64_t value);
	bool putUint64(const uint64_t value);
	
	bool putInt32(const int32_t value);
	bool putUint32(const uint32_t value);
	bool putUint24(const uint32_t value);
	bool putUint16(const uint16_t value);
	bool putUint8(const uint8_t value);
	bool putDouble(const double value);
	bool putFloat(const float value);

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
	static UByteArrayAdapter createCache(OffsetType size, sserialize::MmappedMemoryType mmt);
	static UByteArrayAdapter createFile(OffsetType size, std::string fileName);
	static UByteArrayAdapter open(const std::string & fileName);
	static UByteArrayAdapter openRo(const std::string & fileName, bool compressed, OffsetType maxFullMapSize = MAX_SIZE_FOR_FULL_MMAP, uint8_t chunkSizeExponent = CHUNKED_MMAP_EXPONENT);
	static std::string getTempFilePrefix();
	static std::string getFastTempFilePrefix();
	static std::string getLogFilePrefix();
	static void setTempFilePrefix(const std::string & path);
	static void setFastTempFilePrefix(const std::string & path);
	static void setLogFilePrefix(const std::string & path);
	
	static inline OffsetType OffsetTypeSerializedLength() { return 5; }
	enum {S_OffsetTypeSerializedLength=5};

	//convinience functions
	
	inline uint64_t getVlPackedUint64(const OffsetType pos) const { return getVlPackedUint64(pos, 0); }
	inline int64_t getVlPackedInt64(const OffsetType pos) const { return getVlPackedInt64(pos, 0); }
	inline uint32_t getVlPackedUint32(const OffsetType pos) const { return getVlPackedUint32(pos, 0); }
	inline int32_t getVlPackedInt32(const OffsetType pos) const { return getVlPackedInt32(pos, 0); }
	
	inline uint64_t getVlPackedUint64(const OffsetType pos, int & length) const { return getVlPackedUint64(pos, &length); }
	inline int64_t getVlPackedInt64(const OffsetType pos, int & length) const { return getVlPackedInt64(pos, &length); }
	inline uint32_t getVlPackedUint32(const OffsetType pos, int & length) const { return getVlPackedUint32(pos, &length); }
	inline int32_t getVlPackedInt32(const OffsetType pos, int & length) const { return getVlPackedInt32(pos, &length); }

	inline void put(UByteArrayAdapter::OffsetType pos, uint8_t value) { putUint8(pos, value); }
	inline void put(UByteArrayAdapter::OffsetType pos, uint16_t value) { putUint16(pos, value); }
	inline void put(UByteArrayAdapter::OffsetType pos, uint32_t value) { putUint32(pos, value); }
	inline void put(UByteArrayAdapter::OffsetType pos, uint64_t value) { putUint64(pos, value); }
	inline void put(UByteArrayAdapter::OffsetType pos, int32_t value) { putInt32(pos, value); }
	inline void put(UByteArrayAdapter::OffsetType pos, int64_t value) { putInt64(pos, value); }
	
	inline void get(UByteArrayAdapter::OffsetType pos, uint8_t & value) const { value = getUint8(pos); }
	inline void get(UByteArrayAdapter::OffsetType pos, uint16_t & value) const { value = getUint16(pos); }
	inline void get(UByteArrayAdapter::OffsetType pos, uint32_t & value) const { value = getUint32(pos); }
	inline void get(UByteArrayAdapter::OffsetType pos, uint64_t & value) const { value = getUint64(pos); }
	inline void get(UByteArrayAdapter::OffsetType pos, int32_t & value) const { value = getInt32(pos); }
	inline void get(UByteArrayAdapter::OffsetType pos, int64_t & value) const { value = getInt64(pos); }
	
	inline void get(uint8_t & value) { value = getUint8(); }
	inline void get(uint16_t & value) { value = getUint16(); }
	inline void get(uint32_t & value) { value = getUint32(); }
	inline void get(uint64_t & value) { value = getUint64(); }
	inline void get(int32_t & value) { value = getInt32(); }
	inline void get(int64_t & value) { value = getInt64(); }
	inline void get(double & value) { value = getDouble(); }
	inline void get(float & value) { value = getFloat(); }
	
	template<typename TValue>
	TValue get(UByteArrayAdapter::OffsetType pos) const {
		TValue v;
		get(pos, v);
		return v;
	}
	
	template<typename TValue>
	TValue get() {
		TValue v;
		get(v);
		return v;
	}
	
	template<typename TValue>
	void put(UByteArrayAdapter::OffsetType pos, TValue v) {
		put(pos, v);
	}
	
	//BUG: WTF? This will horibly go wrong if put(v) does not exist. Who coded this shit? (you did!)
	template<typename TValue>
	void put(TValue v) {
		put(v);
	}
	
	static void putUint8(UByteArrayAdapter & dest, uint8_t src);
	static void putUint16(UByteArrayAdapter & dest, uint16_t src);
	static void putUint24(UByteArrayAdapter & dest, uint32_t src);
	static void putUint32(UByteArrayAdapter & dest, uint32_t src);
	static void putInt32(UByteArrayAdapter & dest, int32_t src);
	static void putUint64(UByteArrayAdapter & dest, uint64_t src);
	static void putInt64(UByteArrayAdapter & dest, int64_t src);
	static void putDouble(UByteArrayAdapter & dest, double src);
	static void putFloat(UByteArrayAdapter & dest, float src);
	static void putVlPackedInt32(UByteArrayAdapter & dest, int32_t src);
	static void putVlPackedUint32(UByteArrayAdapter & dest, uint32_t src);
	static void putVlPackedInt64(UByteArrayAdapter & dest, int64_t src);
	static void putVlPackedUint64(UByteArrayAdapter & dest, uint64_t src);
	
	template<typename TValue>
	struct SerializationSupport {
		static const bool value = false;
	};
	
	template<typename TValue>
	struct StreamingSerializer {
		void operator()(const TValue & src, UByteArrayAdapter & dest) const {
			dest << src;
		}
	};
	
	template<typename TValue>
	struct Deserializer {
		TValue operator()(const UByteArrayAdapter & dest) const {
			return TValue(dest);
		}
	};
};

template<>
struct UByteArrayAdapter::SerializationSupport<uint8_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<uint16_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<uint32_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<uint64_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<int32_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<int64_t> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<double> {
	static const bool value = true;
};

template<>
struct UByteArrayAdapter::SerializationSupport<float> {
	static const bool value = true;
};

#define UBA_DESERIALIZER_SPECIALIZATIONS(__TYPE, __FUNCNAME) \
template<> \
struct UByteArrayAdapter::Deserializer<__TYPE> { \
	inline __TYPE operator()(const UByteArrayAdapter & dest) const { \
		return dest.__FUNCNAME(0); \
	} \
}; \

UBA_DESERIALIZER_SPECIALIZATIONS(uint8_t, getUint8);
UBA_DESERIALIZER_SPECIALIZATIONS(uint16_t, getUint16);
UBA_DESERIALIZER_SPECIALIZATIONS(uint32_t, getUint32);
UBA_DESERIALIZER_SPECIALIZATIONS(uint64_t, getUint64);
UBA_DESERIALIZER_SPECIALIZATIONS(int32_t, getInt32);
UBA_DESERIALIZER_SPECIALIZATIONS(int64_t, getInt64);
UBA_DESERIALIZER_SPECIALIZATIONS(float, getFloat);
UBA_DESERIALIZER_SPECIALIZATIONS(double, getDouble);
UBA_DESERIALIZER_SPECIALIZATIONS(std::string, getString);

#undef UBA_DESERIALIZER_SPECIALIZATIONS



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
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const double value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const float value);
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
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, double & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, float & value);
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, std::string & value);

}//end namespace


#endif
