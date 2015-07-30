#include <sserialize/storage/MmappedFile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h> 
#include <stdlib.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <limits>
#include <iostream>

namespace sserialize {

MmappedFilePrivate::MmappedFilePrivate(std::string filename) :
RefCountObject(),
m_fileName(filename),
m_exposedSize(0),
m_realSize(0),
m_fd(-1),
m_data(0),
m_writable(false),
m_deleteOnClose(false),
m_syncOnClose(false)
{}

MmappedFilePrivate::MmappedFilePrivate() :
RefCountObject(),
m_exposedSize(0),
m_realSize(0),
m_fd(-1),
m_data(0),
m_writable(false),
m_deleteOnClose(false),
m_syncOnClose(false)
{}

MmappedFilePrivate::~MmappedFilePrivate() {
	do_close();
}

bool MmappedFilePrivate::do_open() {
	int proto = O_RDONLY;
	if (m_writable)
		proto = O_RDWR;
	if (m_fileName.size() == 0)
		return false;
	m_fd = ::open64(m_fileName.c_str(), proto);
	if (m_fd < 0)
		return false;
	struct ::stat64 stFileInfo;
	if (::fstat64(m_fd,&stFileInfo) == 0 && stFileInfo.st_size >= 0 && static_cast<OffsetType>(stFileInfo.st_size) < std::numeric_limits<OffsetType>::max()) {
		m_realSize = stFileInfo.st_size;
		m_exposedSize = m_realSize;
	}
	else {
		return false;
	}
	if (!S_ISREG (stFileInfo.st_mode))
		return false;
	
	int mmap_proto = PROT_READ;
	if (m_writable)
		mmap_proto |= PROT_WRITE;
	m_data = (uint8_t*) ::mmap64(0, m_realSize, mmap_proto, MAP_SHARED, m_fd, 0);
	
	if (m_data == MAP_FAILED) {
		::close(m_fd);
		return false;
	}
	
	//Bad for performance
// 	::madvise(m_data, m_realSize, MADV_RANDOM);//TODO:should be part of the interface, not just here
	
	return true;
}

bool MmappedFilePrivate::do_close() {
	if (m_fd < 0) {
		return true;
	}
	
	bool allOk = true;
	if (m_data) {
		if (m_writable && m_realSize != m_exposedSize) {
			resize(m_exposedSize);
		}
	
		if (m_syncOnClose) {
			allOk = do_sync() && allOk;
		}
		if (::munmap(m_data, m_realSize) < 0) {
			::perror("MmappedFilePrivate::do_close()");
			allOk = false;
		}
	}
	if (::close(m_fd) == -1) {
		::perror("MmappedFilePrivate::do_close()");
	}
	m_fd = -1;
	m_exposedSize = 0;
	m_realSize = 0;
	m_data = 0;
	
	if (m_deleteOnClose) {
		allOk = (::unlink(m_fileName.c_str()) < 0) && allOk;
	}
	
	return allOk;
}

bool MmappedFilePrivate::do_sync() {
	if ( !(m_data && m_fd > 0) )
		return false;
	int result = ::msync(m_data, m_realSize, MS_SYNC);
	return result == 0;
}

bool MmappedFilePrivate::valid() {
	return (m_data && m_fd > 0 && m_realSize > 0 && m_fileName.size() > 0);
}

bool MmappedFilePrivate::read(uint8_t * buffer, uint32_t len, OffsetType displacement) {
	if (m_data && displacement <= m_exposedSize && m_exposedSize-displacement >= len) {
		for(size_t i = displacement; i < displacement+len; i++)
			buffer[i-displacement] = m_data[i];
		return true;
	}
	return false;
}

#define ROUND_VALUE_EXP 20
//round to the next 1MB
inline OffsetType roundSize(OffsetType size) {
	return ((size >> ROUND_VALUE_EXP) << ROUND_VALUE_EXP) + (static_cast<OffsetType>(1) << ROUND_VALUE_EXP);
}

bool MmappedFilePrivate::resizeRounded(OffsetType size) {
	OffsetType newRealSize = roundSize(size);
	if (newRealSize == m_realSize) {
		m_exposedSize = size;
		return true;
	}
	bool ok = resize(newRealSize);
	if (ok) {
		m_exposedSize = size;
	}
	return ok;
}

bool MmappedFilePrivate::resize(OffsetType size) {
	if (!do_sync() ) {
		::perror("MmappedFilePrivate::resize::sync");
		return false;
	}

	//unmap
	if (::munmap(m_data, m_realSize) == -1) {
		::perror("MmappedFilePrivate::resize::munmap");
		return false;
	}

	bool allOk = true;
	int result = ::ftruncate(m_fd, size);
	if (result < 0) { 
		std::cerr << "MmappedFilePrivate::resize::truncate: failed to truncate file from " << m_realSize << " to " << size << " bytes:";
		::perror("");
		allOk = false;
	}
	else {
		m_exposedSize = size;
		m_realSize = size;
	}
	
	int mmap_proto = PROT_READ;
	if (m_writable)
		mmap_proto |= PROT_WRITE;
	m_data = (uint8_t*) ::mmap64(m_data, m_realSize, mmap_proto, MAP_SHARED, m_fd, 0);
	
	if (m_data == MAP_FAILED) {
		std::cerr << "MmappedFilePrivate::resize::mmap: failed to mmap file while resizing from " << m_realSize << " to " << size << " bytes:";
		::perror("MmappedFilePrivate::resize::mmap");
		m_data = 0;//this needs to come here (do_close() checks for it
		do_close();
		allOk = false;
	}
	
	//Bad for performance
// 	::madvise(m_data, m_realSize, MADV_RANDOM);//TODO:should be part of the interface, not just here

	return allOk;
}

void MmappedFilePrivate::setWriteableFlag(bool writable) {
	if (m_writable && !writable && m_exposedSize != m_realSize) {
		resize(m_exposedSize);
	}
	m_writable = writable;
}

void MmappedFilePrivate::setDeleteOnClose(bool deleteOnClose) {
	m_deleteOnClose = deleteOnClose;
}

void MmappedFilePrivate::setSyncOnClose(bool syncOnClose) {
	m_syncOnClose = syncOnClose;
}

MmappedFilePrivate * MmappedFilePrivate::createTempFile(const std::string & fileNameBase, sserialize::UByteArrayAdapter::OffsetType size) {
	std::size_t fbSize = fileNameBase.size();
	char fileName[fbSize+7];
	::memmove(fileName, fileNameBase.c_str(), sizeof(char)*fbSize);
	::memset(fileName+fbSize, 'X', 6);
	fileName[fbSize+6] = 0;
	
	int fd = mkstemp(fileName);
	
	if (fd < 0)
		return 0;

	if (::ftruncate(fd, size) < 0) {
		::close(fd);
		::unlink(fileName);
		return 0;
	}
	
	uint8_t * data = (uint8_t*) ::mmap64(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (data == MAP_FAILED) {
		::close(fd);
		::unlink(fileName);
		return 0;
	}
	
	//Bad for performance
// 	::madvise(data, size, MADV_RANDOM);//TODO:should be part of the interface, not just here
	
	MmappedFilePrivate * mf = new MmappedFilePrivate();
	mf->m_fileName = std::string(fileName, fbSize+6);
	mf->m_realSize = size;
	mf->m_exposedSize = size;
	mf->m_fd = fd;
	mf->m_writable = true;
	mf->m_deleteOnClose = true;
	mf->m_syncOnClose = false;
	mf->m_data = data;
	return mf;
}

bool createFilePrivate(const std::string & fileName, OffsetType size) {
	int fd = ::open64(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) {
		return false;
	}
	SignedOffsetType result = 0;
	if (size > 0) {
		result = ::lseek64(fd, size-1, SEEK_SET);
	}
	if (result == -1) {
		::close(fd);
		return false;
	}
	result = write(fd, "", 1);
	if (result != 1) {
		::close(fd);
		return false;
	}
	struct ::stat64 stFileInfo;
	::fstat64(fd, &stFileInfo);
	::close(fd);
	return  stFileInfo.st_size >= 0 && static_cast<OffsetType>( stFileInfo.st_size ) >= size;
}

bool MmappedFile::createFile(const std::string & fileName, OffsetType size) {
	return createFilePrivate(fileName, size);
}

bool MmappedFile::createCacheFile(OffsetType size, sserialize::MmappedFile & dest) {
	std::string tempFileName = MmappedFile::findLockFilePath(UByteArrayAdapter::getTempFilePrefix(), 2048);
	if (tempFileName.empty()) {
		return false;
	}
	if (! MmappedFile::createFile(tempFileName, size) ) {
		return false;
	}
	dest = MmappedFile(tempFileName, true);
	dest.setDeleteOnClose(true);
	if (! dest.open() ) {
		if (fileExists(tempFileName))
			unlinkFile(tempFileName);
		return false;
	}
	return true;
}

bool truncateFilePrivate(const std::string & fileName, OffsetType size) {
	return (::truncate(fileName.c_str(), size) > 0);
}

bool MmappedFile::truncateFile(const std::string & fileName, OffsetType size) {
	return truncateFilePrivate(fileName, size);
}

bool fileExistsPrivate(const std::string & fileName) {
	struct ::stat64 stFileInfo;
	return (::stat64(fileName.c_str(), &stFileInfo) == 0 );
}

bool MmappedFile::fileExists(const std::string & fileName) {
	return fileExistsPrivate(fileName);
}

OffsetType MmappedFile::fileSize(const std::string& fileName) {
	struct ::stat64 stFileInfo;
	if (::stat64(fileName.c_str(),&stFileInfo) == 0)
		return stFileInfo.st_size;
	else
		return 0;
}

std::string findLockFilePathPrivate(const std::string & fileNamePrefix, uint32_t maxTest) {
	std::string fn;
	char buf[32];
	for(unsigned int i=0; i < maxTest; i++) {
		int len = ::snprintf(buf, 32, "%d", i);
		if (len > 0) {
			fn = fileNamePrefix + std::string(buf, len);
			if (!fileExistsPrivate(fn)) return fn;
		}
	}
	return std::string();
}

bool MmappedFile::createTempFile(const std::string & fileNameBase, UByteArrayAdapter::OffsetType size, MmappedFile & dest) {
	MmappedFilePrivate * mf = MmappedFilePrivate::createTempFile(fileNameBase, size);
	if (mf) {
		dest = MmappedFile(mf);
	}
	return mf;
}

std::string MmappedFile::findLockFilePath(const std::string & fileNamePrefix, uint32_t maxTest) {
	return findLockFilePathPrivate(fileNamePrefix, maxTest);
}

bool MmappedFile::unlinkFile(const std::string & fileName) {
	return (::unlink(fileName.c_str()) < 0);
}

bool MmappedFile::isDirectory(const std::string & fileName) {
	struct ::stat64 stFileInfo;
	if (::stat64(fileName.c_str(),&stFileInfo) == 0)
		return S_ISDIR( stFileInfo.st_mode );
	else
		return false;
}

bool MmappedFile::createDirectory(const std::string & fileName, __mode_t mode) {
	if (::mkdir(fileName.c_str(), mode) == 0)  {
		::chmod(fileName.c_str(), mode);
		return true;
	}
	else {
		if (isDirectory(fileName)) {
			::chmod(fileName.c_str(), mode);
			return true;
		}
		return false;
	}
}

bool MmappedFile::createSymlink(const std::string & src, const std::string & destination) {
	return (0 == symlink(src.c_str(), destination.c_str()));
}

std::string MmappedFile::realPath(const std::string & path) {
	char buf[PATH_MAX];
	buf[0] = 0;
	::realpath(path.c_str(), buf);
	return std::string(buf);
}

}//end namespace