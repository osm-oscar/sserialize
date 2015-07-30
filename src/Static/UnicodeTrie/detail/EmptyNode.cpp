#include <sserialize/Static/UnicodeTrie/detail/EmptyNode.h>
#include <sserialize/Static/Array.h>


namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

EmptyNode::EmptyNode() {}
EmptyNode::EmptyNode(sserialize::UByteArrayAdapter) {}
EmptyNode::~EmptyNode() {}


bool EmptyNode::valid() const { return false; }

uint32_t EmptyNode::strLen() const { return 0; }
UByteArrayAdapter EmptyNode::strData() const {return UByteArrayAdapter();}
std::string EmptyNode::str() const { return std::string();}

uint32_t EmptyNode::childSize() const { return 0;}
uint32_t EmptyNode::childKey(uint32_t) const { return 0;}
uint32_t EmptyNode::find(uint32_t) const { return sserialize::Static::UnicodeTrie::Node::npos;}
Node* EmptyNode::child(uint32_t) const { return new EmptyNode(); }
uint32_t EmptyNode::childPtr(uint32_t) const {return 0;}

uint32_t EmptyNode::payloadPtr() const {
	return 0;
}

}}}}