#ifndef SSERIALIZE_UNICODE_TRIE_NODE_H
#define SSERIALIZE_UNICODE_TRIE_NODE_H
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace Static {
namespace UnicodeTrie {
namespace detail {

class Node {
public:
	Node();
	virtual ~Node();

	virtual uint8_t strLen() const = 0;
	virtual UByteArrayAdapter strData() const = 0;
	virtual std::string str() const = 0;

	virtual uint32_t childSize() const = 0;
	virtual uint32_t childKey(uint32_t pos) const = 0;
	virtual uint32_t find(uint32_t unicode_point) const = 0;
	virtual Node* child(uint32_t pos) const = 0;
	virtual uint32_t childPtr(uint32_t pos) const = 0;

	virtual void dump() const = 0;
	
	virtual uint32_t payloadPtr() const = 0;
};

struct NodeSerializationInfo {
	std::string::const_iterator strBegin;
	std::string::const_iterator strEnd;
	std::vector<  std::pair<uint32_t, uint32_t> > childKeyPtrOffsets;
	uint32_t payloadPtr;
};

struct NodeCreator {
	virtual bool serialize(const NodeSerializationInfo & src, sserialize::UByteArrayAdapter & dest) = 0;
};

}

class Node {
public:
	static const uint32_t npos = 0xFFFFFFFF;
private:
	std::shared_ptr<detail::Node> m_priv;
protected:
	inline const std::shared_ptr<detail::Node> & priv() const { return m_priv; }
	inline std::shared_ptr<detail::Node> & priv() { return m_priv; }
public:
	Node();
	Node(detail::Node * node) : m_priv(node) {}
	virtual ~Node() {}

	inline uint8_t strLen() const { return priv()->strLen(); }
	inline UByteArrayAdapter strData() const { return priv()->strData(); }
	inline std::string str() const { return priv()->str(); }

	inline uint32_t childSize() const { return priv()->childSize();}
	inline uint32_t childKey(const uint32_t pos) const { return priv()->childKey(pos);}
	inline uint32_t find(const uint32_t ucode) const { return priv()->find(ucode);};
	inline uint32_t childPtr(const uint32_t pos) const { return priv()->childPtr(pos); }
	inline Node child(const uint32_t pos) const { return Node( priv()->child(pos) );}

	inline void dump() const { priv()->dump();}

	inline uint32_t payloadPtr() const { return priv()->payloadPtr();}
	
};

}}}//end namespace

#endif