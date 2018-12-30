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

template<typename TVersionType = uint8_t, typename TBase = void>
class Version: public TBase {
public:
	using VersionType = TVersionType;
public:
	Version(VersionType want, VersionType have) {
		ensureVersion(want, have);
	}
};

}}//end namespace sserialize::Static


#endif
