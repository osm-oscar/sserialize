#ifndef SSERIALIZE_DETAIL_COMPRESSOR_NONE_COMPRESSOR_H
#define SSERIALIZE_DETAIL_COMPRESSOR_NONE_COMPRESSOR_H
#include "Interface.h"

namespace sserialize {
namespace detail {
namespace Compressor {

class NoneCompressor: public Interface {
public:
	NoneCompressor();
	virtual ~NoneCompressor();
	virtual int64_t decompress(const sserialize::UByteArrayAdapter& src, sserialize::UByteArrayAdapter & dest) const override;
	virtual int64_t compress(const sserialize::UByteArrayAdapter& src, sserialize::UByteArrayAdapter & dest) const override;
};

}}}//end namespace sserialize::detail::Compressor

#endif