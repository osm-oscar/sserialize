#include "utilalgos.h"
#include <sserialize/search/StringCompleter.h>
#include <sserialize/strings/unicode_case_functions.h>

namespace sserialize {

std::set<size_t> match(const std::deque< std::string >& strs, std::string str, unsigned int qt, unsigned int maxMatches) {
	std::set<size_t> ret;
	if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
		str = unicode_to_lower(str);
	}
	for(size_t i=0; i < strs.size(); i++) {
		std::string testString;
		if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
			testString = unicode_to_lower(strs[i]);
		}
		else {
			testString = strs[i];
		}
		if (qt & sserialize::StringCompleter::QT_EXACT || qt & sserialize::StringCompleter::QT_PREFIX) {
			size_t firstPos = testString.find(str);
			if (firstPos == 0) {
				if (qt & sserialize::StringCompleter::QT_EXACT && strs[i].size() == str.size()) {
					ret.insert(i);
				}
				else if (qt & sserialize::StringCompleter::QT_PREFIX) {
					ret.insert(i);
				}
			}
		}
		else {
			size_t lastPos = testString.rfind(str);
			if (lastPos < std::string::npos) {
				if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
					ret.insert(i);
				}
				else if (qt & sserialize::StringCompleter::QT_SUFFIX && str.size() + lastPos == strs[i].size()) {
					ret.insert(i);
				}
			}
		}
		if (ret.size() == maxMatches)
			break;
	}
	return ret;
}

}

