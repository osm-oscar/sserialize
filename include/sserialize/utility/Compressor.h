#ifndef SSERIALIZE_COMPRESSOR_H
#define SSERIALIZE_COMPRESSOR_H
#include <sserialize/utility/refcounting.h>

namespace sserialize {
class UByteArrayAdapter;

namespace detail {
namespace Compressor {

class Interface;

}}//end namespace

class Compressor {
public:
	typedef enum {CT_NONE=0, CT_LZO=1} CompressionTypes;
private:
	sserialize::RCPtrWrapper<detail::Compressor::Interface> m_priv;
public:
	Compressor(CompressionTypes ct = CT_NONE);
	virtual ~Compressor();
	///compress src to dest starting at dest[0]
	///@return number of bytes written to dest
	int64_t compress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const;
	///decompress src to dest starting at dest[0]
	///@return number of bytes written to dest
	int64_t decompress(const sserialize::UByteArrayAdapter & src, sserialize::UByteArrayAdapter & dest) const;
};

}//end namespace

#endif