#include <sserialize/vendor/utf8.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <iostream>

void test(std::string::const_iterator strIt, const std::string & str) {
	while(strIt != str.end()) {
		uint32_t ucode = utf8::next(strIt, str.end());
		std::cout << ucode << ":";
	}
	std::cout << std::endl;
}

int main() {

	std::string testStr("Baden-Württemberg");
	test(testStr.begin(), testStr);
	
	std::string testLowerCase = sserialize::unicode_to_lower(testStr);
	test(testLowerCase.begin(), testLowerCase);

	return 0;
}