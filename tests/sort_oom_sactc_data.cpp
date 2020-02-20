#include <iostream>
#include <sserialize/search/OOMSACTCCreator.h>

typedef sserialize::detail::OOMSACTCCreator::CTCValueStoreNode MyNodeIdentifier;
typedef sserialize::detail::OOMCTCValuesCreator::ValueEntry<MyNodeIdentifier> MyValueEntry;
typedef sserialize::OOMArray<MyValueEntry> MyEntriesContainer;
typedef sserialize::OOMArray<MyValueEntry>::iterator MyTVEIterator;

struct MyNodeIdentifierLessThanComparator {
	bool operator()(const MyNodeIdentifier & a, const MyNodeIdentifier & b) {
		return a < b;
	}
};

struct MyNodeIdentifierEqualComparator {
	bool operator()(const MyNodeIdentifier & a, const MyNodeIdentifier & b) {
		return a == b;
	}
};

struct MyLessThan {
	typedef MyNodeIdentifierLessThanComparator NodeComparator;
	NodeComparator nc;
	MyLessThan(const NodeComparator & nc) : nc(nc) {}
	bool operator()(const MyValueEntry & a, const MyValueEntry & b) {
		if (nc(a.nodeId(), b.nodeId())) {
			return true;
		}
		else if (nc(b.nodeId(), a.nodeId())) {
			return false;
		}
		else { //they compare equal, check the cellId and then the nodeId/FullMatch
			if (a.cellId() < b.cellId()) {
				return true;
			}
			else if (b.cellId() < a.cellId()) {
				return false;
			}
			else { //cellId are the same
				return a.itemId() < b.itemId(); //this moves full match cells to the end
			}
		}
	}
};

struct MyEqual {
	typedef MyNodeIdentifierEqualComparator NodeComparator;
	NodeComparator nc;
	MyEqual(const NodeComparator & nc) : nc(nc) {}
	bool operator()(const MyValueEntry & a, const MyValueEntry & b) {
		return a.cellId() == b.cellId() && nc(a.nodeId(), b.nodeId());
	}
};

enum SrcFileType {
	SFT_INVALID, SFT_MEM, SFT_MMAP, SFT_OOM_ARRAY
};

struct State {
	uint64_t maxMemoryUsage = 0;
};

void help() {
	std::cout << "prg -i inputfile -m <maxMemoryUsage in MiB> -st (oomarray|mmap) -ff <path to slow files> -sf <path to fast files> -a" << std::endl;
}

template<typename TC>
void work(TC & entries, State & state) {
// 	typedef typename TC::iterator MyIterator;
	MyNodeIdentifierLessThanComparator nltp;
	MyNodeIdentifierEqualComparator nep;
	MyLessThan ltp(nltp);
	MyEqual ep(nep);
	
	sserialize::detail::oom::SortTraits<false, MyLessThan, MyEqual> traits(nltp, nep);
	traits
		.maxMemoryUsage(state.maxMemoryUsage)
		.maxThreadCount(2
		).mmt(sserialize::MM_SLOW_FILEBASED)
		.queueDepth(64)
		.maxWait(10)
		.makeUnique(true);
	
	sserialize::oom_sort(entries.begin(), entries.end(), traits);
	using std::is_sorted;
	if(!is_sorted(entries.begin(), entries.end(), ltp)) {
		std::cout << std::endl;
		std::cout << "Entries are not sorted" << std::endl;
		std::cout << std::endl;
	}
}

int main(int argc, char ** argv) {
	State state;
	std::string fileName;
	bool ask = false;
	SrcFileType sft = SFT_INVALID;
	
	for(int i(0); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-i" && i+1 < argc) {
			fileName = std::string(argv[i+1]);
			++i;
		}
		else if (token == "-m" && i+1 < argc) {
			state.maxMemoryUsage = ::atoi(argv[i+1]);
			++i;
		}
		else if (token == "-sf" && i+1 < argc) {
			token = std::string(argv[i+1]);
			sserialize::UByteArrayAdapter::setTempFilePrefix(token);
			++i;
		}
		else if (token == "-ff" && i+1 < argc) {
			token = std::string(argv[i+1]);
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(token);
			++i;
		}
		else if (token == "-st" && i+1 < argc) {
			token = std::string(argv[i+1]);
			if (token == "mmap") {
				sft = SFT_MMAP;
			}
			else if (token == "mem") {
				sft = SFT_MEM;
			}
			else if (token == "oomarray") {
				sft = SFT_OOM_ARRAY;
			}
			else {
				sft = SFT_INVALID;
			}
			++i;
		}
		else if (token == "-a") {
			ask = true;
		}
	}
	
	state.maxMemoryUsage <<= 20;
	
	if (!state.maxMemoryUsage) {
		help();
		return -1;
	}
	
	if (!sserialize::MmappedFile::fileExists(fileName)) {
		std::cout << "File " << fileName << " does not exist" << std::endl;
		return -1;
	}
	
	std::cout << "Inputfile: " << fileName << std::endl;
	std::cout << "MaxMemoryUsage: " << sserialize::prettyFormatSize(state.maxMemoryUsage) << std::endl;
	
	if (ask) {
		std::string ok;
		std::cout << "OK?[yes|NO]" << std::endl;
		std::cin >> ok;
		if (ok != "yes") {
			return 0;
		}
	}
	
	if (sft == SFT_OOM_ARRAY) {
		MyEntriesContainer entries(fileName);
		entries.backBufferSize(100*1024*1024);
		entries.readBufferSize(10*1024*1024);
		work(entries, state);
	}
	else if (sft == SFT_MMAP) {
		sserialize::MmappedMemory<MyValueEntry> entries(fileName);
		work(entries, state);
	}
	else {
		std::cout << "Unsupported storage type" << std::endl;
	}
	return 0;
}
