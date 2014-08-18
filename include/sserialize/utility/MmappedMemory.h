#ifndef SSERIALIZE_MMAPPED_MEMORY_H
#define SSERIALIZE_MMAPPED_MEMORY_H
#include <sserialize/utility/types.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/exceptions.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace sserialize {

typedef enum {MM_INVALID=0, MM_FILEBASED, MM_PROGRAM_MEMORY, MM_SHARED_MEMORY} MmappedMemoryType;

struct FileHandler {
	static void * createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess);
	static void * resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize, bool prePopulate, bool randomAccess);
	static bool closeAndUnlink(const std::string & fileName, int fd, void * mem, OffsetType size);
};

namespace detail {
namespace MmappedMemory {

template<typename TValue>
class MmappedMemoryInterface: public RefCountObject {
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
	MmappedMemoryEmpty() {}
	virtual ~MmappedMemoryEmpty() {}
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
public:
	///@para size: has to be larger than 1, otherwise will be set to 1
	MmappedMemoryFileBased(OffsetType size, bool populate = false, bool randomAccess = true) :
	m_data(0),
	m_size(0),
	m_fd(-1),
	m_populate(populate),
	m_randomAccess(randomAccess)
	{
		size = std::max<OffsetType>(1, size);
		m_data = (TValue *) FileHandler::createAndMmappTemp(size*sizeof(TValue), m_fd, m_fileName, populate, randomAccess);
		if (m_data) {
			m_size = size;
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	virtual ~MmappedMemoryFileBased() {
		if (m_data) {
			FileHandler::closeAndUnlink(m_fileName, m_fd, m_data, m_size*sizeof(TValue));
			m_data = 0;
		}
	}
	virtual TValue * data() { return m_data; }
	virtual TValue * resize(OffsetType newSize) {
		newSize = std::max<OffsetType>(1, newSize);
		m_data = (TValue *) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue), m_populate, m_randomAccess);
		if (!m_data)
			throw sserialize::CreationException("MmappedMemory::resize");
		m_size = newSize;
		return m_data;
	}
	virtual OffsetType size() const { return m_size; }
	virtual MmappedMemoryType type() const { return sserialize::MM_FILEBASED;}
};

template<typename TValue>
class MmappedMemoryInMemory: public MmappedMemoryInterface<TValue> {
private:
	TValue * m_data;
	OffsetType m_size;
public:
	MmappedMemoryInMemory(OffsetType size) : m_data(0), m_size(0) {
		if (m_size) {
			m_data = (TValue*) malloc(sizeof(TValue)*size);
		}
		if (m_data) {
			m_size = size;
		}
		else if (m_size) {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	virtual ~MmappedMemoryInMemory() {
		if (m_data) {
			free(m_data);
			m_data = 0;
		}
	}
	virtual TValue * data() { return m_data; }
	virtual TValue * resize(OffsetType newSize) {
		if (!newSize) {
			free(m_data);
			m_data = 0;
			return 0;
		}
		TValue * newD = (TValue*) realloc(m_data, sizeof(TValue)*newSize);
		if (!newD) {
			newD = (TValue*) malloc(sizeof(TValue)*newSize);
			if (newD) {
				memmove(newD, m_data, sizeof(TValue)*m_size);
				free(m_data);
			}
			else {
				throw sserialize::CreationException("MmappedMemory::resize");
			}
		}
		m_size = newSize;
		m_data = newD;
		return m_data;
	}
	virtual OffsetType size() const { return m_size; }
	virtual MmappedMemoryType type() const { return sserialize::MM_PROGRAM_MEMORY;}
};

///Exclusive shared memory based storage backend
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
			m_size = size;
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
	}
	virtual ~MmappedMemorySharedMemory() {
		if (m_data) {
			::munmap(m_data, m_size);
			::shm_unlink(m_name.c_str());
			m_data = 0;
		}
	}
	virtual TValue * data() { return m_data; }
	virtual TValue * resize(OffsetType newSize) {
		m_data = (TValue*) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue), false, true);
		
		if (m_data || newSize == 0) {
			m_size = newSize;
		}
		else {
			throw sserialize::CreationException("MmappedMemory::MmappedMemory");
		}
		return m_data;
	}
	virtual OffsetType size() const { return m_size; }
	virtual MmappedMemoryType type() const { return sserialize::MM_SHARED_MEMORY;}
};

}}

/** This class provides a temporary memory extension with the possiblity to grow the storage
	It does not copy the data, this is just an adaptor a piece of memory (kind of an enhanced array pointer)
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
		case MM_FILEBASED:
			m_priv.reset(new detail::MmappedMemory::MmappedMemoryFileBased<TValue>(size));
			break;
		case MM_SHARED_MEMORY:
			m_priv.reset(new detail::MmappedMemory::MmappedMemorySharedMemory<TValue>(size));
			break;
		case MM_PROGRAM_MEMORY:
		default:
			m_priv.reset(new detail::MmappedMemory::MmappedMemoryInMemory<TValue>(size));
			break;
		}
	}
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