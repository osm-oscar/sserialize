#ifndef SSERIALIZE_DETAIL_COMPRESSOR_INTERFACE_H
#define SSERIALIZE_DETAIL_COMPRESSOR_INTERFACE_H
#include <sserialize/utility/refcounting.h>

namespace sserialize {
class UByteArrayAdapter;

namespace detail {
namespace Compressor {

class Interface: public RefCountObject {
public:
	Interface() {}
	virtual ~Interface() {}
	virtual int64_t compress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const = 0;
	virtual int64_t decompress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const = 0;
};

}}}//end namespace

#endif