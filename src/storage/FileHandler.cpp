#include <sserialize/storage/FileHandler.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/utility/constants.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <chrono>


namespace sserialize {
	
bool FileHandler::fileExists(std::string const & fileName) {
	struct ::stat64 stFileInfo;
	return (::stat64(fileName.c_str(), &stFileInfo) == 0 );
}

OffsetType FileHandler::fileSize(std::string const & fileName) {
	struct ::stat64 stFileInfo;
	if (::stat64(fileName.c_str(), &stFileInfo) == 0) {
		return stFileInfo.st_size;
	}
	return 0;
}

OffsetType FileHandler::fileSize(int fd) {
	struct ::stat64 stFileInfo;
	if (::fstat64(fd, &stFileInfo) == 0) {
		return stFileInfo.st_size;
	}
	return 0;
}

void * FileHandler::mmapFile(int fd, OffsetType fileSize, bool prePopulate, bool randomAccess) {
	int param = MAP_SHARED;
	if (prePopulate) {
		param |= MAP_POPULATE;
	}
	
	void * data = ::mmap(0, fileSize, PROT_READ | PROT_WRITE, param, fd, 0);

	if (data == MAP_FAILED) {
		return data;
	}

	if (randomAccess) {
		::madvise(data, fileSize, MADV_RANDOM);
	}

	return data;
}

void * FileHandler::mmapFile(const std::string & fileName, int & fd, OffsetType & fileSize, bool prePopulate, bool randomAccess) {
	bool fExisted = FileHandler::fileExists(fileName);
	fd = ::open(fileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		//don't create read-only file, just open it
		fd = ::open(fileName.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);
	}
	if (fd < 0) {
		return 0;
	}
	if (!fExisted) {
		if (::ftruncate(fd, 512) < 0) {//reserve at least a block on old rotating media
			::close(fd);
			::unlink(fileName.c_str());
			return 0;
		}
		fileSize = 512;
	}
	else {
		fileSize = FileHandler::fileSize(fd);
	}
		
	void * d = mmapFile(fd, fileSize, prePopulate, randomAccess);
	if (d == MAP_FAILED) {
		::close(fd);
		if (!fExisted) {
			::unlink(fileName.c_str());
		}
		return 0;
	}
	return d;
}

int FileHandler::open(const std::string& fileName) {
	int fd = ::open(fileName.c_str(), O_RDWR);
	if (fd < 0) {
		fd = ::open(fileName.c_str(), O_RDONLY);
	}
	return fd;
}

int FileHandler::createTmp(sserialize::OffsetType fileSize, std::string& tmpFileName, bool fastFile, bool directIo) {
	std::size_t fbSize;
	if (fastFile) {
		fbSize = sserialize::UByteArrayAdapter::getFastTempFilePrefix().size();
	}
	else {
		fbSize = sserialize::UByteArrayAdapter::getTempFilePrefix().size();
	}
	char fileName[fbSize+7];
	if (fastFile) {
		::memmove(fileName, sserialize::UByteArrayAdapter::getFastTempFilePrefix().c_str(), sizeof(char)*fbSize);
	}
	else {
		::memmove(fileName, sserialize::UByteArrayAdapter::getTempFilePrefix().c_str(), sizeof(char)*fbSize);
	}
	::memset(fileName+fbSize, 'X', 6);
	fileName[fbSize+6] = 0;
	
	int fd = -1;
	if (directIo) {
		fd = ::mkostemp(fileName, O_DIRECT);
	}
	else {
		fd = ::mkstemp(fileName);
	}
	
	if (fd < 0) {
		return -1;
	}
	if (fileSize) {
		if (::ftruncate(fd, fileSize) < 0) {
			::close(fd);
			::unlink(fileName);
			return -1;
		}
	}
	
	tmpFileName = std::string(fileName);
	return fd;
}


void * FileHandler::createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess, bool fastFile) {
	fd = createTmp(fileSize, tmpFileName, fastFile);
	
	if (fd < 0) {
		return 0;
	}
	void * d = mmapFile(fd, fileSize, prePopulate, randomAccess);
	
	if (d == MAP_FAILED) {
		::close(fd);
		::unlink(tmpFileName.c_str());
		fd = -1;
		return 0;
	}
	
	return d;
}

void * FileHandler::resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize, bool prePopulate, bool randomAccess) {
	void * prevLocation = mem;
	if (prevLocation) {
		::msync(prevLocation, oldSize, MS_ASYNC);
		if (::munmap(mem, oldSize) < 0) {
			return 0;
		}
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

bool FileHandler::closeAndUnlink(const std::string & fileName, int fd, void * mem, OffsetType size) {
	bool ok = close(fd, mem, size);
	ok = (::unlink(fileName.c_str()) >= 0) && ok;
	return ok;
}

bool FileHandler::close(int fd, void * mem, OffsetType size, bool sync) {
	if (sync) {
		::msync(mem, size, MS_ASYNC);
	}
	bool ok = (::munmap(mem, size) >= 0);
	ok = (::close(fd) >= 0) && ok;
	return ok;
}

int FileHandler::shmCreate(std::string& fileName) {
	std::stringstream ss;
	ss << "/" << m_shmPrefix;
	ss << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	ss << std::this_thread::get_id();
	fileName = ss.str();
	if (fileName.size() > 255) {
		fileName.resize(255);
	}
	return ::shm_open(fileName.c_str(), O_CREAT | O_RDWR | O_EXCL, S_IRWXU);
}

bool FileHandler::shmDestroy(const std::string& fileName, int fd, void* mem, OffsetType size) {
	::munmap(mem, size);
	::close(fd);
	::shm_unlink(fileName.c_str());
	return true;
}

void FileHandler::pwrite(int fd, const void * src, OffsetType size, OffsetType offset) {
	while (size) {
		::ssize_t writtenSize = ::pwrite64(fd, src, size, offset);
		if (UNLIKELY_BRANCH(writtenSize < 0)) {
			int e = errno;
			if (UNLIKELY_BRANCH(e != EINTR)) { //not interrupted by signal, real error
				throw IOException(std::string(::strerror(e)));
			}
			//else interrupted by signal, nothing was written
		}
		else if (UNLIKELY_BRANCH((OffsetType)writtenSize != size)) { //only some parts were written, try to write the rest
			size -= (OffsetType)writtenSize;
			offset += (OffsetType)writtenSize;
			src = static_cast<const char*>(src)+writtenSize;
		}
		else {
			return;
		}
	}
}

void FileHandler::pread(int fd, void * dest, OffsetType size, OffsetType offset) {
	while (size) {
		::ssize_t readSize = ::pread64(fd, dest, size, offset);
		if (UNLIKELY_BRANCH(readSize < 0)) {
			int e = errno;
			if (UNLIKELY_BRANCH(e != EINTR)) { //not interrupted by signal, real error
				throw IOException(std::string(::strerror(e)));
			}
			//else interrupted by signal, nothing was written
		}
		else if (UNLIKELY_BRANCH((OffsetType)readSize != size)) { //only some parts were written, try to write the rest
			size -= (OffsetType)readSize;
			offset += (OffsetType)readSize;
			dest = static_cast<char *>(dest)+readSize;
		}
		else {
			return;
		}
	}
}


void FileHandler::setShmPrefix(const std::string& name) {
	m_shmPrefix = name;
}

const std::string& FileHandler::getShmPrefix() {
	return m_shmPrefix;
}

std::string FileHandler::m_shmPrefix = SSERIALIZE_SHM_FILE_PREFIX;

}//end namespace sserialize
