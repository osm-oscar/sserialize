#include <sserialize/storage/FileHandler.h>
#include <sserialize/storage/MmappedFile.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>


namespace sserialize {

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
	bool fExisted = MmappedFile::fileExists(fileName);
	fd = ::open(fileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return 0;
	if (!fExisted) {
		if (::ftruncate(fd, 512) < 0) {//reserve at least a block on old rotating media
			::close(fd);
			::unlink(fileName.c_str());
			return 0;
		}
		fileSize = 512;
	}
	else {
		fileSize = MmappedFile::fileSize(fileName);
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

void * FileHandler::createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess, bool fastFile) {
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
	
	fd = ::mkstemp(fileName);
	
	if (fd < 0)
		return 0;
	if (::ftruncate(fd, fileSize) < 0) {
		::close(fd);
		::unlink(fileName);
		fd = -1;
		return 0;
	}
	
	void * d = mmapFile(fd, fileSize, prePopulate, randomAccess);
	
	if (d == MAP_FAILED) {
		::close(fd);
		::unlink(fileName);
		fd = -1;
		return 0;
	}
	
	tmpFileName = std::string(fileName);
	
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
	ok = (::unlink(fileName.c_str()) < 0) && ok;
	return ok;
}

bool FileHandler::close(int fd, void * mem, OffsetType size, bool sync) {
	if (sync) {
		::msync(mem, size, MS_ASYNC);
	}
	bool ok = (::munmap(mem, size) < 0);
	ok = (::close(fd) < 0) && ok;
	return ok;
}

int FileHandler::shmCreate(std::string& fileName) {
	std::stringstream ss;
	ss << "/sserialize" << rand() << rand();
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


}