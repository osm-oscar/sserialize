#ifndef SSERIALIZE_TYPE_TRAITS_H
#define SSERIALIZE_TYPE_TRAITS_H
#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#include <type_traits>
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

namespace sserialize {
	template<typename __T>
	struct is_trivially_copyable {
		static constexpr bool value = IS_TRIVIALLY_COPYABLE(__T);
	};
}//end namespace sserialize

#endif