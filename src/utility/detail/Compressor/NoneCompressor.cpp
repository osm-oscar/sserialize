#include "NoneCompressor.h"
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {
namespace Compressor {

NoneCompressor::NoneCompressor() {}
NoneCompressor::~NoneCompressor() {}

int64_t NoneCompressor::decompress(const UByteArrayAdapter& src, UByteArrayAdapter & dest) const {
	dest.putData(0, src);
	return (int64_t) src.size();
}

int64_t NoneCompressor::compress(const UByteArrayAdapter& src, UByteArrayAdapter & dest) const {
	dest.putData(0, src);
	return (int64_t) src.size();
}

}}}//end namespace sserialize::detail::Compressor