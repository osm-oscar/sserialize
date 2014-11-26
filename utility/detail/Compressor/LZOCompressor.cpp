#include "LZOCompressor.h"
#include <sserialize/utility/UByteArrayAdapter.h>
#include <vendor/libs/minilzo/minilzo.h>

namespace sserialize {
namespace detail {
namespace Compressor {

LzoCompressor::LzoCompressor() {}

LzoCompressor::~LzoCompressor() {}

#define HEAP_ALLOC_MINI_LZO(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]


int64_t LzoCompressor::compress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const {
	HEAP_ALLOC_MINI_LZO(wrkmem, LZO1X_1_MEM_COMPRESS);
	const UByteArrayAdapter::MemoryView srcD = src.getMemView(0, src.size());
	UByteArrayAdapter::MemoryView destD = dest.getMemView(0, dest.size());
	lzo_uint destLen = destD.size();
	int ok = ::lzo1x_1_compress(srcD.get(), srcD.size(), destD.get(), &destLen, wrkmem);
	if (ok != LZO_E_OK) {
		return -1;
	}
	else {
		destD.flush(destLen);
		return destLen;
	}
}

int64_t LzoCompressor::decompress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const {
	const UByteArrayAdapter::MemoryView srcD = src.getMemView(0, src.size());
	UByteArrayAdapter::MemoryView destD = dest.getMemView(0, dest.size());
	lzo_uint destLen = destD.size();
	int ok = ::lzo1x_decompress_safe(srcD.get(), src.size(), destD.get(), & destLen, 0);
	if (ok != LZO_E_OK) {
		return -1;
	}
	else {
		destD.flush(destLen);
		return destLen;
	}
}


}}}//end namespace