#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_SIMPLE_NODE
#define SSERIALIZE_STATIC_UNICODE_TRIE_SIMPLE_NODE
#include <sserialize/Static/UnicodeTrie/Node.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

/**
  *Layout:
  *--------------------------------------------------------
  *ChildCount|(ChildChar|ChildPtr)|payloadPtr|NodeString
  *--------------------------------------------------------
  *vu32      |(vu32,vu32)         |vu32      |std::string
  *
  *
  */

class SimpleNode: public Node {
private:
	typedef std::vector< std::pair<uint32_t, uint32_t> > ChildrenContainer;
private:
	ChildrenContainer m_children;
	uint32_t m_payloadPtr;
	uint32_t m_strLen;
	UByteArrayAdapter m_strTrieData; //begins at nodeString
protected:
	inline UByteArrayAdapter trieData() const { return m_strTrieData+m_strLen;}
public:
	SimpleNode();
	SimpleNode(sserialize::UByteArrayAdapter d);
	virtual ~SimpleNode();
	
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

struct SimpleNodeCreator: sserialize::Static::UnicodeTrie::NodeCreator {
	~SimpleNodeCreator() override {}
	virtual uint32_t type() const override;
	virtual bool append(const NodeSerializationInfo & src, sserialize::UByteArrayAdapter & dest) override;
};

}}}}


#endif
