#include <sserialize/utility/assert.h>
#include <sserialize/utility/debug.h>

namespace sserialize {
namespace {

NO_OPTIMIZE void assertion_failed_wait() {
#ifdef SSERIALIZE_WAIT_ON_FAILED_ASSERTION
	bool wait = true;
	while (wait) {
		::sleep(1);
	}
#else
	std::abort();
#endif
}

} //end namespace


void assertion_failed(const char * msg) {
	std::cout << "ASSERTION FAILED!" << msg << std::endl;
	assertion_failed_wait();
}

void assertion_failed(std::string const & msg) {
	assertion_failed(msg.c_str());
}



}
