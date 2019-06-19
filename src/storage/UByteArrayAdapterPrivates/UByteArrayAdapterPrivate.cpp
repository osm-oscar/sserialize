#include "UByteArrayAdapterPrivate.h"

namespace sserialize {
SSERIALIZE_NAMESPACE_INLINE_UBA_NON_CONTIGUOUS
namespace UByteArrayAdapterNonContiguous {
std::string UByteArrayAdapterPrivate::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	uint8_t * buf = new uint8_t[len];
	get(pos, buf, len);
	std::string str(buf, buf+len);
	delete[] buf;
	return str;
}


}}//end namespace sserialize::UByteArrayAdapterNonContiguous
