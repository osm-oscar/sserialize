#ifndef SSERIALIZE_UDW_ITERATOR_HD_H
#define SSERIALIZE_UDW_ITERATOR_HD_H
#include <sserialize/containers/UDWIterator.h>
#include <sserialize/containers/MultiBitIterator.h>
#include <sserialize/Static/HuffmanDecoder.h>

namespace sserialize {

class UDWIteratorPrivateHD: public UDWIteratorPrivate {
	MultiBitIterator m_bitIterator;
	Static::HuffmanDecoder m_decoder;
public:
	UDWIteratorPrivateHD();
	UDWIteratorPrivateHD(const MultiBitIterator & bitIterator, const Static::HuffmanDecoder & decoder);
	virtual ~UDWIteratorPrivateHD();
	virtual uint32_t next();
	virtual uint64_t next64();
	virtual bool hasNext();
	virtual UDWIteratorPrivate * copy();
};



}//end namespace

#endif