#include <sserialize/containers/SetOpTreePrivateSimple.h>
#include <iostream>

%%{

	machine simple_setoptree;
	
	action setTs {
		ts = p;
	}
	
	action setSubtract {
		subtract = true;
	}
	
	action setExactMatch {
		matchType |= sserialize::StringCompleter::QT_EXACT;
	}
	
	action setPrefixMatch {
		matchType |= sserialize::StringCompleter::QT_PREFIX;
	}
	
	action setSuffixMatch {
		matchType |= sserialize::StringCompleter::QT_SUFFIX;
	}
	
	action checkMatchType {
		if (matchType == sserialize::StringCompleter::QT_NONE ||
		(matchType & sserialize::StringCompleter::QT_PREFIX && matchType & sserialize::StringCompleter::QT_SUFFIX)) {
			matchType = sserialize::StringCompleter::QT_SUFFIX_PREFIX;
		}
	}
	
	action apC {
		curToken += *(p-1);
	}
	
	action efBegin {
		std::string efName(ts, p);
		if (m_ef.count(efName) > 0)
			curEf = m_ef[efName];
	}
	
	action qsEnd {
		QueryStringDescription qsd(curToken);
		bool valid = true;
		if (matchType != sserialize::StringCompleter::QT_NONE) {
			qsd.qt() = matchType;
		}
		else if (curEf.priv()) {
			qsd.ef() = curEf;
		}
		else {
			valid = false;
		}
		if (valid && (qsd.ef().priv() != 0 || qsd.str().size() >= m_minStrLen)) {
			if (subtract) {
				m_diffStrings.push_back(qsd);
			}
			else {
				m_intersectStrings.push_back(qsd);
			}
		}
		curToken = "";
		matchType = sserialize::StringCompleter::QT_NONE;
		subtract = false;
	}
	
	action errorHandler {
		std::cout << "Error occured at " << (p-qStr.c_str()) << " with token " << *p << std::endl;
		
		curToken.clear();
		matchType = sserialize::StringCompleter::QT_NONE;
		subtract = false;
		fhold;
		fgoto eEatWS;
	}
	
	myspace = (space | "\n");
	eEatWS := ('\\' . any | (any -- ('\\'|space)))* @{ fgoto main; };
	exactChar = ('\\' . any %apC | (any -- ('\\'| '"')) %apC );
	spChar = ('\\' . any %apC | (any -- ('\\'| '*'| myspace)) %apC );
	efChar = ('\\' . any %apC | (any -- ('\\'| ']')) %apC );
	exactMatch = '"' exactChar+ '"' %setExactMatch;
	spMatch = (('*'%setSuffixMatch)? spChar+ ('*'%setPrefixMatch)?) %checkMatchType;
	efMatch = '$' . alnum+ >setTs %efBegin . '[' efChar* ']';
	queryString = ('-' %setSubtract)? <: space* . ((exactMatch | efMatch) $(eefPrio, 1) | spMatch >(eefPrio, 0) );
	main :=  ((queryString %qsEnd . myspace+)* . queryString % qsEnd . myspace*) @err(errorHandler);
	
	write data;
}%%

void sserialize::SetOpTreePrivateSimple::ragelParse(const std::string& qStr) {
	const char * p = qStr.c_str();
	const char * pe = p+ qStr.size();
	const char * eof = pe;
	const char * ts = p;
	int cs;
	std::string curToken = "";
	bool subtract = false;
	int matchType = sserialize::StringCompleter::QT_NONE;
	RCPtrWrapper<SetOpTree::SelectableOpFilter> curEf;
	
	%% write init;
	
	%% write exec;
}