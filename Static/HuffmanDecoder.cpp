#include <sserialize/Static/HuffmanDecoder.h>

namespace sserialize {
namespace Static {

HuffmanDecoder::StaticNode::StaticNode() : m_bitLength(0), m_initialChildPtr(0) {}
HuffmanDecoder::StaticNode::StaticNode(const UByteArrayAdapter & data) :
m_data(data),
m_bitLength(m_data.getUint8()),
m_initialChildPtr(m_data.getVlPackedUint32())
{
	m_data.shrinkToGetPtr();
}

HuffmanDecoder::StaticNode::~StaticNode() {}

HuffmanDecoder::HuffmanCodePointInfo HuffmanDecoder::StaticNode::at(uint32_t pos) const {
	return HuffmanDecoder::HuffmanCodePointInfo(m_data.getUint32(pos*SerializationInfo<HuffmanCodePointInfo>::length()), m_data.getUint24(pos*SerializationInfo<HuffmanCodePointInfo>::length()+4));
}

HuffmanDecoder::HuffmanDecoder() {}

HuffmanDecoder::HuffmanDecoder(const UByteArrayAdapter & data) :
m_nodes(data),
m_root(m_nodes.at(0))
{}

HuffmanDecoder::~HuffmanDecoder() {}

}}