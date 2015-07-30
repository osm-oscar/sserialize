#ifndef TREE_NODE_H
#define TREE_NODE_H
#include <deque>
#include <map>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/storage/UByteArrayAdapter.h>

template<typename ChildKeyType, typename ItemType>
class TreeNode {
public:
	class ItemTypeSerializer {
		ItemTypeSerializer() {}
		~ItemTypeSerializer() {}
		virtual bool prepend(ItemType v, std::deque<uint8_t> & destination) = 0;
	};
	typedef typename std::map<ChildKeyType, TreeNode*>::iterator ChildIterator;
	typedef typename std::map<ChildKeyType, TreeNode*>::const_iterator ConstChildIterator;

private:
	TreeNode * m_parent;
	std::map<ChildKeyType, TreeNode*> m_children;
	ItemType m_value;
private:
	void addTreeLevelRecurse(uint32_t myLevel, const uint32_t targetLevel, std::deque<TreeNode*> & destination) const {
		if (myLevel == targetLevel) {
			destination.push_back(this);
		}
		else {
			for(ChildIterator it = m_children.begin(); it != m_children.end(); it++) {
				it->second->addTreeLevelRecurse(myLevel+1, targetLevel, destination);
			}
		}
	}
public:
	TreeNode(TreeNode * parent) : m_parent(parent) {}
	~TreeNode() {
		for(ChildIterator it = m_children.begin(); it != m_children.end(); it++) {
			delete it->second;
		}
	}
	uint32_t depth() {
		uint32_t maxSubTreeDepths = 0;
		for(ChildIterator it = m_children.begin(); it != m_children.end(); it++) {
			uint32_t cd = it->second->depth();
			if (cd > maxSubTreeDepths) {
				maxSubTreeDepths = cd;
			}
		}
		return maxSubTreeDepths + 1;
	}

	std::map<ChildKeyType, TreeNode*> & children() { return m_children; }
	ItemType & value() { return m_value;}
	TreeNode* parent() { return m_parent;}
	
	void addTreeLevel(uint32_t level, std::deque<TreeNode*> & destination) const {
		this->addTreeLevelRecurse(0, level, destination);
	}

	//static bool serialize(TreeNode * rootNode, std::deque<uint8_t> & destination);
};

/*
template<typename ChildKeyType, typename ItemType>
bool TreeNode<ChildKeyType, ItemType>::serialize(TreeNode * rootNode, std::deque< uint8_t >& destination) {
	if (!rootNode)
		return false;
	uint32_t depth = rootNode->depth();
	if (depth == 0)
		return false;

	std::map<TreeNode*, uint32_t> nodeOffsets; //from the END of destination!

	ProgressInfo2 progressInfo("level(reverse)", "nodes");
	progressInfo.begin1(depth);
	std::cout << "Serializing Tree:" << std::endl;
	for(int curLevel = depth-1; curLevel >= 0; curLevel--) {
		std::deque< TreeNode<ItemType>* > nodeQue;
		rootNode->addTreeLevel(curLevel, nodeQue);

		progressInfo.begin2(nodeQue.size());

		uint32_t curNodeCount = 0;
		for(std::deque< TreeNode<ItemType>* >::const_reverse_iterator it = nodeQue.rbegin(); it != nodeQue.rend(); it++) {
			TreeNode<ItemType> * curNode = *it;
			
			std::deque<uint32_t> childPtrs;
			for(ConstChildIterator ct = curNode->children().begin(); ct != curNode->children().end(); ct++) {
				childPtrs.push_back(destination.size() - nodeOffsets.at(ct->second));
			}
			uint32_t payloadSize = destination.size();
			
			prependSerialized(curNode->value, destination);
			
			payloadSize = destination.size() - payloadSize;
			
			StaticTreeNode::prependNode(childPtrs, payloadSize, destination);
			nodeOffsets[curNode] = destination.size();
			progressInfo(curLevel, curNodeCount);
			curNodeCount++;
		}
	}
	//now adjust parent pointers which are global pointerss
	UByteArrayAdapter treeData(&destination);
	for(std::map<TreeNode*, uint32_t>::const_iterator nt = nodeOffsets.begin(); nt != nodeOffsets.end(); nt++) {
		uint32_t parentNodeBegin = treeData.size()  - nt->second;
		for(ConstChildIterator ct = nt->first->children().begin(); ct != nt->first->children().end(); ct++) {
			uint32_t nodeBegin = treeData.size() - nodeOffsets.at(ct->second);
			treeData.putUint32(nodeBegin, parentNodeBegin);
		}
	}

}
*/

#endif