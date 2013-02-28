#ifndef SSERIALIZE_TESTS_UTIL_ALGOS_H
#define SSERIALIZE_TESTS_UTIL_ALGOS_H
#include <set>
#include <deque>
#include <string>
#include <algorithm>

namespace sserialize {

template<typename T>
std::set<T> intersectSets(const std::deque< std::set<T> > & sets) {
	if (sets.size() == 0)
		return std::set<T>();
	if (sets.size() == 1)
		return sets.front();
	std::set<T> res;
	for(typename std::set<T>::const_iterator it = sets.front().begin(); it != sets.front().end(); it++) {
		bool doInsert = true;
		for(size_t j = 1; j < sets.size(); j++) {
			if (sets.at(j).count(*it) == 0) {
				doInsert = false;
				break;
			}
		}
		if (doInsert) {
			res.insert(*it);
		}
	}
	return res;
}

/** checks strs for strings that match str with qt and returns their positions */
std::set<size_t> match(const std::deque< std::string >& strs, std::string str, unsigned int qt, unsigned int maxMatches);

inline bool doubleEq(double a, double b, double diff) {
	return std::abs(a-b) < diff;
}

}

#endif