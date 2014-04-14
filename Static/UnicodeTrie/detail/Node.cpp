#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/UnicodeTrie/detail/SimpleNode.h>
#include <sserialize/Static/UnicodeTrie/detail/EmptyNode.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

Node::Node() :
m_priv( new detail::EmptyNode() )
{}

Node RootNodeAllocator::operator()(uint32_t nodeType, const sserialize::UByteArrayAdapter & src) const {
	switch (nodeType) {
	case Node::NT_SIMPLE:
		return Node( new detail::SimpleNode(src) );
	default:
		return Node();
	};
}

}}}