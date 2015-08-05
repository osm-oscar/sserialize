#include <sserialize/Static/TracGraph.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/utility/exceptions.h>


namespace sserialize {
namespace Static {
namespace spatial {

TracGraph::TracGraph()
{}

TracGraph::TracGraph(const UByteArrayAdapter& d) :
m_nodeInfo(d+sserialize::SerializationInfo<uint8_t>::length),
m_edgeInfo(d+(sserialize::SerializationInfo<uint8_t>::length+m_nodeInfo.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_TRAC_GRAPH_VERSION, d.at(0), "sserialize::Static::spatial::TracGraph");
}


}}}//end namespace sserialize::Static::spatial