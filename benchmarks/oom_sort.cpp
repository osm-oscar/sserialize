#include <sserialize/algorithm/oom_algorithm.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/utility/printers.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/MMVector.h>

void help() {
	std::cout << "prg -s <size in mebi> -m <memory-size in mebi> -q <queue depth> -t <thread count> -p <temp-path>" << std::endl;
}

int main(int argc, char ** argv) {
	uint64_t size = 1 << 30;
	uint64_t memorySize = 1 << 28;
	uint32_t threadCount = 2;
	uint32_t queueDepth = 32;
	std::string path;
	
	for(int i(1); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-s" && i+1 < argc) {
			size = atoll(argv[i+1]) << 20;
			++i;
		}
		else if (token == "-m" && i+1 < argc) {
			memorySize = atoll(argv[i+1]) << 20;
			++i;
		}
		else if (token == "-q" && i+1 < argc) {
			queueDepth = atoi(argv[i+1]);
			++i;
		}
		else if (token == "-t" && i+1 < argc) {
			threadCount = atoi(argv[i+1]);
			++i;
		}
		else if (token == "-p" && i+1 < argc) {
			path = std::string(argv[i+1]);
			sserialize::UByteArrayAdapter::setTempFilePrefix(path);
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(path);
		}
		else if (token == "-h" || token == "--help") {
			help();
			return 0;
		}
	}
	if (path.empty()) {
		help();
		return -1;
	}
	
	std::cout << "Creating a file at " << path << "\n";
	std::cout << "File size: " << sserialize::prettyFormatSize(size) << "\n";
	std::cout << "Memory usage: " << sserialize::prettyFormatSize(memorySize) << std::endl;
	
	uint64_t entryCount = size/sizeof(uint64_t);
	
	sserialize::MMVector<uint64_t> data(sserialize::MM_FILEBASED);
	data.reserve(entryCount);
	
	sserialize::ProgressInfo pinfo;
	sserialize::TimeMeasurer tm;
	
	pinfo.begin(entryCount, "Creating file");
	for(int64_t i(entryCount); i > 0; --i) {
		data.push_back(i);
		if ((i & 0x3FF) == 0) {
			pinfo(entryCount-i);
		}
	}
	tm.end();
	std::cout << "File creation took " << tm << std::endl;
	tm.begin();
	sserialize::oom_sort(data.begin(), data.end(), std::less<uint64_t>(), memorySize, threadCount, sserialize::MM_FILEBASED, queueDepth);
	tm.end();
	std::cout << "Sorting took " << tm << std::endl;
	return 0;
}