#ifndef OSMFIND_COMMON_LINEAR_REGRESSION_FUNCTIONS_H
#define OSMFIND_COMMON_LINEAR_REGRESSION_FUNCTIONS_H
#include <set>
#include <stdint.h>
#include <cmath>
#include <numeric>

namespace sserialize {

template<class TSortedContainer>
double getMeanValue(const TSortedContainer & ids) {
	uint64_t res = std::accumulate(ids.begin(), ids.end(), (uint64_t)0);
	std::size_t s = ids.size();
	return (double)(res/s) + static_cast<double>(res%s)/s;
}

template<class TSortedContainer>
bool getLinearRegressionParams(const TSortedContainer & ids, double & slope, double & yintercept) {
	double meanY = getMeanValue(ids);
	double meanX = (double)(ids.size()-1);
	if (ids.size() == 2)
		meanX /= 2.0;
	else
		meanX = ((meanX-1)*(meanX-2)/(2.0))/(meanX+1.0);
	double count = 0.0;
	slope = 0.0;
	double slopedenom = 0.0;
	typename TSortedContainer::const_iterator end( ids.end() );
	for(typename TSortedContainer::const_iterator it = ids.begin(); it != end; ++it) {
		slope += (count - meanX)*(*it-meanY);
		slopedenom += (count - meanX)*(count - meanX);
		count += 1.0;
	}
	slope /= slopedenom;
	yintercept = meanY - slope*meanX;//BUG: this sometimes produces very wrong results
	return true;
}



}//end namespace

#endif
