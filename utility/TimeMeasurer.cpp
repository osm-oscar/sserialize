#include <sserialize/utility/TimeMeasuerer.h>

namespace sserialize {


std::ostream & operator<<(std::ostream & out, const TimeMeasurer & tm) {
	long tms[5];
	tms[0] = tm.elapsedSeconds();//micro
	tms[1] = tms[0]/1000; //milli
	tms[2] = tms[1]/1000; //secons
	tms[3] = tms[2]/60; //minutes
	tms[4] = tms[3]/60; //hours
	const char * e = "umsMd";
	
	bool hasPrev = false;
	for(int i = 4; i >= 0; ++i) {
		if (tms[i]) {
			if (hasPrev)
				out << " ";
			hasPrev = true;
			out << tms[i] << e[i];
		}
	}
	return out;
}



}//end namespace sserialize