#ifndef SSERIALIZE_SERIALIZATION_INFO_H
#define SSERIALIZE_SERIALIZATION_INFO_H
#include <sserialize/utility/types.h>
#include <limits>

namespace sserialize {

template<typename T_TYPE>
struct SerializationInfo {
	static const bool is_fixed_length = false;
	static const OffsetType length = 0;
	static const OffsetType max_length = std::numeric_limits<OffsetType>::max();
	static const OffsetType min_length = 0;
	static OffsetType sizeInBytes(const T_TYPE & value) {
		return value.getSizeInBytes();
	}
};

//default overloads

#define SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(__TYPE, __LENGTH) \
template<> \
struct SerializationInfo<__TYPE> { \
	static const bool is_fixed_length = true; \
	static const OffsetType length = __LENGTH; \
	static const OffsetType max_length = __LENGTH; \
	static const OffsetType min_length = __LENGTH; \
	static inline OffsetType sizeInBytes(const __TYPE & /*value*/) { return __LENGTH; } \
}; \
template<> \
struct SerializationInfo<const __TYPE> { \
	static const bool is_fixed_length = true; \
	static const OffsetType length = __LENGTH; \
	static const OffsetType max_length = __LENGTH; \
	static const OffsetType min_length = __LENGTH; \
	static inline OffsetType sizeInBytes(const __TYPE & /*value*/) { return __LENGTH; } \
}; \
template<> \
struct SerializationInfo<volatile __TYPE> { \
	static const bool is_fixed_length = true; \
	static const OffsetType length = __LENGTH; \
	static const OffsetType max_length = __LENGTH; \
	static const OffsetType min_length = __LENGTH; \
	static inline OffsetType sizeInBytes(const __TYPE & /*value*/) { return __LENGTH; } \
};

SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(uint8_t, 1);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(int8_t, 1);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(uint16_t, 2);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(int16_t, 2);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(uint32_t, 4);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(int32_t, 4);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(uint64_t, 8);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(int64_t, 8);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(double, 8);
SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC(float, 4);

#undef SERIALIZATION_INFO_FIXED_LENGTH_SPECIALICATIONS_FUNC

}//end namespace

#endif
