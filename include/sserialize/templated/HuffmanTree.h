#ifndef SSERIALIZE_HUFFMAN_TREE_H
#define SSERIALIZE_HUFFMAN_TREE_H
#include <set>
#include <unordered_map>
#include <vector>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/utility/hashspecializations.h>

namespace sserialize {

class HuffmanCodePoint {
	uint32_t m_code;
	uint8_t m_codeLength; //length in alphabetSize (bit length = codeLength*alphabetSize)
public:
	HuffmanCodePoint() : m_code(0), m_codeLength(0) {}
	HuffmanCodePoint(uint32_t code, uint8_t length) : m_code(code), m_codeLength(length) {}
	virtual ~HuffmanCodePoint() {}
	uint8_t codeLength() const { return m_codeLength; }
	uint32_t code() const { return m_code; }
};

class HuffmanCodePointIterator {
private:
	HuffmanCodePoint m_cp;
	uint8_t m_pos;
public:
	HuffmanCodePointIterator();
	virtual ~HuffmanCodePointIterator();
	uint8_t operator*() const;
	HuffmanCodePointIterator & operator++();
	bool operator==(const HuffmanCodePointIterator & other);
};

template<typename TValue>
class HuffmanTree {
private:
	struct Node {
		Node() {}
		virtual ~Node() {}
		virtual uint32_t depth() const {return 1;}
	};

	struct InnerNode: public Node {
		typedef typename std::vector<Node*>::const_iterator ConstChildIterator;
		typedef typename std::vector<Node*>::iterator ChildIterator;
		InnerNode() {}
		virtual ~InnerNode() {
			for(typename std::vector<Node*>::iterator it(children.begin()); it != children.end(); ++it)
				delete *it;
		}
		virtual uint32_t depth() const {
			uint32_t md = 0;
			for(typename std::vector<Node*>::const_iterator it(children.begin()); it != children.end(); ++it)
				md = std::max<uint32_t>(md, (*it)->depth());
			return md+1;
		}
		std::vector<Node*> children; 
	};
	
	struct EndNode: public Node {
		EndNode(const TValue & value) : value(value) {}
		virtual ~EndNode() {}
		TValue value;
	};
	
	template<typename TLookUpTable>
	struct NodePtrComparator {
		TLookUpTable * lt;
		NodePtrComparator(TLookUpTable * lt) : lt(lt) {}
		bool operator()(const Node * a, const Node * b) const {
			return (*lt)[a] < (*lt)[b];
		};
	};
	
private:
	Node * m_root;
	uint32_t m_alphabetSize;
	uint32_t m_alphabetBitLength;
private:
	void codePointMapRecurse(std::unordered_map<uint32_t, HuffmanCodePoint> & dest, Node * node, uint32_t prefixCode, uint8_t length) {
		if (dynamic_cast<EndNode*>(node)) {
			EndNode * en = dynamic_cast<EndNode*>(node);
			dest[en->value] = HuffmanCodePoint(prefixCode, length);
		}
		else if (dynamic_cast<InnerNode*>(node)) {
			InnerNode * in = dynamic_cast<InnerNode*>(node);
			for(uint32_t i = 0; i < in->children.size(); ++i) {
				codePointMapRecurse(dest, in->children[i], (prefixCode << m_alphabetBitLength) | i, length+m_alphabetBitLength);
			}
		}
	};
public:
	HuffmanTree(uint8_t alphabetBitLength = 1) : m_root(0), m_alphabetSize(static_cast<uint32_t>(1) << alphabetBitLength), m_alphabetBitLength(alphabetBitLength) {}
	virtual ~HuffmanTree() {
		delete m_root;
	}
	uint32_t depth() {
		return m_root->depth();
	}
	///@parameter begin AlphabetIterator should return a uint8_t holding ONE character from the alphabet
	template<typename AlphabetIterator>
	TValue at(AlphabetIterator begin, const AlphabetIterator & end, TValue def = TValue()) {
		Node * node = m_root;
		while (begin != end) {
			InnerNode * in = dynamic_cast<InnerNode*>(node);
			if (in && *begin < in->children.size()) {
				node = in->children[*begin];
			}
			else
				return def;
		}
		EndNode * en = dynamic_cast<EndNode*>(node);
		if (en)
			return en->value;
		return def;
	};
	
	//SymbolProbIterator->first points to the Symbol ->second the symbolprobability or symbolcount
	template<typename SymbolProbIterator, typename TFreq>
	void create(const SymbolProbIterator & begin, const SymbolProbIterator & end, TFreq total) {
		typedef std::unordered_map<const Node*, std::pair<TFreq, uint32_t> > MyFreqTableType;
		typedef NodePtrComparator<MyFreqTableType> MyNodePtrComparator;
		typedef std::set<Node*, MyNodePtrComparator> MySortContainer;
		MyFreqTableType freLookUpTable;
		uint32_t nodeCounter = 0;
		MyNodePtrComparator cmp(&freLookUpTable);
		MySortContainer tree( cmp );
		SymbolProbIterator it(begin);
		while(it != end) {
			EndNode * node = new EndNode(it->first);
			++nodeCounter;
			freLookUpTable[node] = std::pair<TFreq, uint32_t>(it->second, nodeCounter);
			tree.insert(node);
			++it;
		};
		
		while (tree.size() > 1) {
			InnerNode * in = new InnerNode();
			++nodeCounter;
			freLookUpTable[in] = std::pair<TFreq, uint32_t>(0, nodeCounter);
			typename MySortContainer::iterator nodeIt;
			while (tree.size() > 0 && in->children.size() < m_alphabetSize) {
				nodeIt = tree.begin();
				Node * node = *nodeIt;
				in->children.push_back(node);
				freLookUpTable[in].first += freLookUpTable[node].first;
				freLookUpTable.erase(node);
				tree.erase(nodeIt);
			}
			tree.insert( in );
		}
		//TODO: reorder tree if the first level is not completely filled
		
		m_root = *tree.begin();
	};

	std::unordered_map<TValue, HuffmanCodePoint> codePointMap() {
		std::unordered_map<TValue, HuffmanCodePoint> ret;
		codePointMapRecurse(ret, m_root, 0, 0);
		return ret;
	};
	
};

}//end namespace
#endif