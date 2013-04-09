#include <sserialize/spatial/RTree.h>
#include <unordered_map>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/spatial/GeoPoint.h>


namespace sserialize {
namespace spatial {

std::vector<RTree::Node*> RTree::nodesInLevelOrder() const {
	if (!m_root)
		return std::vector<Node*>();
		
	std::vector<Node*> nodes;
	nodes.push_back(m_root);
	uint32_t i = 0;
	while ( i < nodes.size()) {
		if (!nodes[i]->leafNode) {
			InnerNode * n = static_cast<InnerNode*>(nodes[i]);
			for(uint32_t j = 0; j < n->children.size(); ++j) {
				nodes.push_back(n->children[j]);
			}
		}
		++i;
	}
	return nodes;
}

class NodeInfo {
public:
	std::vector<uint32_t> items;
	uint32_t offset;
};

class SerializationInfo {
public:
	void reserve(uint32_t s) {
		rects.reserve(s);
		children.reserve(s);
	}
	std::vector<spatial::GeoRect> rects;
	std::vector<uint32_t> children;
	uint32_t indexId;
};

bool prependSerializedNode(SerializationInfo & info, std::deque<uint8_t> & dest) {
	std::vector<uint8_t> tmpData;
	UByteArrayAdapter tmp(&tmpData, false);
	tmp.putVlPackedUint32(info.indexId);
	tmp.putVlPackedUint32(info.children.size());
	for(uint32_t i = 0; i < info.children.size(); ++i) {
		tmp << info.rects[i];
		tmp.putVlPackedUint32(info.children[i]);
	}
	prependToDeque(tmpData, dest);
	return true;
}

void RTree::serialize(sserialize::UByteArrayAdapter & dest, ItemIndexFactory & indexFactory) {
	std::vector<Node*> nodes = nodesInLevelOrder();
	std::unordered_map<Node*, NodeInfo> nodeToNodeInfo;
	std::deque<uint8_t> sData;
	
	for(int i = nodes.size()-1; i >= 0; --i) {
		SerializationInfo si;
		Node * curNode = nodes[i];
		if (curNode->leafNode) {
			nodeToNodeInfo[curNode].items.swap(static_cast<LeafNode*>(curNode)->items);
		}
		else { //inner node, set childPtrs and child rects and collect items
			NodeInfo & curNodeInfo = nodeToNodeInfo[curNode];
			InnerNode * in = static_cast<InnerNode*>(curNode);
			si.reserve(in->children.size());
			for(uint32_t i = 0; i < in->children.size(); ++i) {
				Node * curChildNode = in->children.at(i);
				NodeInfo & childNodeInfo = nodeToNodeInfo[curChildNode];
				
				si.rects.push_back(curChildNode->rect);
				si.children.push_back(sData.size() - childNodeInfo.offset);
				mergeSortedContainer(curNodeInfo.items, curNodeInfo.items, childNodeInfo.items);
				
				nodeToNodeInfo.erase(curChildNode);
			}
		}
		
		si.indexId = indexFactory.addIndex(nodeToNodeInfo[curNode].items);
		prependSerializedNode(si, sData);
		nodeToNodeInfo[curNode].offset = sData.size();
	}
	std::cout << "RTree::sserialize: writing to dest..." <<  std::flush;
	dest.put(sData);
	std::cout << "done" << std::endl;
}

}}//end namespace