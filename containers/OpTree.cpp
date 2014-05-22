#include <sserialize/containers/OpTree.h>
#include <iostream>

namespace sserialize {
namespace OpTree {
namespace detail {

Node * OpTreeParser::parse(const std::string & parseString) {
	enum {PS_INIT=0, PS_BRACE_OPENED=1, PS_TERMINAL=2, PS_BEFORE_OP_OR_DONE=3, PS_BEFORE_OP_OR_BRACE=4, PS_AFTER_OP=5, PS_BRACE_CLOSED=6, PS_OP_COMPLETED_OR_BEFORE_OP=7};

	initTokenizer(parseString);
	
	if (!hasNextToken())
		return 0;
	std::vector<uint32_t> stateStack;
	std::vector<Token> parseStack;
	std::vector<Node*> nodeStack;
	stateStack.push_back(PS_INIT);
	Token token(nextToken());
	bool done = false;
	while (stateStack.size() && !done) {
		switch (stateStack.back()) {
		case (PS_INIT):
		{
			if (token.type == Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = nextToken();
			}
			else if (token.type == Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = nextToken();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_TERMINAL)://reduce
		{
			LeafNode * node = new LeafNode();
			node->q = parseStack.back().tokenString;
			parseStack.back().type = Token::NT_SA;
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
			if (token.type == Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = nextToken();
			}
			else if (token.type == Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = nextToken();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_BEFORE_OP_OR_DONE):
		{
			if (token.type == Token::ENDOFINPUT) {
				done = true;
				break;
			}
			else if (token.type == Token::OPERATOR) {
				parseStack.push_back(token);
				stateStack.push_back(PS_AFTER_OP);
				token = nextToken();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_BEFORE_OP_OR_BRACE):
		{
			if (token.type == Token::OPERATOR) {
				parseStack.push_back(token);
				stateStack.push_back(PS_AFTER_OP);
				token = nextToken();
			}
			else if (token.type == Token::BRACE_CLOSE) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_CLOSED);
				token = nextToken();
			}
			else { //try error recovery
				done = true;
			}
			break;
		}
		case (PS_AFTER_OP):
		{
			if (token.type == Token::STRING) {
				parseStack.push_back(token);
				stateStack.push_back(PS_TERMINAL);
				token = nextToken();
			}
			else if (token.type == Token::BRACE_OPEN) {
				parseStack.push_back(token);
				stateStack.push_back(PS_BRACE_OPENED);
				token = nextToken();
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
			parseStack.push_back(token); parseStack.back().type = Token::NT_SA;
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
			OpNode * opNode = new OpNode(parseStack.back().op); parseStack.pop_back();
			Node * leftOperand = nodeStack.back(); nodeStack.pop_back(); parseStack.pop_back();
			opNode->children.push_back(leftOperand);
			opNode->children.push_back(rightOperand);
			nodeStack.push_back(opNode);
			stateStack.pop_back(); stateStack.pop_back(); stateStack.pop_back();
			parseStack.push_back(token); parseStack.back().type = Token::NT_SA;
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

void SetOpsOpTreeParser::sanitize() {
	int braceCount = 0;
	bool beforeWasOp = false;
	std::string ostr = "";
	for(std::string::const_iterator it = m_parseString.begin(); it != m_parseString.end(); ++it) {
		if (*it == '\\') {
			++it;
			if (it != m_parseString.end()) {
				ostr += '\\';
				ostr += *it;
			}
			else
				break;
		}
		else if (*it == ')') {
			if (braceCount <= 0) {
				continue;
			}
			else {
				--braceCount;
				ostr += *it;
			}
		}
		else if (m_opMap.count(*it) > 0) {
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
	m_parseString = ostr;
}

void SetOpsOpTreeParser::readString(Token & token) {
	token.begin = m_strIt - m_parseString.begin();
	token.type = Token::STRING;
	while (m_strIt != m_strEnd) {
		if (*m_strIt == '\\') {
			++m_strIt;
			if (m_strIt != m_strEnd) {
				token.tokenString += *m_strIt;
				++m_strIt;
			}
			else
				break;
		}
		else if (m_opMap.count(*m_strIt) > 0 || *m_strIt == ' ' || *m_strIt == '(' || *m_strIt == ')')
			break;
		else {
			token.tokenString += *m_strIt;
			++m_strIt;
		}
	}
	token.end = m_strIt - m_parseString.begin();
}

void SetOpsOpTreeParser::createDefaultOp(Token & token) const {
	token.begin = m_strIt - m_parseString.cbegin();
	token.tokenString = "";
	token.end = token.begin;
	token.type = Token::OPERATOR;
	token.op = OT_INTERSECT;
}

void SetOpsOpTreeParser::initTokenizer(const std::string & parseString) {
	m_parseString = parseString;
	sanitize();
	m_strIt = m_parseString.cbegin();
}

SetOpsOpTreeParser::Token SetOpsOpTreeParser::nextToken() {
	Token token;
	token.type = Token::ENDOFINPUT;
	while(m_strIt != m_strEnd) {
		if (*m_strIt == ' ') {
			++m_strIt;
		}
		else if (m_opMap.count(*m_strIt) > 0) {
			token.begin = m_strIt - m_parseString.begin();
			token.tokenString += *m_strIt;
			token.type = Token::OPERATOR;
			token.op = m_opMap.at(*m_strIt).type;
			++m_strIt;
			token.end = m_strIt - m_parseString.begin();
			m_beforeWasTerminal = false;
			break;
		}
		else if (*m_strIt == '(') {
			if (m_beforeWasTerminal) {
				createDefaultOp(token);
				m_beforeWasTerminal = false;
			}
			else {
				token.begin = m_strIt - m_parseString.begin();
				token.tokenString += *m_strIt;
				token.type = Token::BRACE_OPEN;
				++m_strIt;
				token.end = m_strIt - m_parseString.begin();
			}
			break;
		}
		else if (*m_strIt == ')') {
			token.begin = m_strIt - m_parseString.begin();
			token.tokenString += *m_strIt;
			token.type = Token::BRACE_CLOSE;
			++m_strIt;
			token.end = m_strIt - m_parseString.begin();
			m_beforeWasTerminal = true;
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

bool SetOpsOpTreeParser::hasNextToken() {
	return m_strIt != m_parseString.cend();
}

SetOpsOpTreeParser::SetOpsOpTreeParser() {
	m_opMap['+'] = OpDesc(OT_UNITE, 2);
	m_opMap['-'] = OpDesc(OT_DIFF, 2);
	m_opMap['/'] = OpDesc(OT_INTERSECT, 2);
	m_opMap[' '] = OpDesc(OT_INTERSECT, 2);
	m_opMap['^'] = OpDesc(OT_SYM_DIFF, 2);
}

}//end namespace detail

}}//end namespace