#include <sserialize/utility/Compressor.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/exceptions.h>
#include "detail/Compressor/Compressors.h"

namespace sserialize {

Compressor::Compressor(Compressor::CompressionTypes ct)
{
	switch(ct) {
	case CT_NONE:
		m_priv.reset(new detail::Compressor::NoneCompressor());
		break;
	case CT_LZO:
		break;
	default:
		throw sserialize::TypeMissMatchException("sserialize::Compressor::Compressor");
		break;
	};
}

Compressor::~Compressor() {}


int64_t Compressor::compress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const {
	return m_priv->compress(src, dest);
}

int64_t Compressor::decompress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const {
	return m_priv->decompress(src, dest);
}



}//end namespace sserialize