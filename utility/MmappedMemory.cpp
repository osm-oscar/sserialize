#include <sserialize/utility/MmappedMemory.h>

namespace sserialize {

void * FileHandler::createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess) {
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

void * FileHandler::resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize, bool prePopulate, bool randomAccess) {
	void * prevLocation = mem;
	if (prevLocation) {
		::msync(prevLocation, oldSize, MS_SYNC);
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
	bool ok = (::munmap(mem, size) < 0);
	ok = (::close(fd) < 0) && ok;
	ok = (::unlink(fileName.c_str()) < 0) && ok;
	return ok;
}

}