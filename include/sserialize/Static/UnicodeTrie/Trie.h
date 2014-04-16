#ifndef SSERIALIZE_UNICODE_TRIE_TRIE_H
#define SSERIALIZE_UNICODE_TRIE_TRIE_H
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/vendor/utf8.h>
#define SSERIALIZE_STATIC_UNICODE_TRIE_TRIE_VERSION 1

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

/** Trie-Layout
  * ---------------------------------------
  * VERSION|Payload|NodeType|TrieNodes*
  * ---------------------------------------
  *  uint8 |Deque  |uint32_t|
  *
  *
  */

template<typename TValue>
class Trie {
public:
	typedef sserialize::Static::UnicodeTrie::Node Node;
private:
	Node m_root;
	sserialize::Static::Deque<TValue> m_values;
public:
	Trie() {}
	Trie(const sserialize::UByteArrayAdapter & d);
	///@param rootNodeAllocator allocator for the root node: Node rootNodeAllocator(uint32_t nodeType, const UByteArrayAdapter & src), used once, not stored
	template<typename T_ROOT_NODE_ALLOCATOR>
	Trie(const sserialize::UByteArrayAdapter & d, T_ROOT_NODE_ALLOCATOR rootNodeAllocator);
	Node getRootNode() const { return m_root; }
	
	///@param prefixMatch strIt->strEnd can be a prefix of the path
	template<typename T_OCTET_ITERATOR>
	TValue at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	inline TValue at(const std::string & str, bool prefixMatch) const { return at(str.cbegin(), str.cend(), prefixMatch);}
	inline TValue payload(uint32_t id) const { return m_values.at(id);}
};

template<typename TValue>
Trie<TValue>::Trie(const UByteArrayAdapter & d) :
m_values(d+1)
{
	UByteArrayAdapter::OffsetType off = m_values.getSizeInBytes()+1;
	uint32_t rootNodeType = d.getUint32(off);
	off += sserialize::SerializationInfo< uint32_t >::length;
	m_root = sserialize::Static::UnicodeTrie::RootNodeAllocator()(rootNodeType, d+off);
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_UNICODE_TRIE_TRIE_VERSION, d.at(0), "sserialize::Static::UnicodeTrie::Trie");
}



template<typename TValue>
template<typename T_ROOT_NODE_ALLOCATOR>
Trie<TValue>::Trie(const UByteArrayAdapter & d, T_ROOT_NODE_ALLOCATOR rootNodeAllocator) :
m_values(d+1)
{
	UByteArrayAdapter::OffsetType off = m_values.getSizeInBytes()+1;
	uint32_t rootNodeType = d.getUint32(off);
	off += sserialize::SerializationInfo< uint32_t >::length;
	m_root = rootNodeAllocator(rootNodeType, d+off);
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_UNICODE_TRIE_TRIE_VERSION, d.at(0), "sserialize::Static::UnicodeTrie::Trie");
}

template<typename TValue>
template<typename T_OCTET_ITERATOR>
TValue Trie<TValue>::at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
	Node node(getRootNode());

	std::string nodeStr(node.str());
	std::string::const_iterator nStrIt(nodeStr.cbegin());
	std::string::const_iterator nStrEnd(nodeStr.cend());
	
	//Find the end-node
	while(strIt != strEnd) {
		while(strIt != strEnd && nStrIt != nStrEnd) {
			if (*strIt == *nStrIt) {
				++strIt;
				++nStrIt;
			}
			else { //no common prefix
				throw sserialize::OutOfBoundsException("sserialize::Static::UnicodeTrie::Trie::at");
			}
		}

		if (nStrIt == nStrEnd && strIt != strEnd) { //node->c is real prefix, strIt points to new element
			uint32_t key = utf8::next(strIt, strEnd);
			uint32_t pos = node.find(key);
			if (pos != Node::npos) {
				node = node.child(pos);
				nodeStr = node.str();
				nStrIt = nodeStr.cbegin();
				nStrEnd = nodeStr.cend();
			}
			else {
				throw sserialize::OutOfBoundsException("sserialize::Static::UnicodeTrie::Trie::at");
			}
		}
	}
	
	if (nStrIt != nStrEnd && !prefixMatch) {
		throw sserialize::OutOfBoundsException("sserialize::Static::UnicodeTrie::Trie::at");
	}
	
	return m_values.at( node.payloadPtr() );
}


}}}//end namespace

#endif