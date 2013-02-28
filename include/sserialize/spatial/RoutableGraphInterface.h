#ifndef SSERIALIZE_ROUTABLE_GRAPH_INTERFACE_H
#define SSERIALIZE_ROUTABLE_GRAPH_INTERFACE_H
#include <sserialize/utility/refcounting.h>

namespace sserialize {
namespace routing {

template<typename NodeIdType>
class RoutableGraphInterface: public sserialize::RefCountObject {
public:
	RoutableGraphInterface() {}
	virtual ~RoutableGraphInterface() {}
	virtual uint32_t edgeCount(NodeIdType node) const = 0;
	/** @return returns the edge weight associated with nodes edge */
	virtual uint32_t edgeWeight(NodeIdType node, uint32_t edgePosition) const = 0;
	/** @return returns the node id, that the edge points to */
	virtual NodeIdType expand(NodeIdType node, uint32_t edgePosition) const = 0;
};

}} //end namespace

#endif