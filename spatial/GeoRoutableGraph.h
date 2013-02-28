#ifndef SSERIALIZE_GEO_ROUTABLE_GRAPH_H
#define SSERIALIZE_GEO_ROUTABLE_GRAPH_H
#include "RoutableGraphInterface.h"

namespace sserialize {
namespace spatial {
namespace routing {

template<typename NodeIdType>
class GeoRoutableGraphInterface: public sserialize::routing::RoutableGraphInterface<NodeIdType> {
public:
	GeoRoutableGraphInterface();
	virtual ~Graph();
	virtual NodeIdType locate(double lat, double lon) const = 0;
};

}}}//end namespace

#endif