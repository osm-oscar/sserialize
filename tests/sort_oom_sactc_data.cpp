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

void help() {
	std::cout << "prg -i inputfile -m <maxMemoryUsage in MiB" << std::endl;
}

int main(int argc, char ** argv) {
	uint64_t maxMemoryUsage = 0;
	std::string fileName;
	bool ask = false;
	for(int i(0); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-i" && i+1 < argc) {
			fileName = std::string(argv[i+1]);
			++i;
		}
		else if (token == "-m" && i+1 < argc) {
			maxMemoryUsage = ::atoi(argv[i+1]);
			++i;
		}
		else if (token == "-a") {
			ask = true;
		}
	}
	
	maxMemoryUsage <<= 20;
	
	if (!maxMemoryUsage) {
		help();
		return -1;
	}
	
	if (!sserialize::MmappedFile::fileExists(fileName)) {
		std::cout << "File " << fileName << " does not exist" << std::endl;
		return -1;
	}
	
	std::cout << "Inputfile: " << fileName << std::endl;
	std::cout << "MaxMemoryUsage: " << sserialize::prettyFormatSize(maxMemoryUsage) << std::endl;
	
	if (ask) {
		std::string ok;
		std::cout << "OK?[yes|NO]" << std::endl;
		std::cin >> ok;
		if (ok != "yes") {
			return 0;
		}
	}
	
	MyEntriesContainer entries(fileName);
	entries.backBufferSize(100*1024*1024);
	entries.readBufferSize(10*1024*1024);
	
	MyNodeIdentifierLessThanComparator nltp;
	MyLessThan ltp(nltp);
	sserialize::oom_sort<MyTVEIterator, MyLessThan, true>(entries.begin(), entries.end(), ltp, maxMemoryUsage, 2, sserialize::MM_SLOW_FILEBASED);
	using std::is_sorted;
	if(!is_sorted(entries.begin(), entries.end(), ltp)) {
		std::cout << std::endl;
		std::cout << "Entries are not sorted" << std::endl;
		std::cout << std::endl;
	}
}