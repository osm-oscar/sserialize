#include <sserialize/spatial/RTree.h>
#include <unordered_map>


namespace sserialize {
namespace spatial {

std::vector<Node*> RTree::nodesInLevelOrder() {
	if (!m_root)
		return std::vector<Node*>();
		
	std::vector<Node*> nodes;
	nodes.push_back(m_root);
	uint32_t i = 0;
	while ( i < nodes.size()) {
		if (!nodes[i]->leafNode) {
			InnerNode * n = static_cast<InnerNode*>(nodes[i]);
			for(uint32_t j = 0; j < n->children.size(); ++j) {
				nodes.push_back(n->children[j].d);
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

class SerializationInfo : public NodeInfo {
public:
	std::vector<spatial::GeoRect> rects;
	std::vector<uint32_t> children;
	uint32_t indexId;
};

bool serializeNode(SerializationInfo & info, std::deque<uint8_t> & dest) {
	std::vector<uint8_t> tmpData;
	UByteArrayAdapter tmp(&tmpData, false);
	tmp.putVlPackedUint32(info.indexId);
	tmp.putVlPackedUint32(info.children.size());
	
}

void RTree::sserialize(sserialize::UByteArrayAdapter & dest) const {
	std::vector<Node*> nodes = nodesInLevelOrder();
	std::unordered_map<Node*, NodeInfo> nodeToOffset;
	std::deque<uint8_t> sData;
	
	for(int i = nodes.size()-1; i >= 0; --i) {
		Node * curNode = nodes[i];
		
		
		nodeToOffset[curNode] = sData.size();
	}
}

}}//end namespace