#include <iostream>
#include <stack>
#include <sserialize/containers/SetOpTreePrivate.h>

/** Grammar:
/*  Sample Strings: Stutt and Stei = Stutt Stei = (Stutt) and (Stei) = (Stutt and Stei)
 * 
 *  CompletionStatement := (CompletionStatement) | AND_STM | OR_STM | XOR_STM | WO_STM | CMD_STM | STRING
 *  AND_STM := CompletionStatement AND_TOKEN CompletionStatement
 *  OR_STM := CompletionStatement OR_TOKEN CompletionStatement
 *  XOR_STM := CompletionStatement XOR_TOKEN CompletionStatement
 *  WO_STM := CompletionStatement WO_TOKEN CompletionStatement
 *  CMD_STM := CMD_TOKEN(String)
 *  STRING := String
 *  
 */


namespace sserialize {

class QueryStringParser {
enum ParserStates {
	PS_INIT, PS_AND, PS_OR, PS_XOR, PS_WO, PS_CMD, PS_COMPLETION, PS_EOF, PS_ERROR, PS_MAXNUM
};

typedef bool (QueryStringParser::*ActionFunctionType)(void);

public:
	std::string m_queryStr;
	ParserStates transitionTable[PS_MAXNUM][PS_MAXNUM];
	ActionFunctionType actionTable[PS_MAXNUM][PS_MAXNUM];
	
	
//Parsing Variables
	SetOpTreePrivate::Node * currentNode = 0;
	std::stack<ParserStates> stateStack;
	ParserStates currentState = PS_INIT;

	
public:
	
	QueryStringParser(std::string qstr) : m_queryStr(qstr) {}
	~QueryStringParser() {}
	void fillTransitionTable() {
		for(size_t i = 0; i < PS_MAXNUM; i++) {
			for(size_t j = 0; j < PS_MAXNUM; j++) {
				transitionTable[i][j] = PS_ERROR;
			}
		}
		transitionTable[PS_INIT][PS_COMPLETION] = PS_COMPLETION;
		transitionTable[PS_INIT][PS_CMD] = PS_CMD;

		transitionTable[PS_COMPLETION][PS_AND] = PS_AND;
		transitionTable[PS_COMPLETION][PS_OR] = PS_OR;
		transitionTable[PS_COMPLETION][PS_XOR] = PS_XOR;
		transitionTable[PS_COMPLETION][PS_WO] = PS_WO;
		
		transitionTable[PS_CMD][PS_AND] = PS_AND;
		transitionTable[PS_CMD][PS_OR] = PS_OR;
		transitionTable[PS_CMD][PS_XOR] = PS_XOR;
		transitionTable[PS_CMD][PS_WO] = PS_WO;
		
		transitionTable[PS_AND][PS_XOR] = PS_XOR;
	}
	
	void actionDispatch(ParserStates nextState) {
		
	}

SetOpTreePrivate::Node * createTreeFromQueryString(std::string qstr) {

	currentNode = 0;
	stateStack = std::stack<ParserStates>();
	currentState = PS_INIT;
	
	
	
	%%{
		machine qstr_main;

		action actAnd { actionDispatch(PS_AND); }
		action actOr { actionDispatch(PS_OR); }
		action actXor { actionDispatch(PS_XOR); }
		action actWo { actionDispatch(PS_WO); }
		action actComp { actionDispatch(PS_COMPLETION); }
		action actCmd { actionDispatch(PS_CMD); }
		action done { actionDispatch(PS_EOF); }
		
		andStr = ('/') 0 @actAnd;
		orStr = ('+') 0 @actOr;
		xorStr = ('^') 0 @actXor;
		woStr = ('-') 0 @actWo;
		actComp = string @actComp;
		actCmd = '\' string '(' string ')' 0 @actCmd;

		LogicStatement = (andStr | orStr | xorStr | woStr)

		SetOperation = ( SetOperation LogicStatement SetOperation ) | actComp | actCmd;
	
		main := (SetOperation) 0 @

	}%%

	
}
	
	
};

}//end namespace
