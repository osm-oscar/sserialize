#ifndef SSERIALIZE_UDW_ITERATOR_HD_H
#define SSERIALIZE_UDW_ITERATOR_HD_H
#include <sserialize/containers/UDWIterator.h>
#include <sserialize/containers/MultiBitIterator.h>
#include <sserialize/Static/HuffmanDecoder.h>

namespace sserialize {

class UDWIteratorPrivateHD: public UDWIteratorPrivate {
	MultiBitIterator m_bitIterator;
	RCPtrWrapper<Static::HuffmanDecoder> m_decoder;
public:
	UDWIteratorPrivateHD();
	UDWIteratorPrivateHD(const MultiBitIterator & bitIterator, const RCPtrWrapper<Static::HuffmanDecoder> & decoder);
	virtual ~UDWIteratorPrivateHD();
	virtual uint32_t next();
	virtual bool hasNext();
	virtual void reset();
	virtual UDWIteratorPrivate * copy() const;
	virtual UByteArrayAdapter::OffsetType dataSize() const;
};



}//end namespace

#endif