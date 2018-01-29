#include <sserialize/utility/debuggerfunctions.h>
#include <sserialize/utility/debug.h>

namespace sserialize {

	NO_OPTIMIZE NO_INLINE bool breakHere() {
		return true;
	}

	NO_OPTIMIZE NO_INLINE bool breakHereIf(bool stop) {
		if (stop)
			return breakHere();
		else
			return false;
	}
	
}