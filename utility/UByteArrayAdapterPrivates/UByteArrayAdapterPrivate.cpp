#include "UByteArrayAdapterPrivate.h"

namespace sserialize {

std::string UByteArrayAdapterPrivate::getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const {
	uint8_t * buf = new uint8_t[len];
	get(pos, buf, len);
	std::string str(buf, buf+len);
	delete buf;
	return str;
}


}//end namespace