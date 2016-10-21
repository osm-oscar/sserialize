#include <sserialize/storage/CompressedMmappedFile.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/stats/ProgressInfo.h>
#include <iostream>


void help() {
std::cout << "-i inFile -o outFile -cc minCompressionRatio chunkSizeExponent [-verify]" << std::endl;
}

int main(int argc, char ** argv) {
	if (argc < 7) {
		help();
		return 1;
	}
	std::string inFile, outFile;
	double minCompressionRatio = -1;
	int chunkSizeExponent = -1;
	bool verify = false;
	
	for(int i = 0; i < argc; ++i) {
		std::string str(argv[i]);
		if (str == "-i" && i+1 < argc) {
			inFile = std::string(argv[i+1]);
			++i;
		}
		else if (str == "-o" && i+1 < argc) {
			outFile = std::string(argv[i+1]);
			++i;
		}
		else if (str == "-cc" && i+2 < argc) {
			minCompressionRatio = atof(argv[i+1]);
			chunkSizeExponent = atoi(argv[i+2]);
			i+=2;
		}
		else if (str == "-verify") {
			verify = true;
		}
	}
	
	std::cout << "in-file: " << inFile << std::endl;
	std::cout << "out-file: " << outFile << std::endl;
	std::cout << "minCompressionRatio: " << minCompressionRatio << std::endl;
	std::cout << "chunkSizeExponent: " << chunkSizeExponent << std::endl;
	std::cout << "verify: " << (verify ? "true" : "false") << std::endl;
	
	if (inFile.empty() || outFile.empty() || minCompressionRatio < 0 || chunkSizeExponent < 10) {
		help();
		return 1;
	}
	
	std::cout << "Creating compressed file" << std::endl;
	
	if (!sserialize::MmappedFile::fileExists(inFile)) {
		std::cout << "In-file does not exist" << std::endl;
		return 1;
	}
	
	if (sserialize::MmappedFile::fileExists(outFile)) {
		std::cout << "out-file exists" << std::endl;
		return 1;
	}
	
	sserialize::UByteArrayAdapter inFileData( sserialize::UByteArrayAdapter::openRo(inFile, false, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT) );
	
	if (!inFileData.size()) {
		std::cout << "Failed to open in-file" << std::endl;
		return 1;
	}
	
	if (inFileData.size() != sserialize::MmappedFile::fileSize(inFile)) {
		std::cout << "inFileData differs from real file size" << std::endl;
	}
	
	sserialize::UByteArrayAdapter outFileData( sserialize::UByteArrayAdapter::createFile(1, outFile) );
	if (!outFileData.size()) {
		std::cout << "Failed to create out-file" << std::endl;
		return 1;
	}
	
	std::cout << "In-File size:" << inFileData.size() << std::endl;
	
	if ( ! sserialize::CompressedMmappedFile::create(inFileData, outFileData, chunkSizeExponent, minCompressionRatio) ) {
		std::cout << "Failed to create compressed file. Deleting remainders" << std::endl;
		outFileData.setDeleteOnClose(true);
		return 1;
	}
	
	
	if (verify) {
		outFileData = sserialize::UByteArrayAdapter();
		outFileData = sserialize::UByteArrayAdapter::openRo(outFile, true, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT);
	
		if (!outFile.size()) {
			std::cout << "Failed to open compressed file" << std::endl;
			return 1;
		}

		sserialize::ProgressInfo info;
		uint32_t chunkSize = 1.3*(static_cast<uint32_t>(1) << chunkSizeExponent);
		uint8_t * inBuf = new uint8_t[chunkSize];
		uint8_t * outBuf = new uint8_t[chunkSize];
		sserialize::UByteArrayAdapter::OffsetType dataSize = inFileData.size();
		info.begin(dataSize, "Verfifying");
		for(sserialize::UByteArrayAdapter::OffsetType i = 0; i < dataSize; i += chunkSize) {
			sserialize::UByteArrayAdapter::OffsetType readLen = chunkSize;
			if (dataSize-i < chunkSize)
				readLen = dataSize - i;
			sserialize::UByteArrayAdapter::OffsetType inReadLen = inFileData.getData(i, inBuf, readLen);
			sserialize::UByteArrayAdapter::OffsetType outReadLen = outFileData.getData(i, outBuf, readLen);
			if (readLen != inReadLen) {
				std::cout << "Failed to read " << readLen << " from in-file at " << i << std::endl;
				return 1;
			}
			if (readLen != outReadLen) {
				std::cout << "Failed to read " << readLen << " from out-file at " << i << std::endl;
				return 1;
			}
			for(sserialize::UByteArrayAdapter::OffsetType j = 0; j < readLen; ++j) {
				if (inBuf[j] != outBuf[j]) {
					std::cout << "outFile[" << i+j << "] != inFile[" << i+j << "]" << std::endl;
					return 1;
				}
			}
			info(i);
		}
		info.end("Verifying");
	}
	
	return 0;
}