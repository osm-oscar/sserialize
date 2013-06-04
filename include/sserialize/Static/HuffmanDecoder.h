#ifndef SSERIALIZE_CONTAINERS_HUFFMAN_DECODER_H
#define SSERIALIZE_CONTAINERS_HUFFMAN_DECODER_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/Deque.h>

/** This is a tablebased huffman decoder.
  * Essentialy its a tree with branching factor B and different child nodes
  *
  *
  * The root level uses fixed 10 Bits which results in 2 4K Pages
  * 
  * Branching factor should not exceed 2^12 as the child ptr encodes
  * the child id difference to the first child (12 Bits)
  * and the length of the huffcode (4 Bits )
  * a huff code length of 0 indicates a child node (otherwise childptr is not valid)
  *
  *Node Layout:
  *--------------------------------------------------------------
  *BranchingFactor|FirstChildId|value:childptr|
  *--------------------------------------------------------------
  *      uint8    | vl32       |(uint32:uint16)
  *
  *
  */


namespace sserialize {
namespace Static {

class HuffmanDecoder {
	class HuffmanCodePointInfo {
		uint32_t m_value;
		uint16_t m_childPtrBitLen;
	public:
		HuffmanCodePointInfo() :
		m_value(0), m_childPtrBitLen(0) {}
		
		HuffmanCodePointInfo(uint32_t value, uint16_t childPtrBitLen) :
		m_value(value), m_childPtrBitLen(childPtrBitLen) {}
		
		virtual ~HuffmanCodePointInfo() {}
		inline uint32_t value() const { return m_value; }
		inline uint8_t length() const { return m_childPtrBitLen & 0xF; }
		inline uint32_t childPtrDiff() const { return m_childPtrBitLen >> 4; }
		inline bool hasChild() { return m_childPtrBitLen & 0xF; }
	};

	class StaticNode {
		UByteArrayAdapter m_data;
		uint8_t m_bitLength;
		uint32_t m_initialChildPtr;
	public:
		StaticNode();
		StaticNode(const UByteArrayAdapter & data);
		virtual ~StaticNode();
		uint8_t bitLength() const { return m_bitLength;}
		uint32_t initialChildPtr() const { return m_initialChildPtr;}
		HuffmanCodePointInfo at(uint16_t pos) const;
	};
	
	Static::Deque<StaticNode> m_nodes;
	StaticNode m_root;
private:

	///selects the @length upper bits
	template<typename T_UINT_TYPE>
	static inline typename std::enable_if< std::is_unsigned<T_UINT_TYPE>::value, T_UINT_TYPE>::type selectBits(T_UINT_TYPE value, uint8_t length) {
		return (value >> (std::numeric_limits<T_UINT_TYPE>::digits-length));
	}

	template<typename T_UINT_TYPE>
	int decodeImp(T_UINT_TYPE src, uint32_t & decodedValue) const {
		uint8_t nodeBitLength = m_root.bitLength();;
		
		uint8_t decodedBitsCount = nodeBitLength;
		T_UINT_TYPE selectionBits = selectBits(src, nodeBitLength);
		src <<= nodeBitLength;
		
		HuffmanCodePointInfo cpInfo = m_root.at(selectionBits);

		if (!cpInfo.hasChild()) {
			decodedValue = cpInfo.value();
			return cpInfo.length();
		}
		
		StaticNode node = m_nodes.at(m_root.initialChildPtr() + cpInfo.childPtrDiff());
		while (decodedBitsCount < std::numeric_limits<T_UINT_TYPE>::digits) {
			nodeBitLength = node.bitLength();
			selectionBits = selectBits(src, nodeBitLength);
			src <<= nodeBitLength;
			decodedBitsCount += nodeBitLength;

			cpInfo = node.at(selectionBits);
			if (!cpInfo.hasChild()) {
				decodedValue = cpInfo.value();
				return cpInfo.length();
			}
			
			node = m_nodes.at(node.initialChildPtr() + cpInfo.childPtrDiff() );
		}
		return -1;
	}

public:
	HuffmanDecoder();
	HuffmanDecoder(const UByteArrayAdapter & data);
	virtual ~HuffmanDecoder();

	///@return on success  the bit length, on error -1
	inline int decode(uint16_t src, uint32_t & decodedValue) const {
		return decodeImp<uint16_t>(src, decodedValue);
	};
	
	///@return on success  the bit length, on error -1
	inline int decode(uint32_t src, uint32_t & decodedValue) const {
		return decodeImp<uint32_t>(src, decodedValue);
	};
	
	inline int decode(uint64_t src, uint32_t & decodedValue) const {
		return decodeImp<uint64_t>(src, decodedValue);
	}
};
}} //end namespace



#endif