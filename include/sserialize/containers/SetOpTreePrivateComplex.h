#ifndef SET_OP_TREE_PRIVATE_COMPLEX_H
#define SET_OP_TREE_PRIVATE_COMPLEX_H
#include "SetOpTreePrivate.h"


/** This class abstracts set operations. You give it a query string and a StringCompleter and it will perform the necessary operations
 *
 * Supported set operations:
 * unite, intersect, difference, symmetric-difference
 * With special symbols:
 * * = String completion operator: "str" matches only str, "str*" matches with prefix, "*str" matches with suffix, "*str*" suffix and prefix
 * + = unite
 * " " = intersect (without the "", also acts as delimeter if it's followed by an operator)
 * - = difference
 * ^ = symmetric-difference
 * () = the usual
 * {} = everything within these will be but into the external completer
 * $ = start of 
 * $String{String}
 * $ExternalFunctoid{} can be registerd on runtime.
 * it will get the String in the surrounding brackets: If you need to use the closeing brace }, then you have to escape it
 */
 
 //TODO:hard constraints: Set op node should ALWAYS have exactly 2 children

namespace sserialize {
 
class SetOpTreePrivateComplex: public SetOpTreePrivate {
public:
	/** A class to represent the parsed syntax/semantic-tree. It should always have zero or 2 children */
	struct Node {
		enum Types { INVALID=0, COMPLETE=0x1, EXTERNAL=0x2, ENDNODE=0x3, IDENTITY=0x4, DIFFERENCE=0x8, INTERSECT=0x10, SYMMETRIC_DIFFERENCE=0x20, UNITE=0x40, OPERATION=0x78};
		uint32_t type;
		StringCompleter::QuerryType cqtype;
		Node * parent;
		std::vector<Node*> children;
		std::string completeString;
		bool cached;
		ItemIndex index;
		SetOpTree::SelectableOpFilter * externalFunc;
		uint16_t srcBegin;
		uint16_t srcEnd;
		Node(Node * parent = 0) :
			type(INVALID), cqtype(StringCompleter::QT_EXACT),
			parent(parent), cached(false), externalFunc(0)
			{children.reserve(2);}
		Node(uint32_t type, Node * parent = 0) :
			type(type), cqtype(StringCompleter::QT_EXACT),
			parent(parent), cached(false), externalFunc(0)
			{children.reserve(2);}
		Node(const std::string & completeString, StringCompleter::QuerryType qtype, Node * parent = 0) :
			type(Node::COMPLETE), cqtype(qtype),
			parent(parent),
			completeString(completeString),
			cached(false), externalFunc(0)
			{children.reserve(2);} 
		Node(const std::string & completeString, StringCompleter::QuerryType qtype, uint32_t type, Node * parent = 0) :
			type(type), cqtype(qtype), parent(parent),
			completeString(completeString),
			cached(false), externalFunc(0)
			{children.reserve(2);}

		~Node();
		void printStructure(std::ostream & out) const;
		void printStructure() const;
		Node * getDeepCopy(Node * parent);
		Node * getMostRight();
		Node * getNodeWithSrcCollision(uint16_t position);
		/** @return false if this == 0 or externalFunc == 0 or externalFunc does not support op */
		bool efSupport(SetOpTree::SelectableOpFilter::SupportedOps op) const;
	};
private:
	class TreeBuilder {
	public:
		class Tokenizer {
		public:
			struct Token {
				Token() : begin(0xFEFE), end(0xFEFF), type(INVALID), op(0xFFFFFFFF)  {}
				enum Type {STRING, EXTERNAL, OPERATOR, BRACE_OPEN, BRACE_CLOSE, ENDOFINPUT, NT_SA, INVALID};
				uint16_t begin;
				uint16_t end;
				/** This holds the string, in case of EXTERNAL it holds the content of EXTERNAL */
				std::string tokenString;
				Type type;
				/** This holds the value of the opMap or externalMap */
				uint32_t op;
			};
		public:
			bool m_beforeWasTerminal;
			uint32_t m_defaultOp;
			std::string parseString;
			std::string::const_iterator strIt;
			std::string::const_iterator strEnd;
			std::map<char, uint32_t> m_opMap;
			std::map<std::string, uint32_t> m_efMap;
		private:
			static std::string sanitize(const std::string & str, const std::map<char, uint32_t> & ops);
			void readString(Token & token);
			void readExternal(Token & token);
			void createDefaultOp(Token& token) const;
		public:
			Tokenizer();
			Tokenizer(const std::string & parseString, uint32_t defaultOp, const std::map<char, uint32_t> & ops, std::map<std::string, uint32_t> & em);
			~Tokenizer();
			bool hasNext();
			Token next();
		};
	private:
		enum {PS_INIT=0, PS_BRACE_OPENED=1, PS_TERMINAL=2, PS_BEFORE_OP_OR_DONE=3, PS_BEFORE_OP_OR_BRACE=4, PS_AFTER_OP=5, PS_BRACE_CLOSED=6, PS_OP_COMPLETED_OR_BEFORE_OP=7} StateTypes;
	private:
		uint32_t m_defaultOp;
		std::map<char, uint32_t> m_opMap;
		std::map<std::string,  std::shared_ptr<SetOpTree::SelectableOpFilter> > m_ef; 
	private:
		Node * createExternalNode(const sserialize::SetOpTreePrivateComplex::TreeBuilder::Tokenizer::Token& token, std::map<uint32_t, std::shared_ptr<sserialize::SetOpTree::SelectableOpFilter> >& ef) const;
		Node * createOperationNode(const Tokenizer::Token & token) const;
		Node * createStringCompletionNode(const Tokenizer::Token & token) const;
	public:
		TreeBuilder(uint32_t defaultOp, const std::map<char, uint32_t> & opMap, const std::map<std::string, std::shared_ptr<SetOpTree::SelectableOpFilter> > & ef);
		virtual ~TreeBuilder();
		void clear();
		Node* build(const std::string & parseString, std::map< std::pair< std::string, uint8_t >, sserialize::ItemIndex >& cmpStrings);
	};

private:
	enum TreeDiffTypes {UDT_SUBSET=1, UDT_SUPERSET=2, UDT_EQUAL=3, UDT_DIFFERENT=4};

private:
	StringCompleter m_strCompleter;
	//Holds the Querry string. i.e. (Stutt AND STEIN) OR (STUTT AND NEU)
	std::string m_queryString;
// 	std::map<uint32_t, Node*> m_qSubStrToNode;
	std::map<std::string, std::shared_ptr<SetOpTree::SelectableOpFilter> > m_externalFunctoids;
	std::map< std::pair<std::string, uint8_t>, ItemIndex> m_completions;
	Node * m_rootNode;
private://utility functions
	TreeDiffTypes completionStringDifference(const std::string & newString, const std::string & oldString);
	ItemIndexIterator createItemIndexIteratorTree(Node * node);

private:
	ItemIndex doSetOperationsRecurse(SetOpTreePrivateComplex::Node* node);
	ItemIndex doSetOperationsRecurse(SetOpTreePrivateComplex::Node* node, SetOpTreePrivateComplex::Node* refTree, TreeDiffTypes & diff);
	bool charHintsCheckChanged(Node * node, Node * child, const ItemIndex & index); 
	std::set<uint16_t> getCharHintsFromNode(Node * node);
	bool parseQuery(const std::string& queryString, sserialize::SetOpTreePrivateComplex::Node*& treeRootNode, std::map< std::pair< std::string, uint8_t >, sserialize::ItemIndex >& cmpStrings);
	void clearTree();
public:
	SetOpTreePrivateComplex();
	SetOpTreePrivateComplex(const StringCompleter & stringCompleter);
	~SetOpTreePrivateComplex();
	virtual SetOpTreePrivate * copy() const;


	virtual void buildTree(const std::string & queryString);
	virtual ItemIndexIterator asItemIndexIterator();
	virtual ItemIndex update(const std::string & queryString);
	virtual void doCompletions();
	virtual ItemIndex doSetOperations();
	/** This is actualy const for cow. It may alter things in StringCompleter */
	virtual std::set<uint16_t> getCharacterHint(uint32_t posInQueryString);
	virtual bool registerExternalFunction(SetOpTree::ExternalFunctoid * function);
	/** creates a copy of filter by calling copy() on filter which will be deleted if Tree is deleted  */
	virtual bool registerSelectableOpFilter(SetOpTree::SelectableOpFilter * filter);
	virtual void clear();
	virtual std::ostream & printStructure(std::ostream & out) const;
};

}

#endif