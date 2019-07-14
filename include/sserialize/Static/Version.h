#ifndef SSERIALIZE_STATIC_VERSION_H
#define SSERIALIZE_STATIC_VERSION_H
#pragma once
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace Static {

template<typename VersionType>
void
ensureVersion(VersionType want, VersionType have) {
	if (want != have) {
		throw sserialize::VersionMissMatchException("", want, have);
	}
}

template<typename VersionType>
sserialize::UByteArrayAdapter &
ensureVersion(sserialize::UByteArrayAdapter & d, VersionType want, VersionType have) {
	ensureVersion(want, have);
	return d;
}

template<typename VersionType>
sserialize::UByteArrayAdapter const &
ensureVersion(sserialize::UByteArrayAdapter const & d, VersionType want, VersionType have) {
	ensureVersion(want, have);
	return d;
}

template<uint8_t TVersion, typename TParent = void>
class SimpleVersion {
public:
	using VersionType = uint8_t;
	static constexpr VersionType value = TVersion;
	struct Consume {};
	struct NoConsume {};
public:
	SimpleVersion() {}
	//this will consume the version information and modify d accordingly
	SimpleVersion(sserialize::UByteArrayAdapter & d, Consume) {
		auto have = d.get<VersionType>(0);
		ensureVersion(SimpleVersion::value, have);
		d += sserialize::SerializationInfo<VersionType>::sizeInBytes(have);
	}
	//this will NOT consume the version information and wil NOT modify 
	SimpleVersion(sserialize::UByteArrayAdapter const & d, NoConsume) {
		auto have = d.get<VersionType>(0);
		ensureVersion(SimpleVersion::value, have);
	}
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const {
		return sserialize::SerializationInfo<VersionType>::length;
	}
};

template<uint8_t TVersion, typename TParent>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, SimpleVersion<TVersion, TParent> const &) {
	dest.put<typename SimpleVersion<TVersion, TParent>::VersionType >(SimpleVersion<TVersion, TParent>::value);
	return dest;
}

}}//end namespace sserialize::Static


#endif
