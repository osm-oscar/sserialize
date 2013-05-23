#ifndef SSERIALIZE_HUFFMAN_TREE_H
#define SSERIALIZE_HUFFMAN_TREE_H
#include <set>
#include <unordered_map>
#include <vector>
#include <sserialize/utility/ProgressInfo.h>

namespace sserialize {

class HuffmanCodePoint {
	uint8_t * m_data;
	uint8_t m_codeLength; //length in alphabetSize (bit length = codeLength*alphabetSize)
	uint8_t m_alphabetSize;
public:
	HuffmanCodePoint(uint8_t alphabetSize);
	virtual ~HuffmanCodePoint();
	uint8_t size() const;
	HuffmanCodePoint & operator+=(uint8_t alphabetChar);
	
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
		bool operator()(const Node * a, const Node * b) {
			return (*lt)[a] < (*lt)[b];
		};
	};
	
private:
	Node * m_root;
	uint32_t m_alphabetSize;
	uint32_t m_alphabetBitLength;
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
		typedef std::unordered_map<const Node*, TFreq> MyFreqTableType;
		typedef NodePtrComparator<MyFreqTableType> MyNodePtrComparator;
		typedef std::set<Node*, MyNodePtrComparator> MySortContainer;
		MyFreqTableType freLookUpTable;
		MyNodePtrComparator cmp(&freLookUpTable);
		MySortContainer tree( cmp );
		SymbolProbIterator it(begin);
		while(it != end) {
			Node * node = new EndNode(it->first);
			freLookUpTable[node] = it->second;
			tree.insert(node);
			++it;
		};
		while (tree.size() > 1) {
			InnerNode * in = new InnerNode();
			freLookUpTable[in] = TFreq(0);
			typename MySortContainer::iterator nodeIt;
			while (tree.size() > 0 && in->children.size() < m_alphabetSize) {
				nodeIt = tree.begin();
				Node * node = *nodeIt;
				in->children.push_back(node);
				freLookUpTable[in] += freLookUpTable[node];
				freLookUpTable.erase(node);
				tree.erase(nodeIt);
			}
			tree.insert( in );
		}
		//TODO: reorder tree if the first level is not completely filled
		
		m_root = *tree.begin();
	};

	std::unordered_map<TValue, HuffmanCodePoint> codePointMap() {
		
	};
	
};

}//end namespace
#endif