#ifndef SSERIALIZE_OP_TREE_H
#define SSERIALIZE_OP_TREE_H
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <sserialize/utility/refcounting.h>

namespace sserialize {
namespace OpTree {
namespace detail {

template<typename T_OPERAND>
class OpTree {
public:
	typedef T_OPERAND value_type;
public:
	OpTree() {}
	virtual ~OpTree() {}
	virtual bool parse(const std::string & query) = 0;
	virtual value_type calc() = 0;
	virtual OpTree * copy() const = 0;
};

template<typename T_OPERAND>
class EmptyOpTree: public OpTree<T_OPERAND> {
public:
	EmptyOpTree() {}
	virtual ~EmptyOpTree() {}
	virtual bool parse(const std::string & /*query*/) {
		return false;
	}
	virtual T_OPERAND calc() { return T_OPERAND();}
	virtual OpTree<T_OPERAND> * copy() const { return new EmptyOpTree(); }
};

struct Node {
	typedef enum {INVALID=0, LEAF=1, OPERATION=2} Type;
	std::vector<Node*> children;
	uint8_t type;
	Node() : type(INVALID) {}
	Node(Type t) : type(t) {}
	virtual ~Node() {
		for(Node * n : children)
			delete n;
	}
	virtual Node* copy() const = 0;
};

struct OpNode: public Node {
	uint8_t opType;
	OpNode() : Node(Node::OPERATION) {}
	OpNode(uint8_t opType) : Node(Node::OPERATION), opType(opType) {}
	virtual Node* copy() const {
		OpNode * n = new OpNode(opType);
		for(Node * cn : this->children) {
			n->children.push_back(cn->copy());
		}
		return n;
	}
};

struct LeafNode: public Node {
	std::string q;
	LeafNode() : Node(Node::LEAF) {}
	LeafNode(const std::string & q) : Node(Node::LEAF), q(q) {}
	virtual Node* copy() const {
		return new LeafNode(q);
	}
};

class OpTreeParser {
protected:
	struct Token {
		Token() : begin(0xFEFE), end(0xFFFF), type(INVALID), op(0xFF)  {}
		enum Type {STRING, OPERATOR, BRACE_OPEN, BRACE_CLOSE, ENDOFINPUT, NT_SA, INVALID}; ///NT_SA is internal
		uint16_t begin;
		uint16_t end;
		std::string tokenString;
		Type type;
		uint8_t op;
	};
protected:
	virtual void initTokenizer(const std::string & parseString) = 0;
	virtual Token nextToken() = 0;
	virtual bool hasNextToken() = 0;
public:
	OpTreeParser() {}
	virtual ~OpTreeParser() {}
	Node * parse(const std::string& parseString);
};

class SetOpsOpTreeParser: public OpTreeParser {
public:
	typedef enum {OT_INVALID=0, OT_UNITE=1, OT_INTERSECT=2, OT_DIFF=3, OT_SYM_DIFF=4} OpTypes;
	struct OpDesc {
		uint8_t type;
		uint8_t children;
		OpDesc(uint8_t type, uint8_t children) : type(type), children(children) {}
		OpDesc() : type(OT_INVALID), children(0) {}
	};
	struct StringHinter: sserialize::RefCountObject {
		virtual bool operator()(const std::string::const_iterator & begin, const std::string::const_iterator end) const = 0;
	};
	typedef sserialize::RCPtrWrapper<StringHinter> StringHinterSharedPtr;
private:
	std::unordered_map<char, OpDesc> m_opMap;
	std::string m_parseString;
private://state
	std::string::const_iterator m_strIt;
	std::string::const_iterator m_strEnd;
	bool m_beforeWasTerminal;
	StringHinterSharedPtr m_strHinter;
private:
	void sanitize();
	void readString(Token & token);
	void createDefaultOp(Token & token) const;
protected:
	virtual void initTokenizer(const std::string & parseString) override;
	virtual Token nextToken() override;
	virtual bool hasNextToken() override;
public:
	SetOpsOpTreeParser(const SetOpsOpTreeParser & other);
	SetOpsOpTreeParser(const StringHinterSharedPtr & strHinter);
	virtual ~SetOpsOpTreeParser() {}
};


}//end namespace detail

template<typename T_OPERAND>
class OpTree {
public:
	typedef T_OPERAND value_type;
	typedef detail::OpTree<value_type> MyImpClass;
private:
	std::unique_ptr<MyImpClass> m_priv;
protected:
	inline const std::unique_ptr<MyImpClass> & priv() const { return m_priv; }
	inline std::unique_ptr<MyImpClass> & priv() { return m_priv; }
	inline MyImpClass* copy() const {
		return priv()->copy();
	}
public:
	OpTree() : m_priv(new detail::EmptyOpTree<T_OPERAND>()) {}
	OpTree(MyImpClass * priv) : m_priv(priv) {}
	OpTree(std::unique_ptr<MyImpClass> && priv) : m_priv(std::move(priv)) {}
	OpTree(const OpTree & other) : m_priv(other.copy()) {}
	OpTree(OpTree && other) : m_priv(std::move(other.m_priv)) {}
	virtual ~OpTree() {}
	inline bool parse(const std::string & query) {
		return priv()->parse(query);
	}
	inline value_type calc() {
		return priv()->calc();
	}
};


}}//end namespace

#endif