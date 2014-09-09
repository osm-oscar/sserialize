#include <sserialize/Static/TrieNodePrivates/CompactTrieNodePrivate.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/find_key_in_array_functions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <iostream>
#include <stdio.h>

#include <sserialize/Static/TrieNode.h>

#define CHILDPTR_STRIPE_SIZE 32

namespace sserialize {
namespace Static {

CompactTrieNodePrivate::CompactTrieNodePrivate(const UByteArrayAdapter & nodeData) :
m_data(nodeData)
{
	UByteArrayAdapter current = m_data;
	m_nodeHeader = m_data.getUint16(0);
	uint8_t arrayInfoLen;

	if (m_nodeHeader & 0x2000) { //carry-bit is set
		m_childCount = (m_nodeHeader & 0xF) | ( static_cast<uint16_t>(m_data.getUint8(2)) << 4);
		arrayInfoLen = 3;
	}
	else {
		m_childCount = m_nodeHeader & 0xF;
		arrayInfoLen = 2;
	}
	m_strLen = current.at(arrayInfoLen);
	m_strStart = arrayInfoLen+1;

	
	uint32_t curOffSet = arrayInfoLen+1+m_strLen;
	
	if (m_childCount > 0) {
		m_childArrayStart = curOffSet;
		
		curOffSet += charWidth()*m_childCount;
		m_childPointerArrayStart = curOffSet;
		
		//correct offset for indexPtr
		curOffSet += (m_childCount+1)*2;
	}
	else {
		m_childArrayStart = 0;
		m_childPointerArrayStart = 0;
	}
	m_indexPtrStart = curOffSet;

	curOffSet += CompactUintArray::minStorageBytes(indexArrBpn(), popCount((unsigned int) indexTypes()));
	
	m_myEndPtr = curOffSet;
}

uint32_t CompactTrieNodePrivate::getChildPtrBeginOffset() const {
	return m_childPointerArrayStart;
}

uint32_t CompactTrieNodePrivate::childCharAt(uint32_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount) {
		pos = m_childCount-1;
	}
	if (charWidth() == 1) {
		return m_data.at(m_childArrayStart+pos);
	}
	else {
		return m_data.getUint16(m_childArrayStart+2*pos);
	}
}

uint32_t CompactTrieNodePrivate::getExactIndexPtr() const {
	if (hasExactIndex()) {
		CompactUintArray arr(m_data + m_indexPtrStart, indexArrBpn());
		return arr.at(0);
	}
	else {
		return 0;
	}
}

uint32_t CompactTrieNodePrivate::getPrefixIndexPtr() const {
	if (hasPrefixIndex()) {
		CompactUintArray arr(m_data + m_indexPtrStart, indexArrBpn());
		uint32_t offSet = popCount( (unsigned int)(indexTypes() & 0x1) );
		return arr.at(offSet);
	}
	else {
		return 0;
	}
}

uint32_t CompactTrieNodePrivate::getSuffixIndexPtr() const {
	if (hasSuffixIndex()) {
		CompactUintArray arr(m_data + m_indexPtrStart, indexArrBpn());
		uint32_t offSet = popCount( (unsigned int)(indexTypes() & 0x3) );
		return arr.at(offSet);
	}
	else {
		return 0;
	}
}

uint32_t CompactTrieNodePrivate::getSuffixPrefixIndexPtr() const {
	if (hasSuffixPrefixIndex()) {
		CompactUintArray arr(m_data + m_indexPtrStart, indexArrBpn());
		uint32_t offSet = popCount( (unsigned int)(indexTypes() & 0x7) );
		return arr.at(offSet);
	}
	else {
		return 0;
	}
}

uint32_t CompactTrieNodePrivate::getChildPtr(uint32_t pos) const {
	if (m_childCount == 0) return 0;
	if (pos >= m_childCount) pos = (m_childCount-1);
	//find our offset child and it's offset
	uint32_t baseOffset = m_data.getUint32(m_childPointerArrayStart);
	if (pos == 0)
		return baseOffset;
	for(unsigned int i = CHILDPTR_STRIPE_SIZE; i < pos; i+=CHILDPTR_STRIPE_SIZE) {
		baseOffset += m_data.getUint16(m_childPointerArrayStart+2+2*i); 
	}
	uint32_t newOf = m_data.getUint16(m_childPointerArrayStart+2+2*pos)+baseOffset; 
	return newOf;
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

int32_t CompactTrieNodePrivate::posOfChar(uint32_t key) const {
	uint8_t charWidth = this->charWidth();
	if (key > 0xFFFF || (charWidth == 1 && key > 0xFF))
		return -1;
	if (charWidth == 1) {
		return findKeyInArray<1>(m_data+m_childArrayStart, m_childCount, static_cast<uint8_t>(key));
	}
	else { //arrayCharSize==2
		return findKeyInArray_uint16<2>(m_data+m_childArrayStart, m_childCount, key);
	}
}

TrieNodePrivate* CompactTrieNodePrivate::childAt(uint32_t pos) const {
	if (pos >= m_childCount)
		pos = (m_childCount-1);
	return new CompactTrieNodePrivate(m_data + (m_myEndPtr + getChildPtr(pos)));
}

void CompactTrieNodePrivate::dump() const {
	std::cout << "[";
	std::cout << "m_nodeHeader=" << m_nodeHeader << ";";
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


uint32_t CompactTrieNodePrivate::getStorageSize() const {
	return m_myEndPtr;
}

uint32_t CompactTrieNodePrivate::getHeaderStorageSize() const {
	if (m_nodeHeader & 0x2000)
		return 3;
	else
		return 2;
}


uint32_t CompactTrieNodePrivate::getNodeStringStorageSize() const {
	return 1 + strLen();
}

uint32_t CompactTrieNodePrivate::getChildPtrStorageSize() const {
	return (m_childCount > 0 ? (m_childCount+1)*2 : 0);
}

uint32_t CompactTrieNodePrivate::getChildCharStorageSize() const {
	return charWidth()*m_childCount;
}

uint32_t CompactTrieNodePrivate::getIndexPtrStorageSize() const {
	return CompactUintArray::minStorageBytes(indexArrBpn(), popCount( (unsigned int)indexTypes() ) );
}

CompactStaticTrieCreationNode::CompactStaticTrieCreationNode(const UByteArrayAdapter & nodeData) : m_node(nodeData), m_data(nodeData) {};

bool CompactStaticTrieCreationNode::setChildPointer(uint32_t childNum, uint32_t offSetFromBeginning) {
	uint32_t childPtrStripeCount = CHILDPTR_STRIPE_SIZE; //TODO: paramtrise this

	if (childNum == 0) { //this is the first child
		m_data.putUint32(m_node.getChildPtrBeginOffset(), offSetFromBeginning);
	}
	else {
		//Get the baseOffset for our current node
		uint32_t summedNodeOffset = 0;
		uint32_t childPtrOffset = m_node.getChildPtrBeginOffset();
		for(unsigned int i = 0; i < childNum; i+=childPtrStripeCount) {
			if (i==0) {
				summedNodeOffset += m_data.getUint32(childPtrOffset);
			}
			else {
				summedNodeOffset += m_data.getUint16(childPtrOffset+2+2*i);
			}
		}
		uint32_t myOffset = offSetFromBeginning-summedNodeOffset;
		if (myOffset > 0xFFFF) { 
			std::cout << "FATAL: node pointer is too large" << std::endl;
			return false;
		}
		else {
			m_data.putUint16(childPtrOffset+2+2*childNum, myOffset);
		}
	}
	return true;
}

unsigned int
CompactStaticTrieCreationNode::createNewNode(
    const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {

	uint32_t putPtr = destination.tellPutPtr();

	//change this later if ptr len is not constant
	uint8_t childPtrLen = 2;

	uint32_t childrenCount = nodeInfo.childChars.size();

	if (childrenCount > 0xFFF) {
		std::cout << "Error: too many children" << std::endl;
		return CompactStaticTrieCreationNode::TOO_MANY_CHILDREN;
	}
	
	uint8_t charWidth = 1;
	if (nodeInfo.childChars.size() > 0) {
		charWidth = minStorageBytesOfValue(nodeInfo.childChars.back());
	}

	//Size of a character in the array length of the children array
	uint16_t nodeHeader = nodeInfo.indexTypes << 4;
	if (charWidth == 2)
		nodeHeader |= 0x8000;
	
	if (nodeInfo.mergeIndex)
		nodeHeader |= 0x4000;

	if (childrenCount > 0xF) {
		nodeHeader |= 0x2000;
		nodeHeader |= childrenCount & 0xF;
		destination.putUint16(nodeHeader);
		destination.putUint8((childrenCount >> 4) & 0xFF);
	}
	else {
		nodeHeader |= childrenCount;
		destination.putUint16(nodeHeader);
	}
	
	//Possibly push our node string
	if (nodeInfo.nodeStr.size() > 0xFF)
		return NODE_STRING_TOO_LONG;
	destination.putUint8(nodeInfo.nodeStr.size());
	if (nodeInfo.nodeStr.size() > 0) {
		destination.put(reinterpret_cast<const uint8_t*>(nodeInfo.nodeStr.c_str()), nodeInfo.nodeStr.size());
	}

	//Push the child chars with dummy child pointers
	if (childrenCount > 0) {

		if (charWidth == 2) {
			for(unsigned int i=0; i < childrenCount; i++) {
				destination.putUint16(nodeInfo.childChars[i]);
			}
		}
		else {
			for(unsigned int i=0; i < childrenCount; i++) {
				destination.putUint8(nodeInfo.childChars[i]);
			}
		}

		//dummy pointer values
		for(unsigned int i = 0; i < 4; i++)
			destination.putUint8(0); //push the first child
		for(unsigned int i=1; i < childrenCount; i++) {
			for(unsigned int j = 0; j < childPtrLen; j++)
				destination.putUint8(0);
		}
	}
	
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
	uint8_t bpn = 1;
	if (nodeInfo.indexTypes & TrieNodePrivate::IT_ALL) {
		bpn = CompactUintArray::create(idxPtrs, indexPtrsData);
	}
	if (!bpn) {
		return CompactStaticTrieCreationNode::INDEX_PTR_FAILED;
	}
	
	nodeHeader |= static_cast<uint16_t>(bpn-1) << 8;
	destination.putUint16(putPtr, nodeHeader);
	destination.put(indexPtrsData);

	return CompactStaticTrieCreationNode::NO_ERROR;
}

unsigned int CompactStaticTrieCreationNode::appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter & destination) {
	uint32_t putPtr = destination.tellPutPtr();
	unsigned int errors = CompactStaticTrieCreationNode::createNewNode(nodeInfo, destination);
	UByteArrayAdapter nodeStart = destination;
	nodeStart.setPutPtr(putPtr);
	if (nodeInfo.childChars.size() > 0) {
		CompactStaticTrieCreationNode creationNode(nodeStart);
		for(size_t i = 0; i < nodeInfo.childPtrs.size(); i++) {
			if (!creationNode.setChildPointer(i, nodeInfo.childPtrs[i])) {
				errors |= CHILD_PTR_FAILED;
				break;
			}
		}
	}
	return errors;
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
	if (error & CompactStaticTrieCreationNode::TOO_MANY_CHILDREN) {
		estr += "Too many children! ";
	}
	if (error & INDEX_PTR_FAILED) {
		estr += "Could not create index ptr set!";
	}
	return estr;
}

uint8_t CompactStaticTrieCreationNode::getType() {
	return TrieNode::T_COMPACT;
}

}}//end namespace