#ifndef SSERIALIZE_UNICODE_TRIE_TRIE_H
#define SSERIALIZE_UNICODE_TRIE_TRIE_H
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/vendor/utf8.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {

template<typename TValue>
class Trie {
private:
	Node m_root;
	sserialize::Static::Deque<TValue> m_values;
public:
	Trie();
	Trie(sserialize::UByteArrayAdapter & d);
	Node getRootNode() const { return m_root; }
	
	template<typename T_OCTET_ITERATOR>
	TValue at(T_OCTET_ITERATOR begin, const T_OCTET_ITERATOR & end) const;
	inline TValue at(const std::string & str) const { return at(str.cbegin(), str.cend());}
};

template<typename TValue>
template<typename T_OCTET_ITERATOR>
TValue Trie::at(T_OCTET_ITERATOR strIt, const T_OCTET_ITERATOR & strEnd) const {
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
				throw std::out_of_range();
			}
		}

		if (nStrIt == nStrEnd && strIt != strEnd) { //node->c is real prefix, strIt points to new element
			uint32_t key = utf8::peek_next(strIt, strEnd);
			uint32_t pos = node.find(key);
			if (pos != Node::npos) {
				utf8::next(strIt, strEnd);
				node = node.child(pos);
				nodeStr = node.str();
				nStrIt = nodeStr.cbegin();
				nStrEnd = nodeStr.cend();
			}
			else {
				throw std::out_of_range();
			}
		}
	}
	
	return m_values.at( node.payloadPtr() );
}


}}}//end namespace

#endif