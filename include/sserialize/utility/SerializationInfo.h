#ifndef SSERIALIZE_SERIALIZATION_INFO_H
#define SSERIALIZE_SERIALIZATION_INFO_H
#include <sserialize/utility/types.h>

namespace sserialize {

template<typename T_TYPE>
struct SerializationInfo {
	static bool is_fixed_length();
	static OffsetType length();
	static OffsetType max_length();
	static OffsetType min_length();
};

//default overloads

//uint8_t
template<> SerializationInfo<uint8_t>::is_fixed_length() { return true; }
template<> SerializationInfo<uint8_t>::length() { return 1; }
template<> SerializationInfo<uint8_t>::max_length() { return 1; }
template<> SerializationInfo<uint8_t>::min_length() { return 1; }

//uint16_t
template<> SerializationInfo<uint16_t>::is_fixed_length() { return true; }
template<> SerializationInfo<uint16_t>::length() { return 2; }
template<> SerializationInfo<uint16_t>::max_length() { return 2; }
template<> SerializationInfo<uint16_t>::min_length() { return 2; }


//uint32_t
template<> SerializationInfo<uint32_t>::is_fixed_length() { return true; }
template<> SerializationInfo<uint32_t>::length() { return 4; }
template<> SerializationInfo<uint32_t>::max_length() { return 4; }
template<> SerializationInfo<uint32_t>::min_length() { return 4; }

//uint64_t
template<> SerializationInfo<uint64_t>::is_fixed_length() { return true; }
template<> SerializationInfo<uint64_t>::length() { return 8; }
template<> SerializationInfo<uint64_t>::max_length() { return 8; }
template<> SerializationInfo<uint64_t>::min_length() { return 8; }

}//end namespace

#endif