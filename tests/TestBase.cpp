#include "TestBase.h"
#include <ostream>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace tests {

int TestBase::argc = 0;
char ** TestBase::argv = 0;
bool TestBase::sm_popProtector = false;
bool TestBase::sm_printHelp = false;

TestBase::TestBase() {
	if (!argc) {
		throw std::runtime_error("sserialize::tests::TestBase:: need to call init first!");
	}
}

TestBase::~TestBase() {}

void TestBase::init(int argc, char ** argv) {
	TestBase::argc = argc;
	TestBase::argv = argv;

	for(int i(0); i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "--tc-fast-temp-file" && i+1 < argc) {
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(std::string(argv[i+1]));
			++i;
		}
		else if (token == "--tc-slow-temp-file" && i+1 < argc) {
			sserialize::UByteArrayAdapter::setTempFilePrefix(std::string(argv[i+1]));
			++i;
		}
		else if (token == "--tc-shm-file" && i+1 < argc) {
			sserialize::FileHandler::setShmPrefix(std::string(argv[i+1]));
		}
		else if (token == "--pop-protector") {
			sm_popProtector = true;
		}
		else if (token == "--help" || token == "-h") {
			sm_printHelp = true;
		}
	}
	
	if (printHelp()) {
		std::cout << "Default options available: \n" \
					"\t--tc-fast-temp-file\tpath to fast temp files\n" \
					"\t--tc-slow-temp-file\tpath to slow temp files\n" \
					"\t--tc-shm-file\tpath to shm temp files\n" \
					"\t--pop-protector\tpop exeception protector" << std::endl;
	}
}


bool TestBase::popProtector(){
	return sm_popProtector;
}

bool TestBase::printHelp() {
	return sm_printHelp;
}



}} //end namespace sserialize:.tests
