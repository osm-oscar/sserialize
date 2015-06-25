#include <sserialize/utility/TimeMeasuerer.h>

namespace sserialize {


std::ostream & operator<<(std::ostream & out, const TimeMeasurer & tm) {
	long tms[3];
	tms[0] = tm.elapsedSeconds()%60; //seconds
	tms[1] = (tm.elapsedSeconds()/60)%60; //minutes
	tms[2] = tm.elapsedSeconds()/3600; //hours
	const char * e = "sMh";
	
	bool hasPrev = false;
	for(int i = 2; i >= 0; --i) {
		if (tms[i]) {
			if (hasPrev)
				out << " ";
			hasPrev = true;
			out << tms[i] << e[i];
		}
	}
	if (!hasPrev) {
		out << "0s";
	}
	return out;
}



}//end namespace sserialize