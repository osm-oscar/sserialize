#include <sserialize/containers/UDWIteratorPrivateHD.h>

namespace sserialize {

UDWIteratorPrivateHD::UDWIteratorPrivateHD() {}

UDWIteratorPrivateHD::UDWIteratorPrivateHD(const MultiBitIterator & bitIterator, const Static::HuffmanDecoder & decoder) :
m_bitIterator(bitIterator),
m_decoder(decoder)
{}

UDWIteratorPrivateHD::~UDWIteratorPrivateHD() {}

uint32_t UDWIteratorPrivateHD::next() {
	uint32_t v = m_bitIterator.get32();
	int len = m_decoder.decode(v, v);
	m_bitIterator += std::max<int>(0, len);
	return v;
}

bool UDWIteratorPrivateHD::hasNext() {
	return m_bitIterator.hasNext();
}

UDWIteratorPrivate * UDWIteratorPrivateHD::copy() {
	return new UDWIteratorPrivateHD(m_bitIterator, m_decoder);
}

UByteArrayAdapter::OffsetType UDWIteratorPrivateHD::dataSize() const {
	return m_bitIterator.dataSize();
}

}//end namespace