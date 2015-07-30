#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_COMPACT_NODE_H
#define SSERIALIZE_STATIC_UNICODE_TRIE_COMPACT_NODE_H
#include <sserialize/Static/UnicodeTrie/Node.h>
#include <sserialize/containers/CompactUintArray.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

/**
  *Layout:
  *----------------------------------------------------------------------
  *(ChildCount|StringAvailable)|(ChildChar|ChildPtr)|payloadPtr|NodeString
  *--------------------------------------------------------------------
  *vu32                        |CompactUintArray    |vu32      |std::string
  *
  *
  */

class CompactNode: public Node {
private:
	typedef sserialize::CompactUintArray ChildrenContainer;
private:
	ChildrenContainer m_children;
	uint32_t m_payloadPtr;
	uint32_t m_strLen;
	UByteArrayAdapter m_strTrieData; //begins at nodeString
protected:
	inline UByteArrayAdapter trieData() const { return m_strTrieData+m_strLen;}
public:
	CompactNode();
	CompactNode(sserialize::UByteArrayAdapter d);
	virtual ~CompactNode();
	
	virtual uint32_t strLen() const override;
	virtual UByteArrayAdapter strData() const override;
	virtual std::string str() const override;

	virtual uint32_t childSize() const override;
	virtual uint32_t childKey(uint32_t pos) const override;
	virtual uint32_t find(uint32_t unicode_point) const override;
	virtual Node* child(uint32_t pos) const override;
	virtual uint32_t childPtr(uint32_t pos) const override;
	
	virtual uint32_t payloadPtr() const override;
};

struct CompactNodeNodeCreator: sserialize::Static::UnicodeTrie::NodeCreator {
	virtual uint32_t type() const override;
	virtual bool append(const NodeSerializationInfo & src, sserialize::UByteArrayAdapter & dest) override;
};

}}}}


#endif