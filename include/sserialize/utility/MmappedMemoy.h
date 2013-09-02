#ifndef SSERIALIZE_MMAPPED_MEMORY_H
#define SSERIALIZE_MMAPPED_MEMORY_H
#include <sserialize/utility/types.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/exceptions.h>
#include <memory>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

namespace sserialize {

/** This class provides a temporary memory extension with the possiblity to grow the storage */


struct FileHandler {
	static inline void * createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess) {
		std::size_t fbSize = sserialize::UByteArrayAdapter::getFastTempFilePrefix().size();
		char * fileName = new char[fbSize+7];
		::memmove(fileName, sserialize::UByteArrayAdapter::getFastTempFilePrefix().c_str(), sizeof(char)*fbSize);
		::memset(fileName+fbSize, 'X', 6);
		fileName[fbSize+6] = 0;
		
		fd = mkstemp(fileName);
		
		if (fd < 0)
			return 0;

		if (::ftruncate(fd, fileSize) < 0) {
			::close(fd);
			::unlink(fileName);
			fd = -1;
			return 0;
		}
		
		int param = MAP_SHARED;
		if (prePopulate) {
			param |= MAP_POPULATE;
		}
		
		void * data = ::mmap(0, fileSize, PROT_READ | PROT_WRITE, param, fd, 0);

		if (data == MAP_FAILED) {
			::close(fd);
			::unlink(fileName);
			fd = -1;
			return 0;
		}

		if (randomAccess) {
			::madvise(data, fileSize, MADV_RANDOM);
		}

		
		tmpFileName = std::string(fileName);
		
		return data;
	}
	
	static inline void * resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize, bool prePopulate, bool randomAccess) {
		void * prevLocation = mem;
		if (::munmap(mem, oldSize) < 0) {
			return 0;
		}
		if (::ftruncate(fd, newSize) < 0) {
			return 0;
		}
		
		int param = MAP_SHARED;
		if (prePopulate) {
			param |= MAP_POPULATE;
		}

		void * data = ::mmap(prevLocation, newSize, PROT_READ | PROT_WRITE, param, fd, 0);

		if (data == MAP_FAILED) {
			return 0;
		}
		if (randomAccess) {
			::madvise(data, newSize, MADV_RANDOM);
		}
		return data;
	}
	
	static inline bool closeAndUnlink(const std::string & fileName, int fd, void * mem, OffsetType size) {
		bool ok = (::munmap(mem, size) < 0);
		ok = (::close(fd) < 0) && ok;
		ok = (::unlink(fileName.c_str()) < 0) && ok;
		return ok;
	}
};

template<typename TValue>
class MmappedMemoryInterface {
public:
	MmappedMemoryInterface() {}
	virtual ~MmappedMemoryInterface() {}
	virtual TValue * data() = 0;
	virtual TValue * resize(OffsetType newSize) = 0;
	virtual OffsetType size() const = 0;
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
	MmappedMemoryFileBased(OffsetType size, bool populate = false, bool randomAccess = true) : m_data(0), m_size(0), m_fd(-1), m_populate(populate), m_randomAccess(randomAccess) {
		if (size) {
			m_data = (TValue *) FileHandler::createAndMmappTemp(size*sizeof(TValue), m_fd, m_fileName, populate, randomAccess);
			if (m_data) {
				m_size = size;
			}
			else {
				throw sserialize::CreationException("MmappedMemory::MmappedMemory");
			}
		}
	}
	virtual ~MmappedMemoryFileBased() {
		if (m_data) {
			FileHandler::closeAndUnlink(m_fileName, m_fd, m_data, m_size*sizeof(TValue));
		}
	}
	virtual TValue * data() { return m_data; }
	virtual TValue * resize(OffsetType newSize) {
		m_data = (TValue *) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue), m_populate, m_randomAccess);
		if (!m_data)
			throw sserialize::CreationException("MmappedMemory::resize");
		m_size = newSize;
		return m_data;
	}
	virtual OffsetType size() const { return m_size; }
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
			if (m_data) {
				m_size = size;
			}
			else {
				throw sserialize::CreationException("MmappedMemory::MmappedMemory");
			}
		}
	}
	virtual ~MmappedMemoryInMemory() {
		if (m_data)
			free(m_data);
	}
	virtual TValue * data() { return m_data; }
	virtual TValue * resize(OffsetType newSize) {
		m_data = (TValue*) realloc(m_data, sizeof(TValue)*newSize);
		if (!m_data)
			throw sserialize::CreationException("MmappedMemory::resize");
		m_size = newSize;
		return m_data;
	}
	virtual OffsetType size() const { return m_size; }
};

template<typename TValue>
class MmappedMemory {
private:
	typedef MmappedMemoryInterface<TValue>  MyInterface;
	std::shared_ptr< MmappedMemoryInterface<TValue> > m_priv;
public:
	///This will not create memory region => data() equals nullptr and resize() will not work
	MmappedMemory() : m_priv(new MmappedMemoryInMemory<TValue>(0)) {}
	///@param size Number of elements @param inMemory false => file-backed, true => memory only
	MmappedMemory(OffsetType size, bool inMemory) :
		m_priv((inMemory ? static_cast<MyInterface*>(new MmappedMemoryInMemory<TValue>(size)) : static_cast<MyInterface*>(new MmappedMemoryFileBased<TValue>(size)))) {}
	virtual ~MmappedMemory() {}
	TValue * data() { return m_priv->data(); }
	///Invalidates pointers when data() before != data() afterwards
	TValue * resize(OffsetType newSize) { return m_priv->resize(newSize); }
	OffsetType size() const { return m_priv->size(); }
	TValue * begin() { return data(); }
	TValue * end() { return data()+size();}
};

}//end namespace


#endif