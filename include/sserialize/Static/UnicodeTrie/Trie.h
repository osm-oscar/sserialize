#ifndef SSERIALIZE_UNICODE_TRIE_TRIE_H
#define SSERIALIZE_UNICODE_TRIE_TRIE_H
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/Static/Array.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/containers/UnicodeStringMap.h>
#include <queue>
#define SSERIALIZE_STATIC_UNICODE_TRIE_TRIE_VERSION 1

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

/** Trie-Layout
  * ---------------------------------------
  * VERSION|Payload|NodeType|TrieNodes*
  * ---------------------------------------
  *  uint8 |Array  |uint32_t|
  *
  *
  */

template<typename TValue>
class Trie {
public:
	typedef sserialize::Static::UnicodeTrie::Node Node;
private:
	Node m_root;
	sserialize::Static::Array<TValue> m_values;
public:
	Trie() {}
	Trie(const sserialize::UByteArrayAdapter & d);
	///@param rootNodeAllocator allocator for the root node: Node rootNodeAllocator(uint32_t nodeType, const UByteArrayAdapter & src), used once, not stored
	template<typename T_ROOT_NODE_ALLOCATOR>
	Trie(const sserialize::UByteArrayAdapter & d, T_ROOT_NODE_ALLOCATOR rootNodeAllocator);
	Node getRootNode() const { return m_root; }

	template<typename T_OCTET_ITERATOR>
	Node find(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;

	///@param prefixMatch strIt->strEnd can be a prefix of the path
	///throws sserialize::OutOfBoundsException on miss
	template<typename T_OCTET_ITERATOR>
	TValue at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR& strEnd, bool prefixMatch) const;
	///throws sserialize::OutOfBoundsException on miss
	inline TValue at(const std::string & str, bool prefixMatch) const { return at(str.cbegin(), str.cend(), prefixMatch);}
	inline TValue payload(uint32_t id) const { return m_values.at(id);}
	const sserialize::Static::Array<TValue> & payloads() const { return m_values;}
	std::ostream & printStats(std::ostream & out) const;
	///Apply @fn to every node in level-order, it gets a @Node as parameter
	template<typename T_FUNC>
	void apply(T_FUNC fn) const;
};

template<typename TValue>
class UnicodeStringMapTrie: public sserialize::detail::UnicodeStringMap<TValue> {
public:
	typedef Trie<TValue> TrieType;
	typedef typename TrieType::Node Node;
private:
	TrieType m_trie;
public:
	UnicodeStringMapTrie() {}
	UnicodeStringMapTrie(const UByteArrayAdapter & d) : m_trie(d) {}
	UnicodeStringMapTrie(const Trie<TValue> & t) : m_trie(t) {}
	virtual ~UnicodeStringMapTrie() {}
	virtual TValue at(const std::string & str, bool prefixMatch) const override {
		return m_trie.at(str.cbegin(), str.cend(), prefixMatch);
	}
	virtual bool count(const std::string & str, bool prefixMatch) const override {
		return m_trie.find(str.cbegin(), str.cend(), prefixMatch).valid();
	}
	virtual std::ostream & printStats(std::ostream & out) const override {
		return m_trie.printStats(out);
	}
	const TrieType & trie() const { return m_trie; }
	Node getRootNode() const { return m_trie.getRootNode(); }
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
typename Trie<TValue>::Node
Trie<TValue>::find(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
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
				return Node();
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
				return Node();
			}
		}
	}
	
	if (nStrIt != nStrEnd && !prefixMatch) {
		return Node();
	}
	return node;
}

template<typename TValue>
template<typename T_OCTET_ITERATOR>
TValue Trie<TValue>::at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd, bool prefixMatch) const {
	Node node = find(strIt, strEnd, prefixMatch);
	if (!node.valid()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::UnicodeTrie::Trie::at");
	}
	return m_values.at( node.payloadPtr() );
}

template<typename TValue>
template<typename T_FUNC>
void Trie<TValue>::apply(T_FUNC fn) const {
	std::queue<Node> nodes;
	nodes.push(m_root);
	while (nodes.size()) {
		fn(nodes.front());
		nodes.pop();
	}
}

template<typename TValue>
std::ostream & Trie<TValue>::printStats(std::ostream & out) const {
	out << "sserialize::Static::UnicodeTrie::Trie--BEGIN_STATS" << std::endl;
	out << "Payload size=" << payloads().size() << std::endl;
	out << "Payload storage size=" << payloads().getSizeInBytes() << std::endl;
	out << "Average storage size per payload item=" << (double)payloads().getSizeInBytes()/payloads().size() << std::endl;
	out << "sserialize::Static::UnicodeTrie::Trie--END_STATS" << std::endl;
	return out;
}

}}}//end namespace

#endif