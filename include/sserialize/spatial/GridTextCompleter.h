#ifndef SSERIALIZE_SPATIAL_GRID_TEXT_COMPLETER_H
#define SSERIALIZE_SPATIAL_GRID_TEXT_COMPLETER_H
#include <sserialize/spatial/GridRegionTree.h>
#include <sserialize/search/OOMCTCValueStore.h>

namespace sserialize {
namespace spatial {
	
//Query -> Nodes in tree matching the query, for each node, the matching items
//(in case of item queries, or full match)
//-> cellIds are grid nodes
//Order of cellIds: level order?
//For item nodes: always pick the highest node fully containing the item?
//For region nodes: pick the highest node fully containing the region?
//or pick at most k nodes representing the region?
//A query is hierarchical, a simple version HCQR
	
class GridTextCompleter: public sserialize::spatial::GridRegionTree {
public:
	using MyBaseClass = sserialize::spatial::GridRegionTree;
public:
	GridTextCompleter(const MyBaseClass & base);
	GridTextCompleter(MyBaseClass && base);
	virtual ~GridTextCompleter();
public:
	
};
	
}}//end namespace sserialize::spatial

#endif
