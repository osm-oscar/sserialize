#ifndef SSERIALIZE_CONTAINERS_HUFFMAN_DECODER_H
#define SSERIALIZE_CONTAINERS_HUFFMAN_DECODER_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/Deque.h>
#include <sserialize/utility/SerializationInfo.h>

/** This is a tablebased huffman decoder.
  * Essentialy its a tree with branching factor B and different child nodes
  *
  *
  * 
  * Branching factor should not exceed 2^19 as the child ptr encodes
  * the child id difference to the first child (18 Bits)
  * and the length of the huffcode (5 Bits )
  * a huff code length of 0 indicates a child node (otherwise childptr is not valid)
  *
  *Node Layout:
  *--------------------------------------------------------------
  *BranchingFactor|FirstChildId|value:childptr|
  *--------------------------------------------------------------
  *      uint8    | vl32       |(uint32:uint32)
  *
  *
  */


namespace sserialize {
namespace Static {

class HuffmanDecoder {
public:
	class HuffmanCodePointInfo {
		uint32_t m_value;
		uint32_t m_childPtrBitLen;
	public:
		HuffmanCodePointInfo() :
		m_value(0), m_childPtrBitLen(0) {}
		
		HuffmanCodePointInfo(uint32_t value, uint32_t childPtrBitLen) :
		m_value(value), m_childPtrBitLen(childPtrBitLen) {}
		
		virtual ~HuffmanCodePointInfo() {}
		inline uint32_t value() const { return m_value; }
		inline uint8_t length() const { return m_childPtrBitLen & 0x1F; }
		inline uint32_t childPtrDiff() const { return m_childPtrBitLen >> 5; }
		inline bool hasChild() { return !(m_childPtrBitLen & 0x1F); }
	};
private:
	class StaticNode {
		UByteArrayAdapter m_data;
		uint8_t m_bitLength;
		uint32_t m_initialChildPtr;
	public:
		StaticNode();
		StaticNode(const UByteArrayAdapter & data);
		virtual ~StaticNode();
		void readInCache() const;
		uint32_t entryCount() const { return static_cast<uint32_t>(1) << bitLength(); }
		uint8_t bitLength() const { return m_bitLength;}
		uint32_t initialChildPtr() const { return m_initialChildPtr;}
		HuffmanCodePointInfo at(uint32_t pos) const;
		bool valid() const { return m_data.size(); }
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
		uint8_t nodeBitLength = m_root.bitLength();
		
		uint8_t decodedBitsCount = nodeBitLength;
		T_UINT_TYPE selectionBits = selectBits(src, nodeBitLength);
		src <<= nodeBitLength;
		
		HuffmanCodePointInfo cpInfo = m_root.at(selectionBits);

		if (!cpInfo.hasChild()) { //we definetly end here (otheriwse the data is corrupt)
			decodedValue = cpInfo.value();
			return cpInfo.length();
		}
		
		StaticNode node = m_nodes.at(m_root.initialChildPtr() + cpInfo.childPtrDiff());
		while (decodedBitsCount < std::numeric_limits<T_UINT_TYPE>::digits) {
			nodeBitLength = node.bitLength();
			selectionBits = selectBits(src, nodeBitLength);
			src <<= nodeBitLength;

			cpInfo = node.at(selectionBits);
			if (!cpInfo.hasChild()) {
				decodedValue = cpInfo.value();
				return cpInfo.length() + decodedBitsCount;
			}
			decodedBitsCount += nodeBitLength;
			node = m_nodes.at(node.initialChildPtr() + cpInfo.childPtrDiff() );
			if (!node.valid())
				throw sserialize::CorruptDataException("sserialize::HuffmanDecoder::decodeImp");
		}
		return -1;
	}

public:
	HuffmanDecoder();
	HuffmanDecoder(const UByteArrayAdapter & data);
	virtual ~HuffmanDecoder();
	void readInCache();
	UByteArrayAdapter::OffsetType getSizeInBytes() const { return m_nodes.getSizeInBytes(); }
	
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

namespace sserialize {
	template<> inline bool SerializationInfo<sserialize::Static::HuffmanDecoder::HuffmanCodePointInfo>::is_fixed_length() { return true; }
	template<> inline OffsetType SerializationInfo<sserialize::Static::HuffmanDecoder::HuffmanCodePointInfo>::length() { return 8; }
	template<> inline OffsetType SerializationInfo<sserialize::Static::HuffmanDecoder::HuffmanCodePointInfo>::max_length() { return 8; }
	template<> inline OffsetType SerializationInfo<sserialize::Static::HuffmanDecoder::HuffmanCodePointInfo>::min_length() { return 8; }
}


#endif