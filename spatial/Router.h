#ifndef SSERIALIZE_ROUTER_H
#define SSERIALIZE_ROUTER_H
#include "RoutableGraphInterface.h"

namespace sserialize {
namespace spatial {
namespace routing {

template<typename NodeIdType>
class Router: public RCWrapper< sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> > {
private:
	
public:
	Router(sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> * graph) : 
		RCWrapper< sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> >(graph) {}
	Router(const Router & other) : RCWrapper< sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> >(other) {}
	virtual ~Router() {}
	Router & operator=(const Router & other) {
		RCWrapper< sserialize::spatial::routing::RoutableGraphInterface<NodeIdType> >::operator=(other);
	}
	
	virtual std::vector<NodeIdType> route(NodeIdType from, NodeIdType to);
};

}}}//end namespace

#endif