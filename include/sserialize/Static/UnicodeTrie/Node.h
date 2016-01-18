#ifndef SSERIALIZE_UNICODE_TRIE_NODE_H
#define SSERIALIZE_UNICODE_TRIE_NODE_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <iostream>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

class Node: public sserialize::RefCountObject {
public:
	Node() {}
	virtual ~Node() {}
	virtual bool valid() const { return true; }

	virtual uint32_t strLen() const = 0;
	virtual UByteArrayAdapter strData() const = 0;
	virtual std::string str() const = 0;

	virtual uint32_t childSize() const = 0;
	virtual uint32_t childKey(uint32_t pos) const = 0;
	virtual uint32_t find(uint32_t unicode_point) const = 0;
	virtual Node* child(uint32_t pos) const = 0;
	virtual uint32_t childPtr(uint32_t pos) const = 0;

	virtual void dump(std::ostream & out) const;
	
	virtual uint32_t payloadPtr() const = 0;
};

struct NodeSerializationInfo {
	std::string::const_iterator strBegin;
	std::string::const_iterator strEnd;
	std::vector<  std::pair<uint32_t, uint32_t> > childKeyPtrOffsets;
	uint32_t payloadPtr;
};

}

struct NodeCreator {
	virtual uint32_t type() const = 0;
	virtual bool append(const detail::NodeSerializationInfo & src, sserialize::UByteArrayAdapter & dest) = 0;
};

class Node {
public:
	static const uint32_t npos = 0xFFFFFFFF;
	typedef enum {NT_SIMPLE=1} NodeTypes;
private:
	sserialize::RCPtrWrapper<detail::Node> m_priv;
protected:
	inline const sserialize::RCPtrWrapper<detail::Node> & priv() const { return m_priv; }
	inline sserialize::RCPtrWrapper<detail::Node> & priv() { return m_priv; }
public:
	Node();
	Node(detail::Node * node) : m_priv(node) {}
	virtual ~Node() {}
	virtual bool valid() const { return priv()->valid(); }

	inline uint32_t strLen() const { return priv()->strLen(); }
	inline UByteArrayAdapter strData() const { return priv()->strData(); }
	inline std::string str() const { return priv()->str(); }

	inline uint32_t childSize() const { return priv()->childSize();}
	inline uint32_t childKey(const uint32_t pos) const { return priv()->childKey(pos);}
	inline uint32_t find(const uint32_t ucode) const { return priv()->find(ucode);};
	inline uint32_t childPtr(const uint32_t pos) const { return priv()->childPtr(pos); }
	inline Node child(const uint32_t pos) const { return Node( priv()->child(pos) );}

	inline void dump() const { priv()->dump(std::cout);}

	inline uint32_t payloadPtr() const { return priv()->payloadPtr();}
	
};

struct RootNodeAllocator {
	Node operator()(uint32_t nodeType, const sserialize::UByteArrayAdapter & src) const;
};

}}}//end namespace

#endif