#include <sserialize/storage/MmappedMemory.h>

namespace sserialize {
	
std::string toString(MmappedMemoryType mmt) {
	switch (mmt) {
		case MM_SHARED_MEMORY: return "shared memory";
		case MM_SLOW_FILEBASED: return "slow file";
		case MM_FAST_FILEBASED: return "fast file";
		case MM_PROGRAM_MEMORY: return "program memory";
		default: return "invalid";
	}
}

void from(std::string const & str, MmappedMemoryType & mmt) {
	if ("shared" == str || "shm" == str || "shared memory" == str) {
		mmt = MM_SHARED_MEMORY;
	}
	else if ("slow-file" == str || "slow file" == str || "slowfile" == str) {
		mmt = MM_SLOW_FILEBASED;
	}
	else if ("fast-file" == str || "fast file" == str || "fastfile" == str) {
		mmt = MM_FAST_FILEBASED;
	}
	else if ("file" == str) {
		mmt = MM_FILEBASED;
	}
	else if ("program memory" == str || "progmem" == str || "mem" == str || "memory" == str) {
		mmt = MM_PROGRAM_MEMORY;
	}
	else {
		mmt = MM_INVALID;
	}
}
	
} //end namespace sserialize
