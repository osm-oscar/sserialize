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

template<typename VersionType>
sserialize::UByteArrayAdapter const &
ensureVersion(sserialize::UByteArrayAdapter & d,  VersionType want, UByteArrayAdapter::ConsumeTag) {
	ensureVersion(want, d.get<VersionType>());
	return d;
}

template<typename VersionType>
sserialize::UByteArrayAdapter const &
ensureVersion(sserialize::UByteArrayAdapter const & d,  VersionType want, UByteArrayAdapter::NoConsumeTag) {
	ensureVersion(want, d.get<VersionType>(0));
	return d;
}

template<uint8_t TVersion, typename TParent = void>
class SimpleVersion {
public:
	using VersionType = uint8_t;
	static constexpr VersionType value = TVersion;
	using Consume = UByteArrayAdapter::ConsumeTag;
	using NoConsume = UByteArrayAdapter::NoConsumeTag;
public:
	SimpleVersion() {}
	//this will consume the version information and modify d accordingly
	SimpleVersion(sserialize::UByteArrayAdapter & d, UByteArrayAdapter::ConsumeTag c) {
		ensureVersion<VersionType>(d, value, c);
	}
	//this will NOT consume the version information and wil NOT modify 
	SimpleVersion(sserialize::UByteArrayAdapter const & d, UByteArrayAdapter::NoConsumeTag c) {
		ensureVersion<VersionType>(d, value, c);
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

}
	
template<uint8_t TVersion, typename TParent>
struct SerializationInfo<Static::SimpleVersion<TVersion, TParent>>:
	SerializationInfo<typename Static::SimpleVersion<TVersion, TParent>::VersionType>
{
	static OffsetType sizeInBytes(const Static::SimpleVersion<TVersion, TParent> & value) {
		return value.getSizeInBytes();
	}
};

}//end namespace sserialize::Static


#endif
