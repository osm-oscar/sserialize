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
	static inline void * createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate) {
		std::size_t fbSize = sserialize::UByteArrayAdapter::getTempFilePrefix().size();
		char * fileName = new char[fbSize+7];
		::memmove(fileName, sserialize::UByteArrayAdapter::getTempFilePrefix().c_str(), sizeof(char)*fbSize);
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
		
		tmpFileName = std::string(fileName);
		
		return data;
	}
	
	static inline void * resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize) {
		if (::munmap(mem, oldSize) < 0) {
			return 0;
		}
		if (::ftruncate(fd, newSize) < 0) {
			return 0;
		}
		void * data = ::mmap(0, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		if (data == MAP_FAILED) {
			return 0;
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
class MmappedMemoryPrivate {
private:
	TValue * m_data;
	OffsetType m_size;
	std::string m_fileName;
	int m_fd;
public:
	MmappedMemoryPrivate(OffsetType size, bool populate = false) : m_data(0), m_size(0), m_fd(-1) {
		if (size) {
			m_data = (TValue *) FileHandler::createAndMmappTemp(size*sizeof(TValue), m_fd, m_fileName, populate);
			if (m_data) {
				m_size = size;
			}
			else {
				throw sserialize::CreationException("MmappedMemory::MmappedMemory");
			}
		}
	}
	virtual ~MmappedMemoryPrivate() {
		if (m_data) {
			FileHandler::closeAndUnlink(m_fileName, m_fd, m_data, m_size*sizeof(TValue));
		}
	}
	TValue * data() { return m_data; }
	TValue * resize(OffsetType newSize) {
		m_data = (TValue *) FileHandler::resize(m_fd, m_data, m_size*sizeof(TValue), newSize*sizeof(TValue));
		if (!m_data)
			throw sserialize::CreationException("MmappedMemory::resize");
		m_size = newSize;
		return m_data;
	}
	OffsetType size() const { return m_size; }
};

template<typename TValue>
class MmappedMemory {
private:
	std::shared_ptr< MmappedMemoryPrivate<TValue> > m_priv;
public:
	///This will not create memory region => data() equals nullptr and resize() will not work
	MmappedMemory() : m_priv(new MmappedMemoryPrivate<TValue>(0)) {}
	///@param size Number of elements
	MmappedMemory(OffsetType size) : m_priv(new MmappedMemoryPrivate<TValue>(size)) {}
	virtual ~MmappedMemory() {}
	TValue * data() { return m_priv->data(); }
	TValue * resize(OffsetType newSize) { return m_priv->resize(newSize); }
	OffsetType size() const { return m_priv->size(); }
	TValue * begin() { return data(); }
	TValue * end() { return data()+size();}
};

}//end namespace


#endif