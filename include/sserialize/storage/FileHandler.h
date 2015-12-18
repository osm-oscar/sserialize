#ifndef SSERIALIZE_UTIL_FILE_HANDLER_H
#define SSERIALIZE_UTIL_FILE_HANDLER_H
#include <sserialize/utility/types.h>
#include <string>

namespace sserialize {

struct FileHandler {
	static void * mmapFile(int fd, OffsetType size, bool prePopulate, bool randomAccess);
	//mmap a file and create if it does not exist
	static void * mmapFile(const std::string & fileName, int & fd, OffsetType & size, bool prePopulate, bool randomAccess);
	///directIo: you have to make sure that transfer sizes are a multiple of the block size! Use with care!
	static int createTmp(OffsetType fileSize, std::string & tmpFileName, bool fastFile, bool directIo = false);
	static void * createAndMmappTemp(OffsetType fileSize, int & fd, std::string & tmpFileName, bool prePopulate, bool randomAccess, bool fastFile);
	static void * resize(int fd, void * mem, OffsetType oldSize, OffsetType newSize, bool prePopulate, bool randomAccess);
	static bool closeAndUnlink(const std::string & fileName, int fd, void * mem, OffsetType size);
	static bool close(int fd, void* mem, sserialize::OffsetType size, bool sync = false);
	
	///@return fd
	static int shmCreate(std::string & fileName);
	static bool shmDestroy(const std::string & fileName, int fd, void* mem, sserialize::OffsetType size);
};

}//end namespace

#endif