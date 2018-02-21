#include <sserialize/Static/RTree.h>
#include <sserialize/Static/GeoShape.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/containers/ItemIndexFactory.h>


namespace sserialize {
namespace Static {
namespace spatial {

RTree::Node::Node() : m_indexPtr(0), m_size(0) {}
RTree::Node::Node(const UByteArrayAdapter & data) : m_childrenData(data) {
	m_indexPtr = m_childrenData.getVlPackedUint32();
	m_size = m_childrenData.getVlPackedUint32();
	m_rects.resize(m_size);
	m_childrenPtr.resize(m_size);
	for(uint32_t i = 0; i < m_size; i++) {
		m_childrenData >> m_rects[i];
		m_childrenPtr[i] = m_childrenData.getVlPackedUint32();
	}
	m_childrenData.shrinkToGetPtr();
}

RTree::RTree() {}
RTree::~RTree() {}

RTree::RTree(const sserialize::UByteArrayAdapter & data, const Static::ItemIndexStore & indexStore) :
m_indexStore(indexStore)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_RTREE_VERSION, data[0], "sserialize::Static::spatial::RTree");
	UByteArrayAdapter d = data + 1;
	d >> m_rootBoundary;
	d.shrinkToGetPtr();
	m_root = std::shared_ptr<Node>( new Node(d) );
}

void RTree::handleIndex(const sserialize::spatial::GeoRect & rect, uint32_t id, sserialize::DynamicBitSet & dest, const sserialize::Static::spatial::RTree::ElementIntersecter * intersecter) {
	if (intersecter) {
		ItemIndex idx(m_indexStore.at(id));
		uint32_t size = idx.size();
		uint32_t itemId;
		for(uint32_t i = 0; i < size; ++i) {
			itemId = idx.at(i);
			if ((*intersecter)(itemId, rect)) {
				dest.set(itemId);
			}
		}
	}
	else {
		m_indexStore.at(id).putInto(dest);
	}
}

void RTree::intersectRecurse(const std::shared_ptr<Node> & node, const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, const ElementIntersecter * intersecter) {
	if (node->size()) { // no leaf
		uint32_t size = node->size();
		for(uint32_t i = 0; i < size; ++i) {
			if (rect.overlap(node->rectAt(i))) {
				if (rect.contains(node->rectAt(i))) { //full overlap
					handleIndex(rect, node->childIndexPtr(i), dest, 0);
				}
				else {
					intersectRecurse(node->childAt(i), rect, dest, intersecter);
				}
			}
		}
	}
	else { //leaf node, this means our boundary intersects with the search rect but does not contain it
		handleIndex(rect, node->indexPtr(), dest, intersecter);
	}
}

void RTree::intersect(const sserialize::spatial::GeoRect & rect, DynamicBitSet & dest, ElementIntersecter * intersecter) {
	if (!m_rootBoundary.overlap(rect)) {
		return;
	}
	else if (rect.contains(m_rootBoundary)) {
		handleIndex(rect, m_root->indexPtr(), dest, 0);
		return;
	}
	else {
		intersectRecurse(m_root, rect, dest, intersecter);
	}
}

void RTree::intersectRecurse(const std::shared_ptr<Node> & node, const sserialize::spatial::GeoRect & rect, ItemIndex & fullyContained, ItemIndex & intersected) {
	if (node->size()) { // no leaf
		uint32_t size = node->size();
		for(uint32_t i = 0; i < size; ++i) {
			if (rect.overlap(node->rectAt(i))) {
				if (rect.contains(node->rectAt(i))) { //full overlap
					fullyContained = fullyContained + m_indexStore.at(node->childIndexPtr(i));
				}
				else {
					ItemIndex tmpFullyContained, tmpIntersected;
					intersectRecurse(node->childAt(i), rect, tmpFullyContained, tmpIntersected);
					fullyContained = fullyContained + tmpFullyContained;
					intersected = intersected + tmpIntersected;
				}
			}
		}
	}
	else { //leaf node, this means our boundary intersects with the search rect but does not contain it
		intersected = m_indexStore.at(node->indexPtr());
	}
}

ItemIndex RTree::intersect(const sserialize::spatial::GeoRect & rect, ElementIntersecter * intersecter) {
	ItemIndex fullyContained, intersected;
	intersectRecurse(m_root, rect, fullyContained, intersected);
	if (intersecter) {
		std::vector<uint32_t> intersectedIds;
		for(uint32_t i = 0, s = intersected.size(); i < s; ++i) {
			uint32_t id = intersected.at(i);
			if (intersecter->operator()(id, rect)) {
				intersectedIds.push_back(id);
			}
		}
		if (intersectedIds.size()) {
			return fullyContained + ItemIndexFactory::create(intersectedIds, m_indexStore.indexTypes());
		}
		return fullyContained;
	}
	return fullyContained + intersected;
}


}}}

