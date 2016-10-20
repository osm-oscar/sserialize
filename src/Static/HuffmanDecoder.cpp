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
	return HuffmanDecoder::HuffmanCodePointInfo(m_data + (pos*SerializationInfo<HuffmanCodePointInfo>::length));
}

void HuffmanDecoder::StaticNode::readInCache() const {
	uint32_t s = entryCount()*SerializationInfo<sserialize::Static::HuffmanDecoder::HuffmanCodePointInfo>::length;
	uint8_t * tmp = new uint8_t[s];
	m_data.getData(0, tmp, s);
	delete tmp;
}

HuffmanDecoder::HuffmanDecoder() {}

HuffmanDecoder::HuffmanDecoder(const UByteArrayAdapter & data) :
m_nodes(data),
m_root(m_nodes.at(0))
{
	m_root.readInCache();
}

void HuffmanDecoder::readInCache() {
	auto s = m_nodes.size();
	for(decltype(s) i = 0; i < s; ++i) {
		m_nodes.at(i).readInCache();
	}
}

HuffmanDecoder::~HuffmanDecoder() {}

}}