#include <sserialize/stats/TimeMeasuerer.h>
#include <array>

namespace sserialize {


std::ostream & operator<<(std::ostream & out, const TimeMeasurer & tm) {
	
	constexpr TimeMeasurer::TimeType div_h = (TimeMeasurer::TimeType(1000)*1000*3600);
	constexpr TimeMeasurer::TimeType div_m = (TimeMeasurer::TimeType(1000)*1000*60);
	constexpr TimeMeasurer::TimeType div_s = (TimeMeasurer::TimeType(1000)*1000);
	constexpr TimeMeasurer::TimeType div_ms = (TimeMeasurer::TimeType(1000));
	
	TimeMeasurer::TimeType elusec = tm.elapsedUseconds();
	bool hasPrev = false;
	int numPrints = 0;
	
	if (elusec/div_h) {
		out << elusec/div_h << "h";
		elusec = elusec%div_h;
	}
	
	if (hasPrev || elusec/div_m) {
		if (hasPrev) {
			out << " ";
		}
		out << elusec/div_m << "m";
		elusec = elusec%div_m;
		hasPrev = true;
		numPrints += 1;
	}
	
	if (hasPrev || elusec/div_s) {
		if (hasPrev) {
			out << " ";
		}
		out << elusec/div_s << "s";
		elusec = elusec%div_s;
		hasPrev = true;
		numPrints += 1;
	}
	
	if (numPrints < 3 && (hasPrev || elusec/div_ms)) {
		if (hasPrev) {
			out << " ";
		}
		out << elusec/div_ms << "ms";
		elusec = elusec%div_ms;
		hasPrev = true;
		numPrints += 1;
	}
	
	if (numPrints < 3 && (hasPrev || elusec)) {
		if (hasPrev) {
			out << " ";
		}
		out << elusec<< "us";
	}
	return out;
}



}//end namespace sserialize
