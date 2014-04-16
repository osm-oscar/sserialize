#include <sserialize/Static/UnicodeTrie/detail/SimpleNode.h>
#include <algorithm>


namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

SimpleNode::SimpleNode() :
m_payloadPtr(0)
{}

SimpleNode::SimpleNode(UByteArrayAdapter d) {
	d.resetGetPtr();
	uint32_t childCount = d.getVlPackedUint32();
	m_children.reserve(childCount);
	for(uint32_t i(0); i < childCount; ++i) {
		uint32_t childChar = d.getVlPackedUint32();
		uint32_t childPtr = d.getVlPackedUint32();
		m_children.push_back(std::pair<uint32_t, uint32_t>(childChar, childPtr));
	}
	m_payloadPtr = d.getVlPackedUint32();
	m_strLen = d.getStringLength();
	m_strTrieData = d;
	m_strTrieData.shrinkToGetPtr();
}

SimpleNode::~SimpleNode() {}

uint32_t SimpleNode::strLen() const {
	return m_strLen;
}

UByteArrayAdapter SimpleNode::strData() const {
	return UByteArrayAdapter(m_strTrieData,0 , m_strLen);
}

std::string SimpleNode::str() const {
	return strData().toString();
}

uint32_t SimpleNode::childSize() const {
	return m_children.size();
}

uint32_t SimpleNode::childKey(uint32_t pos) const {
	return m_children.at(pos).first;
}

uint32_t SimpleNode::find(uint32_t unicode_point) const {
	if (!m_children.size()) {
		return sserialize::Static::UnicodeTrie::Node::npos;
	}
	ChildrenContainer::const_iterator it(
		std::lower_bound(m_children.cbegin(), m_children.cend(), std::pair<uint32_t, uint32_t>(unicode_point, 0)
		)
	);
	if (it->first == unicode_point) {
		return it-m_children.cbegin();
	}
	else {
		return sserialize::Static::UnicodeTrie::Node::npos;
	}
}

Node* SimpleNode::child(uint32_t pos) const {
	return new SimpleNode(trieData()+childPtr(pos));
}

uint32_t SimpleNode::childPtr(uint32_t pos) const {
	return m_children.at(pos).second;
}


uint32_t SimpleNode::payloadPtr() const {
	return m_payloadPtr;
}

uint32_t SimpleNodeCreator::type() const {
	return UnicodeTrie::Node::NT_SIMPLE;
}

bool SimpleNodeCreator::append(const NodeSerializationInfo & src, sserialize::UByteArrayAdapter & dest) {
	dest.putVlPackedUint32(src.childKeyPtrOffsets.size());
	for(const auto & x : src.childKeyPtrOffsets) {
		dest.putVlPackedUint32(x.first);
		dest.putVlPackedUint32(x.second);
	}
	dest.putVlPackedUint32(src.payloadPtr);
	dest.put(std::string(src.strBegin, src.strEnd));
	return true;
}

}}}}
