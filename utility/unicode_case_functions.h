#ifndef COMMON_UNICODE_CASE_FUNCTIONS_H
#define COMMON_UNICODE_CASE_FUNCTIONS_H
#include <stdint.h>
#include <string>

namespace sserialize {

///To lower for unicode characters between 0x0 and 0xFFFF
uint32_t unicode32_to_lower(uint32_t code_point);
///To upper for unicode characters between 0x0 and 0xFFFF
uint32_t unicode32_to_upper(uint32_t code_point);

std::string unicode_to_lower(const std::string & str);
std::string unicode_to_upper(const std::string & str);

}

#endif