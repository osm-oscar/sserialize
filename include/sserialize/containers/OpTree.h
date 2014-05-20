#ifndef SSERIALIZE_OP_TREE_H
#define SSERIALIZE_OP_TREE_H
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <boost/concept_check.hpp>

namespace sserialize {
namespace OpTree {
namespace detail {

template<typename T_OPERAND>
class OpTree {
public:
	OpTree() {}
	virtual ~OpTree() {}
	virtual bool parse(const std::string & query) = 0;
	virtual T_OPERAND calc() = 0;
	virtual OpTree * copy() const = 0;
};

template<typename T_OPERAND>
class EmptyOpTree {
public:
	EmptyOpTree() {}
	virtual ~EmptyOpTree() {}
	virtual bool parse(const std::string & query) {
		return false;
	}
	virtual T_OPERAND calc() { return T_OPERAND();}
	virtual OpTree * copy() const { return new EmptyOpTree(); }
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
		Token() : begin(0xFEFE), end(0xFEFF), type(INVALID), op(0xFFFFFFFF)  {}
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
	Node * parse(const std::string & query) const;
};

class SetOpsOpTreeParser: public OpTreeParser {
public:
	typedef enum {OT_INVALID=0, OT_UNITE=1, OT_INTERSECT=2, OT_DIFF=3, OT_SYM_DIFF=4} OpTypes;
	struct OpDesc {
		uint16_t type:10;
		uint16_t children:2;
		OpDesc(uint16_t type, uint16_t children) : type(type), children(children) {}
	};
private:
	std::unordered_map<char, OpDesc> m_opMap;
	std::string m_parseString;
private://state
	std::string::const_iterator m_strIt;
	std::string::const_iterator m_strEnd;
	bool m_beforeWasTerminal;
private:
	void sanitize();
	void readString(Token & token);
	void createDefaultOp(Token & token) const;
protected:
	virtual void initTokenizer(const std::string & parseString) override;
	virtual Token nextToken() override;
	virtual bool hasNextToken() override;
public:
	SetOpsOpTreeParser();
	virtual ~SetOpsOpTreeParser() {}
};


}//end namespace detail

template<typename T_OPERAND>
class OpTree {
public:
	typedef OpTree<T_OPERAND> MyImpClass;
private:
	std::unique_ptr<MyImpClass> m_priv;
protected:
	inline const std::unique_ptr<MyImpClass> & priv() const { return m_priv; }
	inline std::unique_ptr<MyImpClass> & priv() { return m_priv; }
	inline MyImpClass* copy() const {
		return priv()->copy();
	}
public:
	OpTree();
	OpTree(MyImpClass * priv);
	OpTree(std::unique_ptr<MyImpClass> && priv);
	OpTree(const OpTree & other) : m_priv(other.copy()) {}
	OpTree(OpTree && other) : m_priv(other.m_priv) {}
	virtual ~OpTree();
	inline bool parse(const std::string & query) {
		return priv()->parse();
	}
	inline T_OPERAND calc() {
		return priv()->calc();
	}
};


}}//end namespace

#endif