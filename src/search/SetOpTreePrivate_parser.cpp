#include <sserialize/search/SetOpTreePrivateComplex.h>

#include <iostream>

namespace sserialize {

SetOpTreePrivateComplex::TreeBuilder::Tokenizer::Tokenizer() : m_beforeWasTerminal(false), parseString(""), strIt(parseString.begin()), strEnd(parseString.end()) {}

SetOpTreePrivateComplex::TreeBuilder::Tokenizer::Tokenizer(const std::string & iparseString, uint32_t defaultOp, const std::map<char, uint32_t> & ops, std::map<std::string, uint32_t> & em) :
m_beforeWasTerminal(false),
m_defaultOp(defaultOp),
parseString( sanitize(iparseString, ops) ),
strIt(this->parseString.begin()),
strEnd(this->parseString.end()),
m_opMap(ops),
m_efMap(em)
{}

SetOpTreePrivateComplex::TreeBuilder::Tokenizer::~Tokenizer() {}

std::string SetOpTreePrivateComplex::TreeBuilder::Tokenizer::sanitize(const std::string& str, const std::map<char, uint32_t>& ops) {
	uint32_t braceCount = 0;
	bool beforeWasOp = false;
	std::string ostr = "";
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == '\\') {
			++it;
			if (it != str.end()) {
				ostr += '\\';
				ostr += *it;
			}
			else
				break;
		}
		else if (*it == ')') {
			if (braceCount == 0) {
				continue;
			}
			else {
				--braceCount;
				ostr += *it;
			}
		}
		else if (ops.count(*it) > 0) {
			if (beforeWasOp) {
				continue;
			}
			else {
				beforeWasOp = true;
				ostr += *it;
			}
		}
		else if (*it == '(') {
			++braceCount;
			ostr += *it;
		}
		else {
			beforeWasOp = false;
			ostr += *it;
		}
	}
	if (braceCount > 0) { //append missing braces
		ostr += std::string(braceCount, ')');
	}
	return ostr;
}

bool SetOpTreePrivateComplex::TreeBuilder::Tokenizer::hasNext() {
	return strIt != parseString.end();
}

void SetOpTreePrivateComplex::TreeBuilder::Tokenizer::readString(Token & token) {
	token.begin = strIt - parseString.begin();
	token.type = Token::STRING;
	while (strIt != strEnd) {
		if (*strIt == '\\') {
			++strIt;
			if (strIt != strEnd) {
				token.tokenString += *strIt;
				++strIt;
			}
			else
				break;
		}
		else if (m_opMap.count(*strIt) > 0 || *strIt == '$' || *strIt == ' ' || *strIt == '(' || *strIt == ')')
			break;
		else {
			token.tokenString += *strIt;
			++strIt;
		}
	}
	token.end = strIt - parseString.begin();
}

void SetOpTreePrivateComplex::TreeBuilder::Tokenizer::readExternal(Token & token) {
	token.begin = strIt - parseString.begin();
	++strIt;
	token.type = Token::EXTERNAL;
	token.op = 0xFFFFFFFF;
	std::string efName;
	std::string efValue;
	while(strIt != strEnd) {
		if (*strIt == '[') {
			++strIt;
			break;
		}
		else if (*strIt == ' ') {
			return;
		}
		else {
			efName += *strIt;
			++strIt;
		}
	}
	while(strIt != strEnd) {
		if (*strIt == '\\') {
			++strIt;
			if (strIt != strEnd) {
				efValue += *strIt;
			}
			else
				break;
		}
		else if (*strIt == ']') {
			++strIt;
			break;
		}
		else {
			efValue += *strIt;
			++strIt;
		}
	}
	token.end = strIt - parseString.begin();
	token.tokenString = efValue;
	if (m_efMap.count(efName))
		token.op = m_efMap.at(efName);
	else
		token.op = 0xFFFFFFFF;
}

void SetOpTreePrivateComplex::TreeBuilder::Tokenizer::createDefaultOp(Token & token) const {
	token.begin = strIt - parseString.begin();
	token.tokenString = "";
	token.end = token.begin;
	token.type = Token::OPERATOR;
	token.op = m_defaultOp;
}

//Implements a simple tokenizer. No need to use a psreser generator here (yet?) as the grammar is simpler to parse by hand
SetOpTreePrivateComplex::TreeBuilder::Tokenizer::Token SetOpTreePrivateComplex::TreeBuilder::Tokenizer::next() {
	Token token;
	token.type = Token::ENDOFINPUT;
	while(strIt != strEnd) {
		if (*strIt == ' ') {
			++strIt;
		}
		else if (m_opMap.count(*strIt) > 0) {
			token.begin = strIt - parseString.begin();
			token.tokenString += *strIt;
			token.type = Token::OPERATOR;
			token.op = m_opMap.at(*strIt);
			++strIt;
			token.end = strIt - parseString.begin();
			m_beforeWasTerminal = false;
			break;
		}
		else if (*strIt == '(') {
			if (m_beforeWasTerminal) {
				createDefaultOp(token);
				m_beforeWasTerminal = false;
			}
			else {
				token.begin = strIt - parseString.begin();
				token.tokenString += *strIt;
				token.type = Token::BRACE_OPEN;
				++strIt;
				token.end = strIt - parseString.begin();
			}
			break;
		}
		else if (*strIt == ')') {
			token.begin = strIt - parseString.begin();
			token.tokenString += *strIt;
			token.type = Token::BRACE_CLOSE;
			++strIt;
			token.end = strIt - parseString.begin();
			m_beforeWasTerminal = true;
			break;
		}
		else if (*strIt == '$') {
			if (m_beforeWasTerminal) {
				createDefaultOp(token);
				m_beforeWasTerminal = false;
			}
			else {
				readExternal(token);
				m_beforeWasTerminal = true;
			}
			break;
		}
		else {
			if (m_beforeWasTerminal) {
				createDefaultOp(token);
				m_beforeWasTerminal = false;
			}
			else {
				readString(token);
				m_beforeWasTerminal = true;
			}
			break;
		}
	}
	return token;
}

SetOpTreePrivateComplex::TreeBuilder::TreeBuilder(uint32_t defaultOp, const std::map<char, uint32_t> & opMap, const std::map<std::string, RCPtrWrapper<SetOpTree::SelectableOpFilter> > & ef) :
m_defaultOp(defaultOp),
m_opMap(opMap),
m_ef(ef)
{}
SetOpTreePrivateComplex::TreeBuilder::~TreeBuilder() {}

void SetOpTreePrivateComplex::TreeBuilder::clear() {}


SetOpTreePrivateComplex::Node* SetOpTreePrivateComplex::TreeBuilder::createExternalNode(const Tokenizer::Token & token, std::map<uint32_t, RCPtrWrapper<SetOpTree::SelectableOpFilter> > & ef) const {
	if (token.op != 0xFFFFFFFF && ef.count(token.op) > 0) {
		Node * n = new Node(Node::EXTERNAL, 0);
		n->completeString = token.tokenString;
		n->externalFunc = ef.at(token.op).priv();
		n->srcBegin = token.begin;
		n->srcEnd = token.end;
		return n;
	}
	return new Node(Node::INVALID);
}

SetOpTreePrivateComplex::Node* SetOpTreePrivateComplex::TreeBuilder::createOperationNode(const Tokenizer::Token& token) const {
	Node * node = new Node(token.op, 0);
	node->srcBegin = token.begin;
	node->srcEnd = token.end;
	return node;
}

SetOpTreePrivateComplex::Node* SetOpTreePrivateComplex::TreeBuilder::createStringCompletionNode(const Tokenizer::Token & token) const {
	Node * n = new Node(Node::COMPLETE, 0);
	n->cqtype = StringCompleter::QT_NONE;
	n->completeString = token.tokenString;
	n->srcBegin = token.begin;
	n->srcEnd = token.end;
	char t = 1;
	if (n->completeString.size() == 0) {
		n->cqtype = StringCompleter::QT_PREFIX;
		return n;
	}
	if (*(n->completeString.begin()) == '\"') {
		n->completeString.erase(0,1);
		if (n->completeString.size() > 0 && *(n->completeString.rbegin()) == '\"') {
			n->completeString.erase(n->completeString.size()-1, 1);
		}
	}
	else {
		if (*(n->completeString.begin()) == '*') {
			t = t << 2;
			n->completeString.erase(0, 1);
		}
		if (n->completeString.size() >  0 && *(n->completeString.rbegin()) == '*') {
			t = t << 1;
			n->completeString.erase(n->completeString.size()-1, 1);
		}
		if (t == 1) //default to QT_SUFFIX_PREFIX if no " are given
			t = 8;
	}
	switch (t) {
	case (1):
		n->cqtype = StringCompleter::QT_EXACT;
		break;
	case (2):
		n->cqtype = StringCompleter::QT_PREFIX;
		break;
	case (4):
		n->cqtype = StringCompleter::QT_SUFFIX;
		break;
	case (8):
	default:
		n->cqtype = StringCompleter::QT_SUBSTRING;
		break;
	}
	n->cqtype = (StringCompleter::QuerryType) (n->cqtype | StringCompleter::QT_CASE_INSENSITIVE);
	return n;
}

SetOpTreePrivateComplex::Node * SetOpTreePrivateComplex::TreeBuilder::build(const std::string & parseString, std::map< std::pair< std::string, uint8_t >, sserialize::ItemIndex >& cmpStrings) {
	std::map<std::string, uint32_t> efOpMap;
	std::map<uint32_t, RCPtrWrapper<SetOpTree::SelectableOpFilter> > efOpPtrMap;
	{
		uint32_t count = 0;
		for(std::map<std::string, RCPtrWrapper<SetOpTree::SelectableOpFilter> >::iterator it = m_ef.begin(); it != m_ef.end(); ++it) {
			efOpMap[it->first] = count;
			efOpPtrMap[count] = it->second;
			++count;
		}
	}

	Tokenizer tokenizer(parseString, m_defaultOp, m_opMap, efOpMap);

	if (!tokenizer.hasNext())
		return 0;
	std::vector<uint32_t> stateStack;
	std::vector<Tokenizer::Token> parseStack;
	std::vector<Node*> nodeStack;
	stateStack.push_back(PS_INIT);
	Tokenizer::Token token(tokenizer.next());
	bool done = false;
	while (stateStack.size() && !done) {
		switch (stateStack.back()) {
		case (PS_INIT):
		{
			if (token.type == Tokenizer::Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = tokenizer.next();
			}
			else if (token.type == Tokenizer::Token::EXTERNAL || token.type == Tokenizer::Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = tokenizer.next();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_TERMINAL)://reduce
		{
			Node * node = 0 ;
			if (parseStack.back().type == Tokenizer::Token::EXTERNAL)
				node = createExternalNode(parseStack.back(), efOpPtrMap);
			else {
				node = createStringCompletionNode(parseStack.back());
				cmpStrings[std::pair< std::string, uint8_t >(node->completeString, node->cqtype)] = ItemIndex();
			}
			parseStack.back().type = Tokenizer::Token::NT_SA;
			stateStack.pop_back();
			nodeStack.push_back(node);
			if (stateStack.back() == PS_INIT)
				stateStack.push_back(PS_BEFORE_OP_OR_DONE);
			else if (stateStack.back() == PS_BRACE_OPENED)
				stateStack.push_back(PS_BEFORE_OP_OR_BRACE);
			else if (stateStack.back() == PS_AFTER_OP)
				stateStack.push_back(PS_OP_COMPLETED_OR_BEFORE_OP);
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_BRACE_OPENED):
		{
			if (token.type == Tokenizer::Token::EXTERNAL || token.type == Tokenizer::Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = tokenizer.next();
			}
			else if (token.type == Tokenizer::Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = tokenizer.next();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_BEFORE_OP_OR_DONE):
		{
			if (token.type == Tokenizer::Token::ENDOFINPUT) {
				done = true;
				break;
			}
			else if (token.type == Tokenizer::Token::OPERATOR) {
				parseStack.push_back(token);
				stateStack.push_back(PS_AFTER_OP);
				token = tokenizer.next();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_BEFORE_OP_OR_BRACE):
		{
			if (token.type == Tokenizer::Token::OPERATOR) {
				parseStack.push_back(token);
				stateStack.push_back(PS_AFTER_OP);
				token = tokenizer.next();
			}
			else if (token.type == Tokenizer::Token::BRACE_CLOSE) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_CLOSED);
				token = tokenizer.next();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_AFTER_OP):
		{
			if (token.type == Tokenizer::Token::EXTERNAL || token.type == Tokenizer::Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = tokenizer.next();
			}
			else if (token.type == Tokenizer::Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = tokenizer.next();
			}
			else {
				done = true;
			}
			break;
		}
		case (PS_BRACE_CLOSED):
		{
			parseStack.pop_back(); parseStack.pop_back(); parseStack.pop_back();
			stateStack.pop_back(); stateStack.pop_back(); stateStack.pop_back();
			parseStack.push_back(token); parseStack.back().type = Tokenizer::Token::NT_SA;
			if (stateStack.back() == PS_INIT)
				stateStack.push_back(PS_BEFORE_OP_OR_DONE);
			else if (stateStack.back() == PS_BRACE_OPENED)
				stateStack.push_back(PS_BEFORE_OP_OR_BRACE);
			else if (stateStack.back() == PS_AFTER_OP)
				stateStack.push_back(PS_OP_COMPLETED_OR_BEFORE_OP);
			else { //try error recovery
				done = true;
			}

			break;
		}
		case (PS_OP_COMPLETED_OR_BEFORE_OP):
		{//This one has a shift/reduce-conflict, do reduce
			Node * rightOperand = nodeStack.back(); nodeStack.pop_back(); parseStack.pop_back();
			Node * opNode = createOperationNode(parseStack.back()); parseStack.pop_back();
			Node * leftOperand = nodeStack.back(); nodeStack.pop_back(); parseStack.pop_back();
			leftOperand->parent = opNode;
			rightOperand->parent = opNode;
			opNode->children.push_back(leftOperand);
			opNode->children.push_back(rightOperand);
			nodeStack.push_back(opNode);
			stateStack.pop_back(); stateStack.pop_back(); stateStack.pop_back();
			parseStack.push_back(token); parseStack.back().type = Tokenizer::Token::NT_SA;
			if (stateStack.back() == PS_INIT)
				stateStack.push_back(PS_BEFORE_OP_OR_DONE);
			else if (stateStack.back() == PS_BRACE_OPENED)
				stateStack.push_back(PS_BEFORE_OP_OR_BRACE);
			else if (stateStack.back() == PS_AFTER_OP)
				stateStack.push_back(PS_OP_COMPLETED_OR_BEFORE_OP);
			else { //try error recovery
				done = true;
			}
			break;
		}
		default:
			break;
		};
	}
	if (nodeStack.size() == 1) {
		return nodeStack.front();
	}
	else if (nodeStack.size() == 0)
		return 0;
	else { //try to recover
		std::cerr << "No recovery implemented yet for offending query: " << parseString << ";;" << std::endl;
		for(std::size_t i = 1; i < nodeStack.size(); i++) {
			delete nodeStack[i];
		}
		return nodeStack[0];
	}
}

}//end namespace