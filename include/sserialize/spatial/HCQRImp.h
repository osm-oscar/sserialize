#ifndef SSERIALIZE_SPATIAL_HCQR_IMP_H
#define SSERIALIZE_SPATIAL_HCQR_IMP_H
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {

/** Hierarchical CQR
  * Based on a tree
  * The tree is such that the following holds:
  * Each inner node spans a set of child nodes
  * An inner node is a full-match if each of its children is a full match
  * An inner node is a partial-match if at least one of its children is a partial match and at least one child is not a full match
  * A partial match cell is a partial match leaf node
  * A full match cell is a full match leaf node
  * Full match nodes do not need to list their children
  * 
  * Trees are based on the same cell arrangement.
  * Hence a node can be uniquely defined by the cells it spans
  * 
  * The following is in question:
  * All cqr use the same tree
  * All children of a node represent disjunct sets of cells (likey want that)
  *
  * What might be good:
  * cqr trees have the same root-node, based on same gierarchy-graph
  * hence they can be any subree of a given graph.
  * 
  *
  * Doing set operations:
  * Intersection:
  * Consider two nodes a and b with the same regiond-id
  * a && b full match -> full-match
  * a || b full match -> copy non-fullmatch with subtree
  * a && b partial match -> descend into common nodes
  * 
  * What to do with non-common child nodes?
  * If children are disjunct and cqr use the same tree -> throw away
  * If children are disjunct and cqr do NOT use the same tree multiple options:
  *   *Handle common nodes first
  *   *Handle node exculsive cells
  *   *We now have non-common nodes and non-common cells:
  *     -Examine everything on the cell level and re-construct local subtree
  *     -Expand all remaining children of a and b into new super node a_sub and b_sub and execute intersection on super node
  *      After this step some kind of re-construction should be done (maybe greedy set cover?)
  *      This can be done on the children level since they are all enclosed by the parent node
  *      In the worst case this will result in an expansion down to the cell level (hence this method has an overhead of a factor of 2?)
  *      How to expand full-match nodes (in the tree these are leafs, but in the dag they are not)?
  *      What to do with cells that are not part of any listed child but may be part of a child of the other tree
  *    
  * If children are NOT disjunct and cqr use the same tree:
  *   -This means that there may be cells part of the common nodes that are also part of non common nodes
  *    -> we have to inspect everything on the cell level (or need to know which children to check with each other)
  * If children are NOT disjunct and cqr do NOT use the same tree:
  *   -This is even worse than before. We can first compute the common nodes and
  *    then inspect everything on the cell level for the non-common nodes
  *
  * Some examples for same graph, root node, different tree:
  * Full-match regions are expanded to all 
  *
  */

namespace detail {
namespace HCQR {


class Node {
public:
	uint32_t childrenBegin();
	uint32_t childrenEnd();
};

class RegionInfo {
public:
	uint32_t regionExclusiveCellsBegin() const;
	uint32_t regionExclusiveCellsEnd() const;
};

}} //end namespace detail::HCQR


/* we need a mapping from region -> (full region, full cells, partial cells)
 * It has to support fast query 
 * 
 */

class HCQRImp {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
public:
	HCQRImp();
	HCQRImp(const CellQueryResult & cqr);
	virtual ~HCQRImp();
public:
	bool operator!=(const HCQRImp * other) const;
	bool operator==(const HCQRImp * other) const;
public:
	uint32_t maxItems() const;
	sserialize::ItemIndex::Types defaultIndexType() const;
public:
	HCQRImp * unite(const HCQRImp * other) const;
	HCQRImp * difference(const HCQRImp * other) const;
	HCQRImp * intersect(const HCQRImp * other) const;
	HCQRImp * allToFull() const;
public:
	ItemIndex flaten() const;
	ItemIndex topK(uint32_t numItems) const;
public:
	void dump(std::ostream & out) const;
	void dump() const;
public:
	const GeoHierarchy & gh() const;
	const ItemIndexStore & store() const;
private:
	ItemIndexStore m_store;
	GeoHierarchy m_gh;
	std::vector<RegionInfo> m_regionInfo;
	std::vector<uint32_t> m_cellInfo; //points to m_cqr
	CellQueryResult m_cqr;
};

}} //end namespace sserialize::spatial

#endif