#ifndef STATIC_TREE_NODE_H
#define STATIC_TREE_NODE_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/Static/Map.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <iostream>

/** File Format:
 *------------------------------------------------------
 *ParentPtr|PayLoadLength|ChildPtrMap|PAYLOAD
 *------------------------------------------------------
 * 4 Byte  |    4 byte   |    *      |*
 *
 * Usage: subclass StaticTreeNode and implement the functions you need
 *
 * ChildPtrs are from the of the node (after PAYLOAD)
 */

namespace sserialize {
namespace Static {
 
template<typename KeyType, typename PayloadType>
class TreeNode {
private:
	uint32_t m_parentPtr;
	uint32_t m_payLoadLen;
	sserialize::Static::Map<KeyType, uint32_t> m_childMap;
	UByteArrayAdapter m_payloadData;
protected:
	uint32_t payloadLen() {return m_payLoadLen;}
	/** does not check validity of pos! */
	uint32_t childPtrAt(uint32_t pos);
	/** does not check validity of pos! */
	UByteArrayAdapter childNodeDataAt(uint32_t pos) const;
public:
	TreeNode() : m_parentPtr(0), m_payLoadLen(0) {}
	TreeNode(const UByteArrayAdapter & nodeData);
	~TreeNode() {}
	uint32_t parentPtr() { return m_parentPtr;}
	uint32_t childCount() { return m_childMap.size();}
	TreeNode child(const KeyType & key) const;
	sserialize::Static::Map<KeyType, uint32_t> & children() { return m_childMap;}
	PayloadType payload() { return PayloadType(UByteArrayAdapter(m_payloadData, 0, m_payLoadLen));}
	static bool prependNode(const std::map< KeyType, uint32_t >& childPtrs, PayloadType payload, std::deque< uint8_t >& destination);
};

template<typename KeyType, typename PayloadType>
TreeNode<KeyType, PayloadType>::TreeNode(const UByteArrayAdapter& data) : m_payloadData(data) {
	m_payloadData >> m_parentPtr >> m_payLoadLen;
	m_payloadData.shrinkToGetPtr();
	m_payloadData += m_childMap.getSizeInBytes();
}
template<typename KeyType, typename PayloadType>
uint32_t
TreeNode<KeyType, PayloadType>::childPtrAt(uint32_t pos) {
	return m_payLoadLen + m_childMap.at(pos);
}

template<typename KeyType, typename PayloadType>
UByteArrayAdapter 
TreeNode<KeyType, PayloadType>::childNodeDataAt(uint32_t pos) const {
	return m_payloadData + childPtrAt(pos);
}

template<typename KeyType, typename PayloadType>
TreeNode<KeyType, PayloadType>
TreeNode<KeyType, PayloadType>::child(const KeyType & key) const {
	int32_t childPos = m_childMap.findPosition(key);
	if (childPos >= 0) {
		return TreeNode(childNodeDataAt(childPos));
	}
	return TreeNode();
}

template<typename KeyType, typename PayloadType>
bool
TreeNode<KeyType, PayloadType>::
prependNode(const std::map<KeyType, uint32_t>& childPtrs, PayloadType payload, std::deque< uint8_t >& destination) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adap(&data);
	adap.putUint32(0); //dummy parent pointer
	adap.putUint32(0); //put dummy mayloadLen
	adap << childPtrs;
	uint32_t payloadSize = adap.tellPutPtr();
	adap << payload;
	adap.putUint32(4, adap.tellPutPtr() - payloadSize);
	prependToDeque(data, destination);
	return true;
}

}}

#endif