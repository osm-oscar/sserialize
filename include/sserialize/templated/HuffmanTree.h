#ifndef SSERIALIZE_HUFFMAN_TREE_H
#define SSERIALIZE_HUFFMAN_TREE_H
#include <set>
#include <unordered_map>
#include <vector>
#include <boost-1_49/boost/concept_check.hpp>
#include <sserialize/utility/ProgressInfo.h>
#include <sserialize/utility/hashspecializations.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/utility/utilfuncs.h>
#include <vendor/libs/minilzo/lzoconf.h>

namespace sserialize {

class HuffmanCodePoint {
	uint64_t m_code;
	uint8_t m_codeLength; //length in alphabetSize (bit length = codeLength*alphabetSize)
public:
	HuffmanCodePoint() : m_code(0), m_codeLength(0) {}
	HuffmanCodePoint(uint64_t code, uint8_t length) : m_code(code), m_codeLength(length) {}
	virtual ~HuffmanCodePoint() {}
	uint8_t codeLength() const { return m_codeLength; }
	uint64_t code() const { return m_code; }
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
		virtual uint32_t levelOfFirstLeaf() const { return 1; }
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
		virtual uint32_t levelOfFirstLeaf() const {
			uint32_t md = std::numeric_limits<uint32_t>::max();
			for(typename std::vector<Node*>::const_iterator it(children.begin()); it != children.end(); ++it)
				md = std::min<uint32_t>(md, (*it)->levelOfFirstLeaf());
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
public:
	struct SerializationNode {
		SerializationNode(uint8_t bitLength) : bitLength(bitLength), m_childrenValues(1 << bitLength), m_childrenPtrs(1 << bitLength, -1), m_codePointLength(1 << bitLength, 0) {}
		uint8_t bitLength;
		std::vector<TValue> m_childrenValues;
		std::vector<int> m_childrenPtrs;
		std::vector<uint8_t> m_codePointLength;
		
		
		void addCode(uint32_t code, uint8_t length, const TValue & value) {
			uint8_t bitShift = bitLength - length;
			uint32_t begin = code << bitShift;
			uint32_t end = begin | sserialize::createMask(bitShift);
			for(; begin <= end; ++begin) {
				m_childrenValues[begin] = value;
				m_childrenPtrs[begin] = -1;
				m_codePointLength[begin] = length;
			}
		}
		
		void addChild(uint32_t code, int childPos) {
			m_childrenPtrs[code] = childPos;
			m_codePointLength[code] = 0;
		}
		
		sserialize::UByteArrayAdapter & serialize(sserialize::UByteArrayAdapter & dest) const {
			dest.putUint8(bitLength);
			int beginChildPtr = std::numeric_limits<int>::max();
			for(auto x : m_childrenPtrs) {
				if (x > 0) {
					beginChildPtr = std::min<int>(beginChildPtr, x);
				}
			}
			if (beginChildPtr == std::numeric_limits<int>::max())
				beginChildPtr = -1;
				
			if (beginChildPtr > 0)
				dest.putVlPackedUint32(beginChildPtr);
			else
				dest.putVlPackedUint32(0);
			
			for(uint32_t i = 0; i < m_childrenPtrs.size(); ++i) {
				dest << m_childrenValues[i];
				uint32_t childPtrLen = m_codePointLength[i];
				if (m_childrenPtrs[i] >= 0) {
					childPtrLen = ((m_childrenPtrs[i]-beginChildPtr) << 6);
					childPtrLen |= m_codePointLength[i];
				}
				dest.putUint24(childPtrLen);
			}
			return dest;
		}
	};
private:
	Node * m_root;
	uint32_t m_alphabetSize;
	uint32_t m_alphabetBitLength;
private:

	void codePointMapRecurse(std::unordered_map<TValue, HuffmanCodePoint> & dest, Node * node, uint32_t prefixCode, uint8_t length) {
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
	
	
	struct SerializationRecursionInfo {
		SerializationRecursionInfo(uint32_t destinationNode, uint8_t level, uint32_t prefixCode, uint8_t length) :
		destinationNode(destinationNode), level(level), prefixCode(prefixCode), length(length) {}
		uint32_t destinationNode;
		uint8_t level;
		uint32_t prefixCode;
		uint8_t length;
	};
	
	bool serialize(const std::vector<uint8_t> & bpl, std::vector<SerializationNode> & szn, Node * node, SerializationRecursionInfo & info) {
		bool ok = true;
		if (dynamic_cast<EndNode*>(node)) {
			EndNode * en = dynamic_cast<EndNode*>(node);
			szn[info.destinationNode].addCode(info.prefixCode, info.length, en->value);
		}
		else if (dynamic_cast<InnerNode*>(node)) {
			InnerNode * in = dynamic_cast<InnerNode*>(node);
			if (info.length == bpl[info.level]) {
				//push the code and pointer to the child node to the current parent node
				uint32_t childId = szn.size();
				szn[info.destinationNode].addChild(info.prefixCode, childId);
				szn.push_back( SerializationNode(bpl[info.level+1]) );
				for(uint32_t i = 0; i < in->children.size(); ++i) {
					SerializationRecursionInfo cinfo(childId, info.level+1, i, m_alphabetBitLength);
					ok = serialize(bpl, szn, in->children[i], cinfo) && ok;
				}
			}
			else {
				for(uint32_t i = 0; i < in->children.size(); ++i) {
					SerializationRecursionInfo cinfo(info.destinationNode, info.level, (info.prefixCode << m_alphabetBitLength) | i, info.length + m_alphabetBitLength);
					ok = serialize(bpl, szn, in->children[i], cinfo) && ok;
				}
			}
		}
		else {
			ok = false;
		}
		return ok;
	};
public:
	HuffmanTree(uint8_t alphabetBitLength = 1) : m_root(0), m_alphabetSize(static_cast<uint32_t>(1) << alphabetBitLength), m_alphabetBitLength(alphabetBitLength) {}
	virtual ~HuffmanTree() {
		delete m_root;
	}
	uint32_t depth() const {
		return m_root->depth();
	}
	
	uint32_t levelOfFirstLeaf() const {
		return m_root->levelOfFirstLeaf();
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
	
	typedef void (*ValueSerializer)(sserialize::UByteArrayAdapter & dest, const TValue & src);

	
	///It's up to the user to set correct bitsPerLevel depending on the alphabetBitLength (i.e. everylevel has to have n*alphabetBitLength where n is a natural number)
	bool serialize(sserialize::UByteArrayAdapter & dest, ValueSerializer valueSerializer, const std::vector<uint8_t> & bitsPerLevel) {
		{
			uint8_t myDepth = depth();
			uint32_t maxDepth = std::accumulate(bitsPerLevel.begin(), bitsPerLevel.end(), static_cast<uint32_t>(0));
			if (m_alphabetBitLength* myDepth > maxDepth) {
				std::cerr << "Cannot create huffman tree. Not enough bits available" << std::endl;
				return false;
			}
		}
	
		std::vector<SerializationNode> nodes;
		nodes.push_back(SerializationNode(bitsPerLevel[0]));
		SerializationRecursionInfo recInfo(0, 0, 0, 0);
		
		bool ok = serialize(bitsPerLevel, nodes, m_root, recInfo);
		if (ok)
			dest << nodes;
		return ok;
	}
	
	///use this in concjunction with explicit instanzations of streaming ops as automaitc template deduction for nested classed doesn't seem to work
	static sserialize::UByteArrayAdapter & serialize(sserialize::UByteArrayAdapter & dest, const SerializationNode & src) {
		return src.serialize(dest);
	}
};

}//end namespace


//some (default streaming ops
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const typename sserialize::HuffmanTree<uint32_t>::SerializationNode & src) {
	return sserialize::HuffmanTree<uint32_t>::serialize(dest, src);
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const typename sserialize::HuffmanTree<uint64_t>::SerializationNode & src) {
	return sserialize::HuffmanTree<uint64_t>::serialize(dest, src);
}


#endif