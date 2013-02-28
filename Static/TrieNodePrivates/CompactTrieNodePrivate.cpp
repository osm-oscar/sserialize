#include "CompactTrieNodePrivate.h"
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/find_key_in_array_functions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/vendor/libs/utf8/source/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <iostream>
#include <stdio.h>

#include "../TrieNode.h"

namespace sserialize {
namespace Static {

CompactTrieNodePrivate::CompactTrieNodePrivate(const UByteArrayAdapter & nodeData) :
m_data(nodeData)
{
	m_header = m_data.getUint32();
	m_childCount = m_header & HM_CHILD_COUNT;
	if ( !((m_header >> HS_CHILD_COUNT_FLAG) & HM_CHILD_COUNT_FLAG) ) {
		m_childCount >>= 8;
		m_data.decGetPtr(1);
	}
	
	
	m_strLen = m_data.getUint8();
	if (m_childCount > 0) {
		m_childPtrBeginOffset = m_data.getVlPackedUint32();
		if (m_childCount > 2)
			m_childPtrDiff = m_data.getVlPackedUint32();
	}

	m_strStart = m_data.tellGetPtr();
	m_data.incGetPtr(m_strLen);

	m_childArrayStart = m_data.tellPutPtr();
	m_data.incGetPtr( sserialize::CompactUintArray::minStorageBytes(charWidth(), childCount()) );
	
	m_childPointerArrayStart = m_data.tellGetPtr();
	if (childCount() > 1)
		m_data.incGetPtr( sserialize::CompactUintArray::minStorageBytes(childPtrBits(), childCount()-1) );
	m_indexPtrStart = m_data.tellGetPtr();
	
	if (indexTypes())
		m_data.incGetPtr( sserialize::CompactUintArray::minStorageBytes(indexPtrBits(), popCount(indexTypes())) );
	
	m_myEndPtr = m_data.tellGetPtr();
}

uint32_t CompactTrieNodePrivate::getChildPtrBeginOffset() const {
	return m_childPointerArrayStart;
}

uint8_t CompactTrieNodePrivate::charWidth() const {
	return (m_header >> HS_CHILD_CHAR_BITS) &  HM_CHILD_CHAR_BITS;
}

uint8_t CompactTrieNodePrivate::childPtrBits() const {
	return (m_header >> HS_CHILD_PTR_BITS) & HM_CHILD_PTR_BITS;
}

bool CompactTrieNodePrivate::hasMergeIndex() const {
	return (m_header >> HS_MERGE_INDEX) & HM_MERGE_INDEX;
}

uint8_t CompactTrieNodePrivate::indexPtrBits() const {
	return (m_header >> HS_INDEX_BITS) & HM_INDEX_BITS;
}

TrieNodePrivate::IndexTypes CompactTrieNodePrivate::indexTypes() const {
	return (IndexTypes) ((m_header >> HS_INDEX_TYPES) & HM_INDEX_TYPES);
}


uint32_t CompactTrieNodePrivate::childCharAt(uint16_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount) {
		pos = m_childCount-1;
	}
	CompactUintArray arr(m_data + m_childArrayStart, charWidth());
	return arr.at(pos);
}

uint32_t CompactTrieNodePrivate::getExactIndexPtr() const {
	CompactUintArray arr(m_data + m_indexPtrStart, indexPtrBits());
	return arr.at(0);
}

uint32_t CompactTrieNodePrivate::getPrefixIndexPtr() const {
	CompactUintArray arr(m_data + m_indexPtrStart, indexPtrBits());
	uint32_t offSet = popCount( (indexTypes() & 0x1) );
	return arr.at(offSet);
}

uint32_t CompactTrieNodePrivate::getSuffixIndexPtr() const {
	CompactUintArray arr(m_data + m_indexPtrStart, indexPtrBits());
	uint32_t offSet = popCount( (indexTypes() & 0x3) );
	return arr.at(offSet);
}

uint32_t CompactTrieNodePrivate::getSuffixPrefixIndexPtr() const {
	CompactUintArray arr(m_data + m_indexPtrStart, indexPtrBits());
	uint32_t offSet = popCount( (indexTypes() & 0x7) );
	return arr.at(offSet);
}

uint32_t CompactTrieNodePrivate::getChildPtr(uint32_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount)
		pos = (m_childCount-1);
	if (pos == 0)
		return m_childPtrBeginOffset;
	CompactUintArray arr(m_data + m_childPointerArrayStart, childPtrBits());
	return m_childPtrBeginOffset + arr.at(pos-1) + m_childPtrDiff*pos;
}

UByteArrayAdapter CompactTrieNodePrivate::strData() const {
	return UByteArrayAdapter(m_data, m_strStart, m_strLen);
}

std::string CompactTrieNodePrivate::str() const {
	std::string st;
	for(int i = 0; i < m_strLen; i++)
		st.append(1, static_cast<char>(m_data.at(m_strStart+i)));
	return st;
}

int16_t CompactTrieNodePrivate::posOfChar(uint32_t key) const {
	CompactUintArray arr(m_data+m_childArrayStart, charWidth());
	return arr.findSorted(key, childCount());
}

TrieNodePrivate* CompactTrieNodePrivate::childAt(uint16_t pos) const {
	if (pos >= m_childCount)
		pos = (m_childCount-1);
	return new CompactTrieNodePrivate(m_data + (m_myEndPtr + getChildPtr(pos)));
}

void CompactTrieNodePrivate::dump() const {
	std::cout << "[";
	std::cout << "m_current[0]=" << static_cast<int>(m_data.at(0)) << ";";
	std::cout << "m_charWidth=" << static_cast<int>(charWidth()) << "; ";
	std::cout << "m_childCount=" << static_cast<int>(m_childCount) << "; ";
	std::cout << "m_strLen=" << static_cast<int>(m_strLen) << "; ";
	std::cout << "str=" << str() << ";";
	std::cout << "exactIndexPtr=" << getExactIndexPtr() << ";";
	std::cout << "prefixIndexPtr=" << getPrefixIndexPtr() << ";";
	std::cout << "suffixIndexPtr=" << getSuffixIndexPtr() << ";";
	std::cout << "suffixPrefixIndexPtr=" << getSuffixPrefixIndexPtr() << ";";
	std::cout << "childrenChars=(";
	std::string childCharStr;
	for(uint16_t i = 0; i < m_childCount; i++) {
		utf8::append(childCharAt(i), std::back_inserter(childCharStr));
		childCharStr += ", ";
	}
	std::cout << childCharStr << ")]" << std::endl << std::flush;
}


uint32_t CompactTrieNodePrivate::getStorageSize() const {
	return m_myEndPtr;
}

uint32_t CompactTrieNodePrivate::getHeaderStorageSize() const {
	if ((m_header >> HS_CHILD_COUNT_FLAG) & HM_CHILD_COUNT_FLAG)
		return 4;
	return 3;
}


uint32_t CompactTrieNodePrivate::getNodeStringStorageSize() const {
	return 1 + strLen();
}

uint32_t CompactTrieNodePrivate::getChildPtrStorageSize() const {
	uint32_t size = vl_pack_uint32_t_size(m_childPtrBeginOffset);
	size += vl_pack_uint32_t_size(m_childPtrDiff);
	if (childCount() > 1)
		size += CompactUintArray::minStorageBytes(childPtrBits(), childCount());
	return size;
}

uint32_t CompactTrieNodePrivate::getChildCharStorageSize() const {
	return CompactUintArray::minStorageBytes(charWidth(), childCount());
}

uint32_t CompactTrieNodePrivate::getIndexPtrStorageSize() const {
	return CompactUintArray::minStorageBytes(indexPtrBits(), popCount( indexTypes() ) );
}

CompactStaticTrieCreationNode::CompactStaticTrieCreationNode(const UByteArrayAdapter & nodeData) : m_node(nodeData), m_data(nodeData) {};


uint32_t smallestGap(const std::vector<uint32_t> & src) {
	if (src.size() < 2)
		return 0;
	uint32_t o = std::numeric_limits<uint32_t>::max();
	std::vector<uint32_t>::const_iterator end(src.end());
	std::vector<uint32_t>::const_iterator prev(src.begin());
	std::vector<uint32_t>::const_iterator ahead(prev+1);
	for(; ahead != end; ++ahead, ++prev) {
		o = std::min<uint32_t>(*ahead-*prev, o);
	}
	return o;
}

uint32_t largestOffset(const std::vector<uint32_t> & src, uint32_t gap) {
	if (src.size() < 2)
		return 0;
	uint32_t first = *src.begin();
	uint32_t o = 0;
	std::vector<uint32_t>::const_iterator end(src.end());
	std::vector<uint32_t>::const_iterator it(src.begin()+1);
	for(uint32_t cg = gap; it != end; ++it, cg += gap) {
		o = std::max<uint32_t>((*it-first)- cg, o);
	}
	return o;
}


unsigned int
CompactStaticTrieCreationNode::createNewNode(
    const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {
    uint32_t putPtr = destination.tellPutPtr();
    
	uint32_t header = 0;
	
	uint32_t charBits = CompactUintArray::minStorageBits(nodeInfo.childChars.back());
	
	uint32_t ptrGap = smallestGap(nodeInfo.childPtrs);
	uint32_t ptrOffset = 0;
	if (nodeInfo.childrenCount > 2) {
		ptrOffset = largestOffset(nodeInfo.childPtrs, ptrGap);
	}
	else if (nodeInfo.childrenCount == 2) {
		ptrOffset = nodeInfo.childPtrs.back() -  nodeInfo.childPtrs.front();
	}
	uint32_t ptrBits = CompactUintArray::minStorageBits(ptrOffset);
	
	uint32_t mergeIndex = (nodeInfo.mergeIndex ? 0x1 : 0x0);
	
	//Push index pointers
	std::deque<uint32_t> idxPtrs;
	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_EXACT) {
		idxPtrs.push_back(nodeInfo.exactIndexPtr);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_PREFIX) {
		idxPtrs.push_back(nodeInfo.prefixIndexPtr);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX) {
		idxPtrs.push_back(nodeInfo.suffixIndexPtr);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX_PREFIX) {
		idxPtrs.push_back(nodeInfo.suffixPrefixIndexPtr);
	}

	std::deque<uint8_t> indexPtrsData;
	uint8_t indexBits = CompactUintArray::createFromDeque(idxPtrs, indexPtrsData);
	
	if (!indexBits) {
		return CompactStaticTrieCreationNode::INDEX_PTR_FAILED;
	}
	
	uint32_t childCountFlag = (nodeInfo.childrenCount > 0x7 ? 0x1 : 0x0);
	uint32_t childCount = (childCountFlag ?  nodeInfo.childrenCount : (nodeInfo.childrenCount << 8));

	//assemble header
	header |= charBits << CompactTrieNodePrivate::HS_CHILD_CHAR_BITS;
	header |= ptrBits << CompactTrieNodePrivate::HS_CHILD_PTR_BITS;
	header |= indexBits << CompactTrieNodePrivate::HS_INDEX_BITS;
	header |= mergeIndex << CompactTrieNodePrivate::HS_MERGE_INDEX;
	header |= nodeInfo.indexTypes << CompactTrieNodePrivate::HS_INDEX_TYPES;
	header |= childCountFlag << CompactTrieNodePrivate::HS_CHILD_COUNT_FLAG;
	header |= (nodeInfo.childrenCount << (childCountFlag ? 0 : 8) );

	//header is completeley assembled
	if (childCountFlag) {
		destination.putUint32(header);
	}
	else {
		destination.putUint24(header >> 8);
	}
	
	//put the dummy string length
	uint8_t & nodeStrLenDestination = destination[destination.tellPutPtr()];
	destination.putUint8(0);
	
	//first child ptr
	
	uint32_t childPtrDiff = 0;
	if (nodeInfo.childrenCount) {
		destination.putVlPackedUint32(nodeInfo.childPtrs.front());
		if (nodeInfo.childrenCount > 2) {
			childPtrDiff = smallestGap(nodeInfo.childPtrs);
			
		}
	}

	
	//Possibly push our node string
	
	if (nodeInfo.nodeStr.size() > 0) {
		//push the string with string length
		std::string::const_iterator nodeStrIt = nodeInfo.nodeStr.begin();
		utf8::next(nodeStrIt, nodeInfo.nodeStr.end()); //remove the first char as that one is already in our parent
		std::string stnodeStr(nodeStrIt, nodeInfo.nodeStr.end());
		
		if (stnodeStr.size() <= 0xFF) {
			nodeStrLenDestination = stnodeStr.size();
		}
		else {
			std::cout << "node string is too long: " << nodeInfo.nodeStr << std::endl;
			return CompactStaticTrieCreationNode::NODE_STRING_TOO_LONG;
		}
		//Node string (not null terminated)

		for(std::string::iterator i = stnodeStr.begin(); i != stnodeStr.end(); i++) {
			destination.putUint8(static_cast<uint8_t>(*i));
		}
	}



	return CompactStaticTrieCreationNode::NO_ERROR;
}

unsigned int CompactStaticTrieCreationNode::appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {
	return CompactStaticTrieCreationNode::createNewNode(nodeInfo, destination);
}

unsigned int
CompactStaticTrieCreationNode::
prependNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, std::deque< uint8_t >& destination)
{
	std::vector< uint8_t > tmpDest;
	UByteArrayAdapter tmpDestAdap(&tmpDest);
	unsigned int err = CompactStaticTrieCreationNode::appendNewNode(nodeInfo, tmpDestAdap);
	prependToDeque(tmpDest, destination);
	return err;
}

bool CompactStaticTrieCreationNode::isError(unsigned int error) {
	return (error !=  CompactStaticTrieCreationNode::NO_ERROR);
}

std::string CompactStaticTrieCreationNode::errorString(unsigned int error) {
	std::string estr = "";
	if (error & CompactStaticTrieCreationNode::TOO_MANY_CHILDREN) estr += "Too many children! ";
	if (error & INDEX_PTR_FAILED) estr += "Could not create index ptr set!";
	return estr;
}

uint8_t CompactStaticTrieCreationNode::getType() {
	return TrieNode::T_COMPACT;
}

}}//end namespace