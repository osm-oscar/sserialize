#include <sserialize/Static/TrieNodePrivates/SimpleTrieNodePrivate.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/find_key_in_array_functions.h>
#include <sserialize/utility/unicode_case_functions.h>
#include <sserialize/vendor/utf8.h>
#include <sserialize/utility/utilfuncs.h>
#include <iostream>
#include <sserialize/Static/TrieNode.h>
#include <sserialize/containers/CompactUintArray.h>

#define CHILDPTR_STRIPE_SIZE 32

namespace sserialize {
namespace Static {


SimpleTrieNodePrivate::SimpleTrieNodePrivate(const UByteArrayAdapter & nodeData)  : m_data(nodeData) {
	uint32_t curOffSet = 0;
	
	m_childCount = m_data.getUint16(curOffSet);
	curOffSet += 2;

	m_charWidth = m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_indexTypes = (IndexTypes) m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_strLen = m_data.getUint8(curOffSet);
	curOffSet += 1;

	m_strStart = curOffSet;
	curOffSet += m_strLen;

	m_childArrayStart = curOffSet;
	curOffSet += m_childCount*4; //charwidth is  always 4

	m_childPointerArrayStart = curOffSet;
	curOffSet += m_childCount*4;

	m_indexPtrStart = curOffSet;
	curOffSet += 16;
	
	m_myEndPtr = curOffSet;
}

uint32_t SimpleTrieNodePrivate::getChildPtrBeginOffset() const {
	return m_childPointerArrayStart;
}

uint32_t SimpleTrieNodePrivate::childCharAt(uint32_t pos) const {
	if (m_childCount == 0)
		return 0;
	if (pos >= m_childCount) {
		pos = m_childCount-1;
	}
	return m_data.getUint32(m_childArrayStart+4*pos);
}

uint32_t SimpleTrieNodePrivate::getExactIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart);
	return r;
}

uint32_t SimpleTrieNodePrivate::getPrefixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+4);
	return r;
}


uint32_t SimpleTrieNodePrivate::getSuffixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+8);
	return r;
}

uint32_t SimpleTrieNodePrivate::getSuffixPrefixIndexPtr() const {
	uint32_t r = m_data.getUint32(m_indexPtrStart+12);
	return r;
}

uint32_t SimpleTrieNodePrivate::getChildPtr(uint32_t pos) const {
	return m_data.getUint32(m_childPointerArrayStart+4*pos);
}

UByteArrayAdapter SimpleTrieNodePrivate::strData() const {
	return UByteArrayAdapter(m_data, m_strStart, m_strLen);
}


std::string SimpleTrieNodePrivate::str() const {
	std::string st;
	for(int i = 0; i < m_strLen; i++)
		st.append(1, static_cast<char>(m_data.at(m_strStart+i)));
	return st;
}

int32_t SimpleTrieNodePrivate::posOfChar(uint32_t ucode) const {
	if (ucode > 0xFFFF || (m_charWidth == 1 && ucode > 0xFF))
		return -1;
	return findKeyInArray_uint32<4>(m_data+m_childArrayStart, m_childCount, ucode);
}

TrieNodePrivate* SimpleTrieNodePrivate::childAt(uint32_t pos) const {
	if (pos >= m_childCount)
		pos = (m_childCount-1);
	uint32_t childOffSet = getChildPtr(pos);
	return new SimpleTrieNodePrivate(m_data + (m_myEndPtr+childOffSet));
}

void SimpleTrieNodePrivate::dump() const {
	std::cout << "[";
	std::cout << "m_current[0]=" << static_cast<int>(m_data.at(0)) << ";";
	std::cout << "m_charWidth=" << static_cast<int>(m_charWidth) << "; ";
	std::cout << "m_childCount=" << static_cast<int>(m_childCount) << "; ";
	std::cout << "m_strLen=" << static_cast<int>(m_strLen) << "; ";
	std::cout << "str=" << str() << ";";
	std::cout << "mergeIndex=" << (hasMergeIndex() ? "true" : "false") << "; ";
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

uint32_t SimpleTrieNodePrivate::getStorageSize() const {
	return 5+strLen()+8*m_childCount+16;
}

uint32_t SimpleTrieNodePrivate::getHeaderStorageSize() const {
	return 4;
}


uint32_t SimpleTrieNodePrivate::getNodeStringStorageSize() const {
	return 1 + strLen();
}

uint32_t SimpleTrieNodePrivate::getChildPtrStorageSize() const {
	return m_childCount*4;
}

uint32_t SimpleTrieNodePrivate::getChildCharStorageSize() const {
	return 4*m_childCount;
}

uint32_t SimpleTrieNodePrivate::getIndexPtrStorageSize() const {
	return 16;
}


SimpleStaticTrieCreationNode::SimpleStaticTrieCreationNode(const UByteArrayAdapter & nodeData) : m_node(nodeData), m_data(nodeData) {};


bool SimpleStaticTrieCreationNode::setChildPointer(uint32_t childNum, uint32_t offSetFromBeginning) {
	if (m_node.childCount() <= childNum)
		return false;
	m_data.putUint32(m_node.getChildPtrBeginOffset()+4*childNum, offSetFromBeginning);
	return true;
}

unsigned int
SimpleStaticTrieCreationNode::createNewNode(
    const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination) {

	uint32_t childrenCount = nodeInfo.childChars.size();

	if (childrenCount > 0xFFFF) {
		std::cout << "Error: too many children" << std::endl;
		return SimpleStaticTrieCreationNode::TOO_MANY_CHILDREN;
	}

	uint8_t charWidth = 1;
	if (nodeInfo.childChars.size() > 0) {
		charWidth = CompactUintArray::minStorageBitsFullBytes(nodeInfo.childChars.back());
	}

	destination.putUint16(childrenCount);

	destination.putUint8(charWidth);

	destination.putUint8(nodeInfo.indexTypes | (nodeInfo.mergeIndex ? Static::TrieNodePrivate::IT_MERGE_INDEX : 0));
	
	//Possibly push our node string
	if (nodeInfo.nodeStr.size() > 0xFF)
		return NODE_STRING_TOO_LONG;
	destination.putUint8(nodeInfo.nodeStr.size());
	if (nodeInfo.nodeStr.size() > 0) {
		destination.putData(reinterpret_cast<const uint8_t*>(nodeInfo.nodeStr.c_str()), nodeInfo.nodeStr.size());
	}

	//Push the child chars and child ptrs
	if (childrenCount) {
		for(unsigned int i=0; i < childrenCount; i++) {
			destination.putUint32(nodeInfo.childChars[i]);
		}

		//dummy pointer values
		for(unsigned int j = 0; j < childrenCount; j++) {
			destination.putUint32(nodeInfo.childPtrs[j]);
		}
	}
	
	//Push index pointers
	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_EXACT) {
		destination.putUint32(nodeInfo.exactIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_PREFIX) {
		destination.putUint32(nodeInfo.prefixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX) {
		destination.putUint32(nodeInfo.suffixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	if (nodeInfo.indexTypes & sserialize::Static::TrieNodePrivate::IT_SUFFIX_PREFIX) {
		destination.putUint32(nodeInfo.suffixPrefixIndexPtr);
	}
	else {
		destination.putUint32(0xFFFFFFFF);
	}

	return SimpleStaticTrieCreationNode::NO_ERROR;
}


unsigned int SimpleStaticTrieCreationNode::appendNewNode(const sserialize::Static::TrieNodeCreationInfo& nodeInfo, UByteArrayAdapter& destination) {
	return SimpleStaticTrieCreationNode::createNewNode(nodeInfo, destination);
}

unsigned int
SimpleStaticTrieCreationNode::
prependNewNode(const TrieNodeCreationInfo & nodeInfo, std::deque< uint8_t >& destination)
{
	std::vector< uint8_t > tmpDest;
	UByteArrayAdapter tmpDestAdap(&tmpDest);
	unsigned int err = SimpleStaticTrieCreationNode::appendNewNode(nodeInfo, tmpDestAdap);
	prependToDeque(tmpDest, destination);
	return err;
}

bool SimpleStaticTrieCreationNode::isError(unsigned int error) {
	return (error !=  SimpleStaticTrieCreationNode::NO_ERROR);
}

std::string SimpleStaticTrieCreationNode::errorString(unsigned int error) {
	std::string estr = "";
	if (error & SimpleStaticTrieCreationNode::TOO_MANY_CHILDREN) estr += "Too many children! ";
	if (error & SimpleStaticTrieCreationNode::CHILD_PTR_FAILED) estr += "Failed to set child ptr! ";
	if (error & SimpleStaticTrieCreationNode::NODE_STRING_TOO_LONG) estr += "Nodestring too long! ";
	return estr;
}


uint8_t SimpleStaticTrieCreationNode::getType() {
	return TrieNode::T_SIMPLE;
}

}}//end namespace