#ifndef SSERIALIZE_STATIC_UNICODE_TRIE_DETAIL_EMPTY_NODE_H
#define SSERIALIZE_STATIC_UNICODE_TRIE_DETAIL_EMPTY_NODE_H
#include <sserialize/Static/UnicodeTrie/Node.h>


namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

class EmptyNode: public Node {
public:
	EmptyNode();
	EmptyNode(sserialize::UByteArrayAdapter d);
	virtual ~EmptyNode();
	virtual bool valid() const override;
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

}}}}

#endif