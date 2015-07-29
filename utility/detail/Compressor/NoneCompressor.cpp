#include "NoneCompressor.h"
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {
namespace Compressor {

NoneCompressor::NoneCompressor() {}
NoneCompressor::~NoneCompressor() {}

int64_t NoneCompressor::decompress(const UByteArrayAdapter& src, UByteArrayAdapter & dest) const {
	dest.putData(0, src);
	return src.size();
}

int64_t NoneCompressor::compress(const UByteArrayAdapter& src, UByteArrayAdapter & dest) const {
	dest.putData(0, src);
	return src.size();
}

}}}//end namespace sserialize::detail::Compressor