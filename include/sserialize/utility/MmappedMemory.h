#ifndef SSERIALIZE_MMAPPED_MEMORY_H
#define SSERIALIZE_MMAPPED_MEMORY_H
#include <type_traits>
#include <functional>
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/types.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/FileHandler.h>

#ifdef __ANDROID__
#include <sserialize/utility/log.h>
#endif

namespace sserialize {

typedef enum {MM_INVALID=0, MM_PROGRAM_MEMORY, MM_SHARED_MEMORY, MM_FAST_FILEBASED, MM_SLOW_FILEBASED, MM_FILEBASED = MM_FAST_FILEBASED} MmappedMemoryType;

namespace detail {
namespace MmappedMemory {

template<typename TValue, typename TDummy=TValue>
struct MmappedMemoryHelper {
	static void initMemory(TValue * begin, TValue * end);
	static void initMemory(const TValue * srcBegin, const TValue * srcEnd, TValue * dest);
	static void deinitMemory(TValue * begin, TValue * end);
};

template<typename TValue>
struct MmappedMemoryHelper< typename std::enable_if<std::is_pod<TValue>::value, TValue>::type, TValue > {
	static void initMemory(TValue * /*begin*/, TValue * /*end*/) {}
	static void initMemory(const TValue * srcBegin, const TValue * srcEnd, TValue * dest) {
		memmove(dest, srcBegin, (srcEnd-srcBegin)*sizeof(TValue));
	}
	static void deinitMemory(TValue * /*begin*/, TValue * /*end*/) {}
};

template<typename TValue>
struct MmappedMemoryHelper< typename std::enable_if<! std::is_pod<TValue>::value, TValue>::type, TValue > {
	static void initMemory(TValue * begin, TValue * end) {
		for(; begin != end; ++begin) {
			new (begin) TValue();
		}
	}
	static void initMemory(const TValue * srcBegin, const TValue * srcEnd, TValue * dest) {
		for(; srcBegin != srcEnd; ++srcBegin, ++dest) {
			new (dest) TValue(*srcBegin);
		}
	}
	static void deinitMemory(TValue * begin, TValue * end) {
		for(; begin != end; ++begin) {
			begin->~TValue();
		}
	}
};

template<typename TValue>
class MmappedMemoryInterface: public RefCountObject{
public:
	MmappedMemoryInterface() {}
	virtual ~MmappedMemoryInterface() {}
	virtual TValue * data() = 0;
	virtual TValue * resize(OffsetType newSize) = 0;
	virtual OffsetType size() const = 0;
	virtual MmappedMemoryType type() const = 0;
};

template<typename TValue>
class MmappedMemoryEmpty: public MmappedMemoryInterface<TValue> {
public:
	MmappedMemoryEmpty() : MmappedMemoryInterface<TValue>() {}
	virtual ~MmappedMemoryEmpty() override {}
	virtual TValue * data() override { return 0; }
	virtual TValue * resize(OffsetType /*newSize*/) override { return 0; }
	virtual OffsetType size() const override { return 0; }
	virtual MmappedMemoryType type() const override { return MM_INVALID; }
};

template<typename TValue>
class MmappedMemoryFileBased: public MmappedMemoryInterface<TValue> {
private:
	TValue * m_data;
	OffsetType m_size;
	std::string m_fileName;
	int m_fd;
	bool m_populate;
	bool m_randomAccess;
	bool m_unlink;
public:
	///@para size: has to be larger than 1, otherwise will be set to 1
	MmappedMemoryFileBased(OffsetType size, bool fastFile, bool populate = false, bool randomAccess = false) :
	m_data(0),
	m_size(0),
	m_fd(-1),
	m_populate(populate),
	m_randomAccess(randomAccess),
	m_unlink(true)
	{
		size = std::max<OffsetType>(1, size);
		m_data = (TValue *) FileHandler::createAndMmappTemp(size*sizeof(TValue), m_fd, m_fileName, populate, randomAccess, fastFile);
		if (m_data) {
			m_size = size;
			MmappedMemoryHelper<TValue>::initMemory(m_data, m_data+size);
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	///map as much of fileName into memory as possible, if file does not exists, create it
	MmappedMemoryFileBased(const std::string & fileName, bool populate = false, bool randomAccess = false) :
	m_data(0),
	m_size(0),
	m_fd(-1),
	m_populate(populate),
	m_randomAccess(randomAccess),
	m_unlink(false)
	{
		OffsetType size = 0;
		m_data = (TValue *) FileHandler::mmapFile(fileName, m_fd, size, populate, randomAccess);
		if (m_data) {
			m_size = size/sizeof(TValue);
			if (m_size % sizeof(TValue)) {//resize the file so that a full TValue fits in
				resize(m_size+1);
			}
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	virtual ~MmappedMemoryFileBased() override {
		if (m_data) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data, m_data+m_size);
			if (m_unlink) {
				FileHandler::closeAndUnlink(m_fileName, m_fd, m_data, m_size*sizeof(TValue));
			}
			else {
				FileHandler::close(m_fd, m_data, m_size*sizeof(TValue));
			}
			m_data = 0;
		}
	}
	virtual TValue * data() override { return m_data; }
	virtual TValue * resize(OffsetType newSize) override {
		newSize = std::max<OffsetType>(1, newSize);
		if (newSize < m_size) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data+newSize, m_data+m_size);
		}
		m_data = (TValue *) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue), m_populate, m_randomAccess);
		if (!m_data) {
			throw sserialize::CreationException("MmappedMemory::resize");
		}
		if (newSize > m_size) {
			MmappedMemoryHelper<TValue>::initMemory(m_data+m_size, m_data+newSize);
		}
		m_size = newSize;
		return m_data;
	}
	virtual OffsetType size() const override { return m_size; }
	virtual MmappedMemoryType type() const override { return sserialize::MM_FILEBASED;}
};

template<typename TValue>
class MmappedMemoryInMemory: public MmappedMemoryInterface<TValue> {
private:
	TValue * m_data;
	OffsetType m_size;
public:
	MmappedMemoryInMemory(OffsetType size) : m_data(0), m_size(0) {
		if (size) {
			m_data = (TValue*) malloc(sizeof(TValue)*size);
			if (!m_data) {
				throw sserialize::CreationException("MmappedMemory::MmappedMemory");
			}
			else {
				MmappedMemoryHelper<TValue>::initMemory(m_data, m_data+size);
				m_size = size;
			}
		}
	}
	virtual ~MmappedMemoryInMemory() override {
		if (m_data) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data, m_data+m_size);
			free(m_data);
			m_data = 0;
			m_size = 0;
		}
	}
	virtual TValue * data() override { return m_data; }
	virtual TValue * resize(OffsetType newSize) override {
		if (newSize < m_size) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data+newSize, m_data+m_size);
		}
		if (!newSize) {
			free(m_data);
			m_data = 0;
			return 0;
		}
		TValue * newD = (TValue*) realloc(m_data, sizeof(TValue)*newSize);
		if (!newD) {
			newD = (TValue*) malloc(sizeof(TValue)*newSize);
			if (newD) {
				OffsetType tmp = std::min(newSize, m_size);
				MmappedMemoryHelper<TValue>::initMemory(m_data, m_data+tmp, newD);//copy memory
				MmappedMemoryHelper<TValue>::initMemory(newD+tmp, newD+newSize);//possibly init remaining memory if newSize > m_size
				MmappedMemoryHelper<TValue>::deinitMemory(m_data, m_data+m_size);//deinit old memory
				free(m_data);
			}
			else {
				throw sserialize::CreationException("MmappedMemory::resize");
			}
		}
		else if (newSize > m_size) { //reallocation happened => m_data==newD, init the new allocated data
			MmappedMemoryHelper<TValue>::initMemory(m_data+m_size, m_data+newSize);
		}
		m_size = newSize;
		m_data = newD;
		return m_data;
	}
	virtual OffsetType size() const override { return m_size; }
	virtual MmappedMemoryType type() const override { return sserialize::MM_PROGRAM_MEMORY;}
};

#ifndef __ANDROID__
///Exclusive shared memory based storage backend, currently unspported on android
//TODO:port this to ashm
template<typename TValue>
class MmappedMemorySharedMemory: public MmappedMemoryInterface<TValue> {
private:
	TValue * m_data;
	OffsetType m_size;
	int m_fd;
	std::string m_name;
private:
	std::string generateName(OffsetType size) const {
		std::stringstream ss;
		ss << "/sserialize" << size << rand();
		std::string ret(ss.str());
		if (ret.size() > 255)
			ret.resize(255);
		return ret;
	}
public:
	MmappedMemorySharedMemory(OffsetType size) : m_data(0), m_size(0) {
		m_name = generateName(size);
		m_fd = ::shm_open(m_name.c_str(), O_CREAT | O_RDWR | O_EXCL, S_IRWXU);
		if (m_fd < 0) {
			throw sserialize::CreationException("sserialize::MmappedMemorySharedMemory unable to create shm region");
		}
		
		m_data = (TValue*) FileHandler::resize(m_fd, 0, 0, size*sizeof(TValue), false, true);
		
		if (m_data || size == 0) {
			MmappedMemoryHelper<TValue>::initMemory(m_data, m_data+size);
			m_size = size;
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	virtual ~MmappedMemorySharedMemory() override {
		if (m_data) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data, m_data+m_size);
			assert(::munmap(m_data, m_size) == 0);
			assert(::shm_unlink(m_name.c_str()) == 0);
			m_data = 0;
		}
	}
	virtual TValue * data() override { return m_data; }
	virtual TValue * resize(OffsetType newSize) override {
		if (newSize < m_size) {
			MmappedMemoryHelper<TValue>::deinitMemory(m_data+newSize, m_data+m_size);
		}
		m_data = (TValue*) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue), false, true);
		
		if (m_data || newSize == 0) {
			if (newSize > m_size) {
				MmappedMemoryHelper<TValue>::initMemory(m_data+m_size, m_data+newSize);
			}
			m_size = newSize;
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
		return m_data;
	}
	virtual OffsetType size() const override { return m_size; }
	virtual MmappedMemoryType type() const override { return sserialize::MM_SHARED_MEMORY;}
};
#endif

}}

/** This class provides a temporary memory extension with the possiblity to grow the storage
	It does not copy the data, this is just an adaptor to a piece of memory (kind of an enhanced array pointer)
	Use the MMVector if you need funtionality equivalent to std::vector
  */
template<typename TValue>
class MmappedMemory {
public:
private:
	typedef typename detail::MmappedMemory::MmappedMemoryInterface<TValue>  MyInterface;
	sserialize::RCPtrWrapper< MyInterface > m_priv;
public:
	///This will not create memory region => data() equals nullptr and resize() will not work
	MmappedMemory() : m_priv(new detail::MmappedMemory::MmappedMemoryEmpty<TValue>()) {}
	///@param size Number of elements
	MmappedMemory(OffsetType size, MmappedMemoryType t) : m_priv(0) {
		switch (t) {
		case MM_FAST_FILEBASED:
			m_priv.reset(new detail::MmappedMemory::MmappedMemoryFileBased<TValue>(size, true));
			break;
		case MM_SLOW_FILEBASED:
			m_priv.reset(new detail::MmappedMemory::MmappedMemoryFileBased<TValue>(size, false));
			break;
		case MM_SHARED_MEMORY:
#ifndef __ANDROID__
			m_priv.reset(new detail::MmappedMemory::MmappedMemorySharedMemory<TValue>(size));
			break;
#else
			sserialize::info("sserialize::MmappedMemory", "Using MM_PROGRAM_MEMORY instead of MM_SHARED_MEMORY on android");
#endif
		case MM_PROGRAM_MEMORY:
		default:
			m_priv.reset(new detail::MmappedMemory::MmappedMemoryInMemory<TValue>(size));
			break;
		}
	}
	///use the given file name as base, create if it does not exist or reuse it
	MmappedMemory(const std::string & fileName) : m_priv(new detail::MmappedMemory::MmappedMemoryFileBased<TValue>(fileName)) {}
	virtual ~MmappedMemory() {}
	MmappedMemoryType type() const { return m_priv->type(); }
	TValue * data() { return m_priv->data(); }
	///Invalidates pointers when data() before != data() afterwards
	TValue * resize(OffsetType newSize) { return m_priv->resize(newSize); }
	OffsetType size() const { return m_priv->size(); }
	TValue * begin() { return data(); }
	TValue * end() { return data()+size();}
};

}//end namespace


#endif