#ifndef SSERIALIZE_UBYTE_ARRAY_ADAPTER_H
#define SSERIALIZE_UBYTE_ARRAY_ADAPTER_H

#if defined(WITH_SSERIALIZE_CONTIGUOUS_UBA_ONLY) || defined(WITH_SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL)
	#define SSERIALIZE_UBA_ONLY_CONTIGUOUS
	#ifdef WITH_SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL
		#define SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL
	#else
		#undef SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL
	#endif
	#undef SSERIALIZE_UBA_NON_CONTIGUOUS
#else
	#undef SSERIALIZE_UBA_ONLY_CONTIGUOUS
	#undef SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL
	#define SSERIALIZE_UBA_NON_CONTIGUOUS
#endif
#if defined(WITH_SSERIALIZE_UBA_OPTIONAL_REFCOUNTING)
	#define SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
#else
	#undef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
#endif

#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
	#define SSERIALIZE_NAMESPACE_INLINE_UBA_ONLY_CONTIGUOUS inline
	#define SSERIALIZE_NAMESPACE_INLINE_UBA_NON_CONTIGUOUS
#else
	#define SSERIALIZE_NAMESPACE_INLINE_UBA_ONLY_CONTIGUOUS
	#define SSERIALIZE_NAMESPACE_INLINE_UBA_NON_CONTIGUOUS inline
#endif

#include <sserialize/utility/types.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/storage/SerializationInfo.h>
#include <stdint.h>
#include <iterator>
#include <deque>
#include <vector>
#include <string>

//Split this into array slice, streaming, and iterator, there may be multiple iterators (like uint8, uint16., uint32 iterators, any constant sized iterator
//The underlying UByteArrayAdapterPrivate does not need to be changed
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
  * (u)int64: 1-10
  * 
  * All functions that change the size of the underlying storage are NOT thread-safe.
  * All functions that return references may NOT be thread-safe (depending on the backend)
  * 
  */

namespace sserialize {

class MmappedFile;
// class UByteArrayAdapterPrivate;
class ChunkedMmappedFile;
class CompressedMmappedFile;
class UByteArrayAdapter;

SSERIALIZE_NAMESPACE_INLINE_UBA_ONLY_CONTIGUOUS
namespace UByteArrayAdapterOnlyContiguous {
	class UByteArrayAdapterPrivate;
}

SSERIALIZE_NAMESPACE_INLINE_UBA_NON_CONTIGUOUS
namespace UByteArrayAdapterNonContiguous {
	class UByteArrayAdapterPrivate;
}

#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
using UByteArrayAdapterOnlyContiguous::UByteArrayAdapterPrivate;
#else
using UByteArrayAdapterNonContiguous::UByteArrayAdapterPrivate;
#endif


class UByteArrayAdapterPrivateArray;

namespace detail {
namespace __UByteArrayAdapter {

	class MemoryView final {
		friend class sserialize::UByteArrayAdapter;
	public:
		#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
		typedef sserialize::UByteArrayAdapterPrivateArray MyPrivate;
		#else
		typedef sserialize::UByteArrayAdapterPrivate MyPrivate;
		#endif
		#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
		typedef RCPtrWrapper<MyPrivate, true> MyPrivatePtr;
		#else
		typedef RCPtrWrapper<MyPrivate, false> MyPrivatePtr;
		#endif
	public:
		typedef const uint8_t * const_iterator;
		typedef uint8_t * iterator;
	private:
		class MemoryViewImp final: public sserialize::RefCountObject {
			MyPrivatePtr m_dataBase;
			uint8_t * m_d;
			OffsetType m_off;
			OffsetType m_size;
			bool m_copy;
		public:
			MemoryViewImp(uint8_t * ptr, OffsetType off, OffsetType size, bool isCopy, MyPrivate * base);
			~MemoryViewImp();
			UByteArrayAdapter dataBase() const;
			inline uint8_t * get() { return m_d; }
			inline const uint8_t * get() const { return m_d; }
			inline OffsetType size() const { return m_size; }
			inline bool isCopy() const { return m_copy; }
			bool flush(OffsetType len, OffsetType off);
		};
	private:
		sserialize::RCPtrWrapper<MemoryViewImp> m_priv;
		///@param isCopy: if true, then ptr gets deleted by delete[]
		MemoryView(uint8_t * ptr, OffsetType off, OffsetType size, bool isCopy, MyPrivate * base) : m_priv(new MemoryViewImp(ptr, off, size, isCopy, base)) {}
	public:
		MemoryView() {}
		~MemoryView() {}
		inline const uint8_t & operator[](SizeType i) const { return *(data()+i); }
		inline uint8_t & operator[](SizeType i) { return *(data()+i); }
		inline uint8_t * data() { return m_priv->get();}
		inline const uint8_t * data() const { return m_priv->get();}
		inline uint8_t * get() { return m_priv->get();}
		inline const uint8_t * get() const { return m_priv->get();}
		inline uint8_t * begin() { return get(); }
		inline const uint8_t * cbegin() const { return get(); }
		inline const uint8_t * begin() const { return get(); }
		inline uint8_t * end() { return get()+size(); }
		inline const uint8_t * cend() const { return get()+size(); }
		inline const uint8_t * end() const { return get()+size(); }

		inline OffsetType size() const { return m_priv->size();}
		///If this is true, then writes are not passed through to the UBA, call flush() to do that
		inline bool isCopy() const { return m_priv->isCopy();}
		///flush up to len bytes starting from off
		inline bool flush(OffsetType len, OffsetType off = 0) { return m_priv->flush(len, off); }
		inline bool flush() { return flush(size()); }
		///storage this memview is based on
		UByteArrayAdapter dataBase() const;
	};
	
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
	
}}//end namespace detail::__UByteArrayAdapter


class UByteArrayAdapter: public std::iterator<std::random_access_iterator_tag, uint8_t, sserialize::SignedOffsetType> {
public:
	#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
	typedef sserialize::UByteArrayAdapterPrivateArray MyPrivate;
	#else
	typedef sserialize::UByteArrayAdapterPrivate MyPrivate;
	#endif
	#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
	typedef RCPtrWrapper<MyPrivate, true> MyPrivatePtr;
	#else
	typedef RCPtrWrapper<MyPrivate, false> MyPrivatePtr;
	#endif
public:
	typedef sserialize::OffsetType OffsetType;
	typedef sserialize::SignedOffsetType NegativeOffsetType;
	typedef sserialize::OffsetType SizeType;
	
	struct ConsumeTag {};
	struct NoConsumeTag {};

	static constexpr ConsumeTag Consume = ConsumeTag();
	static constexpr NoConsumeTag NoConsume = NoConsumeTag();
	
	typedef enum {
		//will read the next bytes
		AT_READ = 0x1,
		//will write the next bytes
		AT_WRITE = 0x2,
		//will not use the next bytes
		AT_DROP = 0x4,
		//lock the memory region if possible
		AT_LOCK = 0x8,
		//unlock the memory region
		AT_UNLOCK = 0x10,
		//load the next bytes into memory
		AT_LOAD=0x20,
	} AdviseType;
	
	typedef detail::__UByteArrayAdapter::MemoryView MemoryView;
	
	template<typename TValue>
	using SerializationSupport = detail::__UByteArrayAdapter::SerializationSupport<TValue>;
	
	template<typename TValue>
	using Deserializer = detail::__UByteArrayAdapter::Deserializer<TValue>;

	template<typename TValue>
	using StreamingSerializer = detail::__UByteArrayAdapter::StreamingSerializer<TValue>;
public: //static functions
	static UByteArrayAdapter createCache(OffsetType size = 0, sserialize::MmappedMemoryType mmt = MM_PROGRAM_MEMORY);
	static UByteArrayAdapter createFile(OffsetType size, std::string fileName);
	///if chunkSizeExponent == 0 => use ThreadSafeFile instead of ChunkedMmappedFile
	static UByteArrayAdapter open(const std::string & fileName, bool writable = true, UByteArrayAdapter::OffsetType maxFullMapSize = SSERIALIZE_MAX_SIZE_FOR_FULL_MMAP, uint8_t chunkSizeExponent = SSERIALIZE_CHUNKED_MMAP_EXPONENT);
	///if chunkSizeExponent == 0 => use ThreadSafeFile instead of ChunkedMmappedFile
	static UByteArrayAdapter openRo(const std::string & fileName, bool compressed, OffsetType maxFullMapSize = SSERIALIZE_MAX_SIZE_FOR_FULL_MMAP, uint8_t chunkSizeExponent = SSERIALIZE_CHUNKED_MMAP_EXPONENT);
	static std::string getTempFilePrefix();
	static std::string getFastTempFilePrefix();
	static std::string getLogFilePrefix();
	static void setTempFilePrefix(const std::string & path);
	static void setFastTempFilePrefix(const std::string & path);
	static void setLogFilePrefix(const std::string & path);
	
	static UByteArrayAdapter & makeContigous(UByteArrayAdapter & d);
	static UByteArrayAdapter makeContigous(const UByteArrayAdapter & d);
	
	static inline OffsetType OffsetTypeSerializedLength() { return 5; }
	enum {S_OffsetTypeSerializedLength=5};
	
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
public://constructors
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
	UByteArrayAdapter(std::vector<uint8_t> * data, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(std::vector<uint8_t> * data);
	UByteArrayAdapter(std::vector<uint8_t> * data, bool deleteOnClose);
	UByteArrayAdapter(const sserialize::MmappedFile& file, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(const sserialize::MmappedFile& file);
	UByteArrayAdapter(const sserialize::MmappedMemory<uint8_t> & mem);
	UByteArrayAdapter(const MemoryView & mem);
	UByteArrayAdapter(OffsetType size, std::string fileName);
	UByteArrayAdapter(OffsetType size, sserialize::MmappedMemoryType mmt);
	UByteArrayAdapter(sserialize::MmappedMemoryType mmt);
	
#if defined(SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL) || defined(SSERIALIZE_UBA_NON_CONTIGUOUS)
	UByteArrayAdapter(std::deque<uint8_t> * data, OffsetType offSet, OffsetType len);
	UByteArrayAdapter(std::deque<uint8_t> * data);
	UByteArrayAdapter(std::deque<uint8_t> * data, bool deleteOnClose);
	UByteArrayAdapter(const ChunkedMmappedFile & file);
	UByteArrayAdapter(const CompressedMmappedFile & file);
#endif
	~UByteArrayAdapter();
	UByteArrayAdapter & operator=(const UByteArrayAdapter & node);
#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
	///This operation is NOT thread-safe
	void disableRefCounting();
	void enableRefCounting();
#endif
	void swap(UByteArrayAdapter & other);
public:
	UByteArrayAdapter fromGetPtr() const;
	UByteArrayAdapter fromPutPtr() const;
public:
	///Tell UByteArrayAdapter about the intended usage of the next count bytes
	void advice(AdviseType type, SizeType count);
	///Tell UByteArrayAdapter about the intended usage of all of its data
	void advice(AdviseType type);
	///Sync all data to disk
	void sync();
public://templated get/put functions to specify the types via template parameters
	template<typename TValue>
	TValue get(UByteArrayAdapter::OffsetType pos) const;
	
	template<typename TValue>
	void get(UByteArrayAdapter::OffsetType pos, TValue & get) const;
	
	template<typename TValue>
	TValue get();
	
	template<typename TValue>
	void get(TValue & v);
	
	template<typename TValue>
	void put(UByteArrayAdapter::OffsetType pos, const TValue & v);
	
	template<typename TValue>
	void put(const TValue & v);
public: //iterator api
	///This is NOT thread-safe for some backends
	uint8_t & operator*();
	///This is NOT thread-safe for some backends
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
	bool isContiguous() const;

public: //comparisson
	bool equalContent(const sserialize::UByteArrayAdapter & b) const;
	bool equalContent(const std::deque<uint8_t> & b) const;
public://state info
	inline OffsetType size() const { return m_len;};
	inline OffsetType offset() const { return m_offSet;}
	inline bool isEmpty() const { return (m_len == 0);}
	inline OffsetType tellPutPtr() const { return m_putPtr; }
	inline OffsetType tellGetPtr() const { return m_getPtr; }
	bool getPtrHasNext() const;

public: //state manipulation
	void incPutPtr(OffsetType num);
	void decPutPtr(OffsetType num);
	void setPutPtr(OffsetType pos);
	void resetPutPtr();
	/** Moves the offset to the putPtr */
	UByteArrayAdapter& shrinkToPutPtr();
	void incGetPtr(OffsetType num);
	void decGetPtr(OffsetType num);
	void setGetPtr(OffsetType pos);
	UByteArrayAdapter& resetGetPtr();
	/** Moves the offset to the getPtr */
	UByteArrayAdapter& shrinkToGetPtr();
	UByteArrayAdapter & resetPtrs();
	void resetToStorage();
	
public: //storage manipulation
	void zero();
	void fill(uint8_t value, SizeType begin, SizeType length);

public://storage manipulationm
	/** tries to shrink the underlying data source, use with caution, others Adapter are not notified of this change */
	bool shrinkStorage(OffsetType byte);
	/** tries to grow the storage of this adapter by byte bytes.
	  * If the underlying storage is already larege enough, then no additional storage will be allocates */
	bool growStorage(OffsetType byte);
	///resize to @param byte bytes (grows but does not shrink the underlying storage)
	bool resize(OffsetType byte);
	///reserves @bytes bytes beginning at tellPutPtr()
	bool reserveFromPutPtr(OffsetType bytes);
	void setDeleteOnClose(bool del);
public://conversion functions
	
	UByteArrayAdapter writeToDisk(std::string fileName, bool deleteOnClose = true);

	///Returns a read/writable MemoryView. If you don't write to it, then this function behaves like a const function
	MemoryView getMemView(const OffsetType pos, OffsetType size);
	const MemoryView getMemView(const OffsetType pos, OffsetType size) const;
	inline MemoryView asMemView() { return getMemView(0, size());}
	const MemoryView asMemView() const { return getMemView(0, size());}
	
	std::string toString() const;
	
public://get functions with offset

	///The life-time of this reference is only guaranteed til the next call of ANY function
	///This is therefor not considered to be a good way of accessing the data
	///Use this only in conjuction with contiguous data AND in read-only mode
	uint8_t & operator[](const OffsetType pos);
	///The life-time of this reference is only guaranteed til the next call of ANY function
	///This is therefor not considered to be a good way of accessing the data
	///Use this only in conjuction with contiguous data AND in read-only mode
	const uint8_t & operator[](const OffsetType pos) const;
	
	uint8_t at(OffsetType pos) const;
	int64_t getInt64(const OffsetType pos) const;
	uint64_t getUint64(const OffsetType pos) const;
	int32_t getInt32(const OffsetType pos) const;
	uint32_t getUint32(const OffsetType pos) const;
	uint32_t getUint24(const OffsetType pos) const;
	uint16_t getUint16(const OffsetType pos) const;
	uint8_t getUint8(const OffsetType pos) const;
	double getDouble(const OffsetType pos) const;
	float getFloat(const OffsetType pos) const;

	uint64_t getVlPackedUint64(const OffsetType pos, int* length = 0) const;
	int64_t getVlPackedInt64(const OffsetType pos, int* length = 0) const;
	uint32_t getVlPackedUint32(const OffsetType pos, int* length = 0) const;
	int32_t getVlPackedInt32(const OffsetType pos, int* length = 0) const;
	
	//Offset storage
	OffsetType getOffset(const OffsetType pos) const;
	NegativeOffsetType getNegativeOffset(const OffsetType pos) const;
	
	//returns an empty string if length is invalid
	std::string getString(const OffsetType pos, int * length = 0) const;
	inline uint32_t getStringLength(const OffsetType pos) { return getVlPackedUint32(pos);}
	//returns an empty string if length is invalid
	UByteArrayAdapter getStringData(const OffsetType pos, int * length = 0) const;
	
	OffsetType getData(const OffsetType pos, uint8_t * dest, OffsetType len) const;
	
public://put functions with offset

	void putUint64(const OffsetType pos, const uint64_t value);
	void putInt64(const OffsetType pos, const int64_t value);
	void putInt32(const OffsetType pos, const int32_t value);
	void putUint32(const OffsetType pos, const uint32_t value);
	void putUint24(const OffsetType pos, const uint32_t value);
	void putUint16(const OffsetType pos, const uint16_t value);
	void putUint8(const OffsetType pos, const uint8_t value);
	void putDouble(const OffsetType pos, const double value);
	void putFloat(const OffsetType pos, const float value);
	
	void putOffset(const OffsetType pos, const OffsetType value);
	void putNegativeOffset(const OffsetType pos, const NegativeOffsetType value);

	/** @return: Length of the number, -1 on failure **/
	int putVlPackedUint64(const OffsetType pos, const uint64_t value);
	int putVlPackedInt64(const OffsetType pos, const int64_t value);
	
	int putVlPackedUint32(const OffsetType pos, const uint32_t value);
	int putVlPackedPad4Uint32(const OffsetType pos, const uint32_t value);
	int putVlPackedInt32(const OffsetType pos, const int32_t value);
	int putVlPackedPad4Int32(const OffsetType pos, const int32_t value);

	/** @return number of bytes added, -1 if failed */
	int putString(const OffsetType pos, const std::string & str);
	void putData(const OffsetType pos, const uint8_t* data, OffsetType len);
	void putData(const OffsetType pos, const std::deque<uint8_t> & data);
	void putData(const OffsetType pos, const std::vector<uint8_t> & data);
	void putData(const OffsetType pos, const UByteArrayAdapter & data);
	void putData(const OffsetType pos, const MemoryView & data);

public://streaming get functions

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
	OffsetType getData(uint8_t * dest, OffsetType len);

	inline uint32_t getStringLength() { return getVlPackedUint32();}
	UByteArrayAdapter getStringData();
	std::string getString();

public://streaming put functions
	
	void putOffset(const OffsetType value);
	void putNegativeOffset(const NegativeOffsetType value);
	
	void putInt64(const int64_t value);
	void putUint64(const uint64_t value);
	
	void putInt32(const int32_t value);
	void putUint32(const uint32_t value);
	void putUint24(const uint32_t value);
	void putUint16(const uint16_t value);
	void putUint8(const uint8_t value);
	void putDouble(const double value);
	void putFloat(const float value);

	int putVlPackedUint64(const uint64_t value);
	int putVlPackedInt64(const int64_t value);
	
	int putVlPackedUint32(const uint32_t value);
	int putVlPackedPad4Uint32(const uint32_t value);
	int putVlPackedInt32(const int32_t value);
	int putVlPackedPad4Int32(const int32_t value);

	void putString(const std::string & str);
	void putData(const uint8_t * data, OffsetType len);
	void putData(const std::deque<uint8_t> & data);
	void putData(const std::vector<uint8_t> & data);
	void putData(const UByteArrayAdapter & data);
public://debugging functions
	void dump(OffsetType byteCount) const;
	void dumpAsString(OffsetType byteCount) const;
/*---------------PRIVATE PART---------------------------*/
private:
	friend class detail::__UByteArrayAdapter::MemoryView;
private:
	/** Data is at offset, not at base address **/
	MyPrivatePtr m_priv;
	OffsetType m_offSet;
	OffsetType m_len;
	OffsetType m_getPtr;
	OffsetType m_putPtr;

	static std::string m_tempFilePrefix;
	static std::string m_fastTempFilePrefix;
	static std::string m_logFilePrefix;
	
private:
	explicit UByteArrayAdapter(const MyPrivatePtr & priv);
	explicit UByteArrayAdapter(const MyPrivatePtr & priv, OffsetType offSet, OffsetType len);
	///base ctor which sets all member variables, default init to 0
	explicit UByteArrayAdapter(MyPrivate * priv, OffsetType offSet = 0, OffsetType len = 0, OffsetType getPtr = 0, OffsetType putPtr = 0);
	bool resizeForPush(OffsetType pos, OffsetType length);
// 	void moveAndResize(uint32_t offset, unsigned int smallerLen);
	void range_check(OffsetType pos, OffsetType length) const;
};

//Streaming operators

/** Iterator equality comparisson, does not check if a contains the same as b */ 
bool operator==(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b);

/** Iterator equality comparisson, does not check if a contains the same as b */ 
bool operator!=(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b);

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const int64_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint64_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const int32_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint32_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint16_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const uint8_t value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const double value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const float value);
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & data, const std::string & value);
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

template<typename T>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, T & value) {
	sserialize::UByteArrayAdapter tmp(data);
	tmp.shrinkToGetPtr();
	value = T(tmp);
	data.incGetPtr( sserialize::SerializationInfo<T>::sizeInBytes(value) );
	return data;
}

//template specialiazations

#define SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(__TYPE, __GETFUNC, __PUTFUNC) \
template<> \
inline void UByteArrayAdapter::put(const __TYPE & v) { \
	__PUTFUNC(v); \
} \
template<> \
inline void UByteArrayAdapter::put(UByteArrayAdapter::OffsetType pos, const __TYPE & v) { \
	__PUTFUNC(pos, v); \
} \
template<> \
inline __TYPE UByteArrayAdapter::get() { \
	return __GETFUNC(); \
} \
template<> \
inline void UByteArrayAdapter::get(__TYPE & v) { \
	v = __GETFUNC(); \
} \
template<> \
inline __TYPE UByteArrayAdapter::get(UByteArrayAdapter::OffsetType pos) const { \
	return __GETFUNC(pos); \
} \
template<> \
inline void UByteArrayAdapter::get(UByteArrayAdapter::OffsetType pos, __TYPE & v) const { \
	v = __GETFUNC(pos); \
} \

SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(uint8_t, getUint8, putUint8);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(uint16_t, getUint16, putUint16);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(uint32_t, getUint32, putUint32);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(int32_t, getInt32, putInt32);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(uint64_t, getUint64, putUint64);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(int64_t, getInt64, putInt64);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(float, getFloat, putFloat);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(double, getDouble, putDouble);
SSERIALIZE_UBA_GET_PUT_TEMPLATE_SPECIALIZATIONS(std::string, getString, putString);

template<>
inline void UByteArrayAdapter::put(const sserialize::UByteArrayAdapter & v) {
	putData(v);
}

template<>
inline void UByteArrayAdapter::put(UByteArrayAdapter::OffsetType pos, const sserialize::UByteArrayAdapter & v) { \
	putData(pos, v);
}

namespace detail {
namespace __UByteArrayAdapter {

#define UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(__TYPE) \
template<> \
struct SerializationSupport<__TYPE> { \
	static const bool value = true; \
};

UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(uint8_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(uint16_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(uint32_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(uint64_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(int32_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(int64_t);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(float);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(double);
UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS(std::string);
#undef UBA_SERIALIZATION_SUPPORT_SPECIALICATIONS

#define UBA_DESERIALIZER_SPECIALIZATIONS(__TYPE, __FUNCNAME) \
template<> \
struct Deserializer<__TYPE> { \
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

}}//end namespace detail::__UByteArrayAdapter

template<>
struct SerializationInfo<std::string> {
	static const bool is_fixed_length = false;
	static const OffsetType length = 0;
	static const OffsetType max_length = 0;
	static const OffsetType min_length = 0;
	static OffsetType sizeInBytes(const std::string & value);
};

}//end namespace


#endif
