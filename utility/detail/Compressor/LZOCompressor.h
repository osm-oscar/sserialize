#ifndef SSERIALIZE_DETAIL_COMPRESSOR_LZO_COMPRESSOR_H
#define SSERIALIZE_DETAIL_COMPRESSOR_LZO_COMPRESSOR_H
#include "Interface.h"

namespace sserialize {
namespace detail {
namespace Compressor {

class LzoCompressor: public Interface {
public:
	LzoCompressor();
	virtual ~LzoCompressor();
	virtual int64_t compress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const override;
	virtual int64_t decompress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const override;
};

}}}//end namespace


#endif