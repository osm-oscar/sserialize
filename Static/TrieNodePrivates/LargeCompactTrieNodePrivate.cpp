#include <sserialize/Static/TrieNodePrivates/LargeCompactTrieNodePrivate.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/find_key_in_array_functions.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/exceptions.h>
#include <iostream>
#include <stdio.h>

#include <sserialize/Static/TrieNode.h>

#define CHILDPTR_STRIPE_SIZE 32

namespace sserialize {
namespace Static {

LargeCompactTrieNodePrivate::LargeCompactTrieNodePrivate(const UByteArrayAdapter & nodeData) :
m_data(nodeData),
m_exactIndexPtr(0),
m_prefixIndexPtr(0),
m_suffixIndexPtr(0),
m_substrIndexPtr(0)
{
	m_header = m_data.getUint16();
	m_childCount = m_header & 0x7;
	if (m_header & 0x8) {
		m_childCount |= m_data.getVlPackedUint32() << 3;
	}
	m_header >>= 4;
	//header extracted
	
	//get the string length
	m_strLen = m_data.getUint8();
	m_strBegin = m_data.tellGetPtr();
	m_data.incGetPtr(m_strLen);

	//skip the pointers
	m_childArrayStart = m_data.tellGetPtr();
	m_data.incGetPtr(charWidth()*childCount());
	
	//child ptr stuff
	if (childCount()) {
		m_childPtrBaseOffset = m_data.getVlPackedUint32();
		m_childPointerArrayStart = m_data.tellGetPtr();
		if (childCount()) {
			m_data.incGetPtr( CompactUintArray::minStorageBytes(childPtrBits(), childCount()-1) );
		}
	}
	
	//indexptr stuff
	if (hasAnyIndex()) {
		uint32_t indexPtrBaseOffset = m_data.getVlPackedUint32();
		if (hasExactIndex())
			m_exactIndexPtr = indexPtrBaseOffset + m_data.getVlPackedUint32();
		if (hasPrefixIndex())
			m_prefixIndexPtr = indexPtrBaseOffset + m_data.getVlPackedUint32();
		if (hasSuffixIndex())
			m_suffixIndexPtr = indexPtrBaseOffset + m_data.getVlPackedUint32();
		if (hasSuffixPrefixIndex())
			m_substrIndexPtr = indexPtrBaseOffset + m_data.getVlPackedUint32();
	}
	m_myEndPtr = m_data.tellGetPtr();
}

uint32_t LargeCompactTrieNodePrivate::getChildPtrBeginOffset() const {
	return m_childPointerArrayStart;
}

uint32_t LargeCompactTrieNodePrivate::childCharAt(uint32_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount) {
		pos = m_childCount-1;
	}
	
	uint8_t cw = charWidth();
	
	if (cw == 1) {
		return m_data.at(m_childArrayStart+pos);
	}
	else if (cw == 2) {
		return m_data.getUint16(m_childArrayStart+2*pos);
	}
	else if (cw == 3) {
		return m_data.getUint24(m_childArrayStart+3*pos);
	}
	else {
		return m_data.getUint32(m_childArrayStart+4*pos);
	}
}

uint32_t LargeCompactTrieNodePrivate::getExactIndexPtr() const {
	return m_exactIndexPtr;
}

uint32_t LargeCompactTrieNodePrivate::getPrefixIndexPtr() const {
	return m_prefixIndexPtr;
}

uint32_t LargeCompactTrieNodePrivate::getSuffixIndexPtr() const {
	return m_suffixIndexPtr;
}

uint32_t LargeCompactTrieNodePrivate::getSuffixPrefixIndexPtr() const {
	return m_substrIndexPtr;
}

uint32_t LargeCompactTrieNodePrivate::getChildPtr(uint32_t pos) const {
	if (m_childCount == 0 || pos >= m_childCount) {
		throw sserialize::OutOfBoundsException("LargeCompactTrieNodePrivate::getChildPtr");
	}
	if (pos == 0) {
		return m_childPtrBaseOffset;
	}
	
	CompactUintArray cps(m_data+m_childPointerArrayStart, childPtrBits());
	uint32_t baseOffset = m_childPtrBaseOffset;
	for(uint32_t i = CHILDPTR_STRIPE_SIZE; i < pos; i += CHILDPTR_STRIPE_SIZE) {
		baseOffset += cps.at(i-1); 
	}
	uint32_t newOf = cps.at(pos-1)+baseOffset; 
	return newOf;
}

UByteArrayAdapter LargeCompactTrieNodePrivate::strData() const {
	return UByteArrayAdapter(m_data, m_strBegin, m_strLen);
}

std::string LargeCompactTrieNodePrivate::str() const {
	uint8_t * str = new uint8_t[m_strLen];
	m_data.getData(m_strBegin, str, m_strLen);
	std::string st(reinterpret_cast<char*>(str), m_strLen);
	delete str;
	return st;
}

int32_t LargeCompactTrieNodePrivate::posOfChar(uint32_t key) const {
	uint8_t charWidth = this->charWidth();
	if (key > 0xFFFF || (charWidth == 1 && key > 0xFF))
		return -1;
	if (charWidth == 1) {
		return findKeyInArray<1>(m_data+m_childArrayStart, m_childCount, static_cast<uint8_t>(key));
	}
	else if (charWidth == 2) { //arrayCharSize==2
		return findKeyInArray_uint16<2>(m_data+m_childArrayStart, m_childCount, key);
	}
	else if (charWidth == 3) { //arrayCharSize==3
		return findKeyInArray_uint24<3>(m_data+m_childArrayStart, m_childCount, key);
	}
	else if (charWidth == 4) { //arrayCharSize==4
		return findKeyInArray_uint32<4>(m_data+m_childArrayStart, m_childCount, key);
	}
	else {
		throw sserialize::CorruptDataException("LargeCompactTrieNodePrivate::posOfChar has invalid charWidth");
	}
}

TrieNodePrivate* LargeCompactTrieNodePrivate::childAt(uint32_t pos) const {
	if (pos >= m_childCount)
		throw sserialize::OutOfBoundsException("LargeCompactTrieNodePrivate::childAt");
	return new LargeCompactTrieNodePrivate(m_data + (m_myEndPtr + getChildPtr(pos)));
}

void LargeCompactTrieNodePrivate::dump() const {
	std::cout << "[";
	std::cout << "m_current[0]=" << static_cast<int>(m_data.at(0)) << ";";
	std::cout << "m_charWidth=" << static_cast<int>(charWidth()) << "; ";
	std::cout << "m_childCount=" << static_cast<int>(m_childCount) << "; ";
	std::cout << "m_strLen=" << static_cast<int>(m_strLen) << "; ";
	std::cout << "str=" << str() << ";";
	if(hasExactIndex())
		std::cout << "exactIndexPtr=" << getExactIndexPtr() << ";";
	if (hasPrefixIndex())
		std::cout << "prefixIndexPtr=" << getPrefixIndexPtr() << ";";
	if(hasSuffixIndex())
		std::cout << "suffixIndexPtr=" << getSuffixIndexPtr() << ";";
	if(hasSuffixPrefixIndex())
		std::cout << "suffixPrefixIndexPtr=" << getSuffixPrefixIndexPtr() << ";";
	std::cout << "childrenChars=(";
	std::string childCharStr;
	for(uint16_t i = 0; i < m_childCount; i++) {
		utf8::append(childCharAt(i), std::back_inserter(childCharStr));
		childCharStr += ", ";
	}
	std::cout << childCharStr << ")]" << std::endl << std::flush;
}


uint32_t LargeCompactTrieNodePrivate::getStorageSize() const {
	return m_myEndPtr;
}

uint32_t LargeCompactTrieNodePrivate::getHeaderStorageSize() const {
	return 2 + psize_vu32(m_childCount >> 3);
}

uint32_t LargeCompactTrieNodePrivate::getNodeStringStorageSize() const {
	return 1 + strLen();
}

uint32_t LargeCompactTrieNodePrivate::getChildPtrStorageSize() const {
	if (childCount()) {
		uint32_t s = psize_vu32(m_childPtrBaseOffset);
		if (childCount() > 1)
			s += CompactUintArray::minStorageBytes(childPtrBits(), childCount()-1);
		return s;
	}
	return 0;
}

uint32_t LargeCompactTrieNodePrivate::getChildCharStorageSize() const {
	return charWidth()*m_childCount;
}

uint32_t LargeCompactTrieNodePrivate::getIndexPtrStorageSize() const {
	return getStorageSize()-(getHeaderStorageSize()+getNodeStringStorageSize()+getChildPtrStorageSize()+getChildCharStorageSize());
}

unsigned int
LargeCompactTrieNodeCreator::createNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {
	UByteArrayAdapter::OffsetType destBegin = destination.tellPutPtr();

	uint8_t charWidth = 1;
	if (nodeInfo.childChars.size() > 0) {
		charWidth = CompactUintArray::minStorageBitsFullBytes(nodeInfo.childChars.back());
	}
	uint16_t header = charWidth-1;
	header <<= 1; //charWidth
	if (nodeInfo.mergeIndex)
		header |= 0x1;
	header <<= 5; //childPtrBits
	header <<= 4; //indexTypes
	header |= nodeInfo.indexTypes & 0xF;
	header <<= 4; //childrenCount
	header |= nodeInfo.childChars.size() & 0x7;
	if (nodeInfo.childChars.size() > 0x7) {
		header |= 0x8;
	}
	
	destination.putUint16(header);
	if (nodeInfo.childChars.size() > 0x7) {
		destination.putVlPackedUint32(nodeInfo.childChars.size() >> 3);
	}
	
	//node string
	if (nodeInfo.nodeStr.size() > 0xFF)
		return NODE_STRING_TOO_LONG;
	destination.putUint8(nodeInfo.nodeStr.size());
	if (nodeInfo.nodeStr.size() > 0) {
		destination.putData(reinterpret_cast<const uint8_t*>(nodeInfo.nodeStr.c_str()), nodeInfo.nodeStr.size());
	}
	
	if (nodeInfo.childChars.size()) {
		if (charWidth == 1) {
			for(std::vector<uint32_t>::const_iterator it(nodeInfo.childChars.begin()), end(nodeInfo.childChars.end()); it != end; ++it) {
				destination.putUint8(*it);
			}
		}
		else if (charWidth == 2) {
			for(std::vector<uint32_t>::const_iterator it(nodeInfo.childChars.begin()), end(nodeInfo.childChars.end()); it != end; ++it) {
				destination.putUint16(*it);
			}
		}
		else if (charWidth == 3) {
			for(std::vector<uint32_t>::const_iterator it(nodeInfo.childChars.begin()), end(nodeInfo.childChars.end()); it != end; ++it) {
				destination.putUint24(*it);
			}
		}
		else if (charWidth == 4) {
			for(std::vector<uint32_t>::const_iterator it(nodeInfo.childChars.begin()), end(nodeInfo.childChars.end()); it != end; ++it) {
				destination.putUint32(*it);
			}
		}
		else {
			return CHILD_CHAR_FAILED;
		}

		destination.putVlPackedUint32(nodeInfo.childPtrs[0]);
		uint8_t childPtrBits = 0;
		if (nodeInfo.childChars.size() > 1) {
			std::vector<uint32_t> childPtrs = nodeInfo.childPtrs;
			uint32_t curChildPtrOffset = childPtrs[0];
			childPtrs[0] = 0;
			for(uint32_t i = 1, s = childPtrs.size(); i < s; ++i) {
				childPtrs[i] -= curChildPtrOffset;
				if (i % CHILDPTR_STRIPE_SIZE == 0) {
					curChildPtrOffset += childPtrs[i];
				}
			}
			uint32_t maxOffset = 0;
			for(std::vector<uint32_t>::const_iterator it(childPtrs.begin()), end(childPtrs.end()); it != end; ++it) {
				maxOffset = std::max<uint32_t>(maxOffset, *it);
			}
			
			childPtrBits = CompactUintArray::minStorageBits(maxOffset);
			UByteArrayAdapter tempD(new std::vector<uint8_t>(CompactUintArray::minStorageBytes(childPtrBits, nodeInfo.childChars.size()-1)), true);
			CompactUintArray carr(tempD, childPtrBits);
			for(uint32_t i = 1, s = childPtrs.size(); i < s; ++i) {
				carr.set(i-1, childPtrs[i]);
			}
			destination.putData(carr.data());
			header |= (static_cast<uint16_t>(childPtrBits-1) << 8);
		}
		
		destination.putUint16(destBegin, header);
	}
	if (nodeInfo.indexTypes & TrieNodePrivate::IT_ALL) {
		uint32_t idxBaseOffset = std::min( std::min(nodeInfo.exactIndexPtr, nodeInfo.prefixIndexPtr), std::min(nodeInfo.suffixIndexPtr, nodeInfo.suffixPrefixIndexPtr) );
		destination.putVlPackedUint32(idxBaseOffset);
		if (nodeInfo.indexTypes & TrieNodePrivate::IT_EXACT)
			destination.putVlPackedUint32(nodeInfo.exactIndexPtr - idxBaseOffset);
		if (nodeInfo.indexTypes & TrieNodePrivate::IT_PREFIX)
			destination.putVlPackedUint32(nodeInfo.prefixIndexPtr - idxBaseOffset);
		if (nodeInfo.indexTypes & TrieNodePrivate::IT_SUFFIX)
			destination.putVlPackedUint32(nodeInfo.suffixIndexPtr - idxBaseOffset);
		if (nodeInfo.indexTypes & TrieNodePrivate::IT_SUFFIX_PREFIX)
			destination.putVlPackedUint32(nodeInfo.suffixPrefixIndexPtr - idxBaseOffset);
	}
	return LargeCompactTrieNodeCreator::NO_ERROR;
}

unsigned int LargeCompactTrieNodeCreator::appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {
	return LargeCompactTrieNodeCreator::createNewNode(nodeInfo, destination);
}

unsigned int
LargeCompactTrieNodeCreator::
prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque< uint8_t >& destination)
{
	std::vector< uint8_t > tmpDest;
	UByteArrayAdapter tmpDestAdap(&tmpDest);
	unsigned int err = LargeCompactTrieNodeCreator::appendNewNode(nodeInfo, tmpDestAdap);
	prependToDeque(tmpDest, destination);
	return err;
}

bool LargeCompactTrieNodeCreator::isError(unsigned int error) {
	return (error !=  LargeCompactTrieNodeCreator::NO_ERROR);
}

std::string LargeCompactTrieNodeCreator::errorString(unsigned int error) {
	std::string estr = "";
	if (error & LargeCompactTrieNodeCreator::TOO_MANY_CHILDREN) {
		estr += "Too many children! ";
	}
	if (error & INDEX_PTR_FAILED) {
		estr += "Could not create index ptr set!";
	}
	return estr;
}

uint8_t LargeCompactTrieNodeCreator::getType() {
	return TrieNode::T_LARGE_COMPACT;
}

}}//end namespace