#ifndef FILEWRITER_H
#define FILEWRITER_H
#include <deque>
#include <list>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include  <sserialize/utility/CompressedMmappedFile.h>

namespace sserialize {


inline int64_t writeCompressed(const std::string & fileName, const UByteArrayAdapter & src, double minCompressionRatio, uint8_t chunkSizeExponent) {
	UByteArrayAdapter outFile( UByteArrayAdapter::createFile(10, fileName) );
	bool ok = CompressedMmappedFile::create(src, outFile, chunkSizeExponent, minCompressionRatio);
	if (!ok)
		return -1;
	else
		return outFile.size();
}

template<typename octetIterator> int64_t writeBytesToFile(const std::string & fileName, octetIterator begin, octetIterator end) {
	size_t bufferSize = 8192;
	uint8_t fileBuffer[bufferSize];
	FILE * dataFile;
	dataFile = fopen(fileName.c_str(), "wb");
	size_t fileSize = 0;
	
	if (!dataFile) {
		perror("writeBytesToFile");
		return 0;
	}
	for(octetIterator i = begin; i != end;) {
		size_t bufferPointer = 0; //point to next write element = length of the buffer
		for(size_t j=0; j < bufferSize && i != end; j++) {
			fileBuffer[j] = *i;
			i++;
			bufferPointer++;
		}
		size_t writeLen = fwrite(fileBuffer, 1, bufferPointer, dataFile);
		if (writeLen != bufferPointer) {
			perror("writeBytesToFile");
			fclose(dataFile);
			return -1;
		}
		fileSize += writeLen;
	}
	int fd = fileno(dataFile);
	if (fd < 0) {
		fclose(dataFile);
		return -1;
	}
	fsync(fd);
	if (fclose(dataFile) != 0) return -1;
	return fileSize;
}


inline int64_t writeDequeToFile(const std::string & fileName, std::deque<uint8_t> &data) {
	return writeBytesToFile(fileName, data.begin(), data.end());
}

inline int64_t writeListToFile(const std::string & fileName, std::list<uint8_t> &data) {
	return writeBytesToFile(fileName, data.begin(), data.end());
}

}//end namespace

#endif