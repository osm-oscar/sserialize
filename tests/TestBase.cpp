#include "TestBase.h"
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace tests {

int TestBase::argc = 0;
char ** TestBase::argv = 0;

TestBase::TestBase() {
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
	}
}

TestBase::~TestBase() {}

}} //end namespace sserialize:.tests