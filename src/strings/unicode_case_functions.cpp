#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/strings/unicode_case_table.h>
#include <sserialize/vendor/utf8.h>

namespace sserialize {

inline int binarySearchCase(uint16_t * arrayStart, int len, uint16_t key) {
	if (len == 0) {
		return -1;
	}
	int left = 0;
	int right = len-1;
	int mid = (right-left)/2+left;
	while( left < right ) {
		if (arrayStart[mid] == key) return mid;
		if (arrayStart[mid] < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	return ((arrayStart[mid] == key) ? mid : -1);
}

uint32_t unicode32_to_lower(uint32_t code_point) {
	if (code_point <= 0xFF) {
		int pos = binarySearchCase(unicode_source_case_table, UNICODE_CASE_TABLE_SIZE, (uint16_t)code_point);
		if (pos >= 0 && unicode_lower_case_table[pos] > 0) {
			return unicode_lower_case_table[pos];
		}
		else {
			return code_point;
		}
	}
	return code_point;
}

uint32_t unicode32_to_upper(uint32_t code_point) {
	if (code_point <= 0xFF) {
		int pos = binarySearchCase(unicode_source_case_table, UNICODE_CASE_TABLE_SIZE, (uint16_t)code_point);
		if (pos >= 0 && unicode_upper_case_table[pos] > 0) {
			return unicode_upper_case_table[pos];
		}
		else {
			return code_point;
		}
	}
	return code_point;
}

std::string unicode_to_lower(const std::string & str) {
	std::string retStr;
	std::string::const_iterator strIt = str.begin();
	uint32_t ucode;
	while(strIt != str.end()) {
		ucode = utf8::peek_next(strIt, str.end());
		ucode = unicode32_to_lower(ucode);
		utf8::append(ucode, std::back_inserter(retStr));
		utf8::next(strIt, str.end());
	}
	return retStr;
}

std::string unicode_to_upper(const std::string & str) {
	std::string retStr;
	std::string::const_iterator strIt = str.begin();
	uint32_t ucode;
	while(strIt != str.end()) {
		ucode = utf8::peek_next(strIt, str.end());
		ucode = unicode32_to_upper(ucode);
		utf8::append(ucode, std::back_inserter(retStr));
		utf8::next(strIt, str.end());
	}
	return retStr;
}

}//end namespace