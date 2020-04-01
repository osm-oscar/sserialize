#include <sserialize/algorithm/oom_algorithm.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/utility/printers.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/MMVector.h>

void help() {
	std::cout << "prg -s <size in mebi> -m <memory-size in mebi> -q <queue depth> -t <thread count> -w <max wait in s> -st <memory|mmap|oompm|oomff|oomsf> -sp <source tmp path> -tp <tmp path> --u[niquify] --no-fetch-lock --no-flush-lock" << std::endl;
}

enum SrcFileType {
	SFT_INVALID, SFT_MEM, SFT_MMAP, SFT_OOM_ARRAY_FF, SFT_OOM_ARRAY_SF
};

struct State {
	uint64_t entryCount = 0;
	uint64_t size = 1 << 30;
	uint64_t memorySize = 1 << 28;
	uint32_t threadCount = 2;
	uint32_t queueDepth = 32;
	uint32_t maxWait = 10;
	sserialize::MmappedMemoryType tmt{sserialize::MM_SLOW_FILEBASED};
	bool uniquify = false;
	bool fetchLock{true};
	bool flushLock{true};
};

template<typename T_SRC_CONTAINER_TYPE>
void worker(T_SRC_CONTAINER_TYPE & data, State & state) {
	data.reserve(state.entryCount);
	
	sserialize::ProgressInfo pinfo;
	sserialize::TimeMeasurer tm;
	
	sserialize::detail::oom::SortTraits<true, std::less<uint64_t>, std::equal_to<uint64_t> > sortTraits;
	sortTraits
		.maxMemoryUsage(state.memorySize)
		.maxThreadCount(state.threadCount)
		.mmt(sserialize::MM_SLOW_FILEBASED)
		.queueDepth(state.queueDepth)
		.maxWait(state.maxWait)
		.ioFetchLock(state.fetchLock)
		.ioFlushLock(state.flushLock)
		.makeUnique(state.uniquify);
	
	if (state.uniquify) {
		pinfo.begin(state.entryCount/3, "Creating file");
		for(int64_t i(state.entryCount/3); i > 0; --i) {
			data.push_back(i);
			data.push_back(i);
			data.push_back(i);
			if ((i & 0x3FF) == 0) {
				pinfo(state.entryCount/3-i);
			}
		}
		pinfo.end();
		tm.begin();
		sserialize::oom_sort(data.begin(), data.end(), sortTraits);
		tm.end();
		std::cout << "Sorting took " << tm << std::endl;
		pinfo.begin(state.entryCount/3, "Verifying");
		for(uint64_t i(0), s(state.entryCount/3); i < s; ++i) {
			if (data.at(i) != i+1) {
				std::cout << "Sort is BROKEN! SHOULD=" << i << "IS=" << data.at(i) << std::endl;
			}
			if (i % 1000 == 0) {
				pinfo(i);
			}
		}
		pinfo.end();
	}
	else {
		pinfo.begin(state.entryCount, "Creating file");
		for(int64_t i(state.entryCount); i > 0; --i) {
			data.push_back(i);
			if ((i & 0x3FF) == 0) {
				pinfo(state.entryCount-i);
			}
		}
		pinfo.end();
		tm.begin();
		sserialize::oom_sort(data.begin(), data.end(), sortTraits);
		tm.end();
		std::cout << "Sorting took " << tm << std::endl;
		pinfo.begin(state.entryCount, "Verifying");
		for(uint64_t i(0), s(state.entryCount); i < s; ++i) {
			if (data.at(i) != i+1) {
				std::cout << "Sort is BROKEN! SHOULD=" << i << "IS=" << data.at(i) << std::endl;
			}
			if (i % 1000 == 0) {
				pinfo(i);
			}
		}
		pinfo.end();
	}
}

int main(int argc, char ** argv) {
	State state;
	SrcFileType sft = SFT_INVALID;
	
	for(int i(1); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-s" && i+1 < argc) {
			state.size = atoll(argv[i+1]) << 20;
			state.entryCount = state.size/sizeof(uint64_t);
			++i;
		}
		else if (token == "-m" && i+1 < argc) {
			state.memorySize = atoll(argv[i+1]) << 20;
			++i;
		}
		else if (token == "-q" && i+1 < argc) {
			state.queueDepth = atoi(argv[i+1]);
			++i;
		}
		else if (token == "-t" && i+1 < argc) {
			state.threadCount = atoi(argv[i+1]);
			++i;
		}
		else if (token == "-w" && i+1 < argc) {
			state.maxWait = atoi(argv[i+1]);
			++i;
		}
		else if (token == "-sp" && i+1 < argc) {
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(std::string(argv[i+1]));
		}
		else if (token == "-tp" && i+1 < argc) {
			sserialize::UByteArrayAdapter::setTempFilePrefix(std::string(argv[i+1]));
		}
		else if (token == "-st" && i+1 < argc) {
			token = std::string(argv[i+1]);
			if (token == "mmap") {
				sft = SFT_MMAP;
			}
			else if (token == "mem" || token == "memory") {
				sft = SFT_MEM;
			}
			else if (token == "oomff") {
				sft = SFT_OOM_ARRAY_FF;
			}
			else if (token == "oomsf") {
				sft = SFT_OOM_ARRAY_SF;
			}
			else {
				sft = SFT_INVALID;
			}
			++i;
		}
		else if (token == "-u" || token == "--uniquify") {
			state.uniquify = true;
		}
		else if (token == "--no-fetch-lock") {
			state.fetchLock = false;
		}
		else if (token == "--no-flush-lock") {
			state.flushLock = false;
		}
		else if (token == "-h" || token == "--help") {
			help();
			return 0;
		}
	}
	
	std::cout << "Fast file path: " << sserialize::UByteArrayAdapter::getFastTempFilePrefix() << '\n';
	std::cout << "Slow file path: " << sserialize::UByteArrayAdapter::getTempFilePrefix() << '\n';
	std::cout << "File size: " << sserialize::prettyFormatSize(state.size) << '\n';
	std::cout << "Memory usage: " << sserialize::prettyFormatSize(state.memorySize) << '\n';
	std::cout << "Thread count: " << state.threadCount << '\n';
	std::cout << "Max wait: " << state.maxWait << '\n';
	std::cout << "Src file type: ";
	switch (sft) {
	case SFT_MEM:
		std::cout << "memory";
		break;
	case SFT_MMAP:
		std::cout << "mmap";
		break;
	case SFT_OOM_ARRAY_FF:
		std::cout << "oomarray fast file based";
		break;
	case SFT_OOM_ARRAY_SF:
		std::cout << "oomarray slow file based";
		break;
	default:
		std::cout << "invalid";
		break;
	};
	std::cout << std::endl;
	switch (sft) {
	case SFT_MEM:
		{
			std::vector<uint64_t> data;
			state.tmt = sserialize::MM_PROGRAM_MEMORY;
			worker(data, state);
			break;
		}
	case SFT_MMAP:
		{
			sserialize::MMVector<uint64_t> data(sserialize::MM_FAST_FILEBASED);
			state.tmt = sserialize::MM_FAST_FILEBASED;
			worker(data, state);
			break;
		}
	case SFT_OOM_ARRAY_FF:
		{
			sserialize::OOMArray<uint64_t> data(sserialize::MM_FAST_FILEBASED);
			state.tmt = sserialize::MM_FAST_FILEBASED;
			data.backBufferSize(100*1024*1024);
			worker(data, state);
			break;
		}
	case SFT_OOM_ARRAY_SF:
		{
			sserialize::OOMArray<uint64_t> data(sserialize::MM_SLOW_FILEBASED);
			state.tmt = sserialize::MM_SLOW_FILEBASED;
			data.backBufferSize(100*1024*1024);
			worker(data, state);
			break;
		}
	default:
		break;
	};

	return 0;
}
