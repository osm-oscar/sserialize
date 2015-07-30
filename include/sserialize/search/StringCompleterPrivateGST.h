#ifndef SSERIALIZE_STRING_COMPLETER_PRIVATE_GST_H
#define SSERIALIZE_STRING_COMPLETER_PRIVATE_GST_H
#include <sserialize/search/StringCompleterPrivate.h>
#include <sserialize/containers/GeneralizedTrie.h>

namespace sserialize {

template<typename IndexStorageContainer>
class StringCompleterPrivateGST: public StringCompleterPrivate {
	GeneralizedTrie::BaseTrie<IndexStorageContainer> * m_trie;
public:
	StringCompleterPrivateGST() : StringCompleterPrivate(), m_trie(0) {}
	StringCompleterPrivateGST(GeneralizedTrie::BaseTrie<IndexStorageContainer> * trie) : StringCompleterPrivate(), m_trie(trie) {}
	virtual ~StringCompleterPrivateGST() {}
	
	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const;
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const {
		return m_trie->complete(str, qtype);
	}

};

template<typename IndexStorageContainer>
StringCompleter::SupportedQuerries
StringCompleterPrivateGST<IndexStorageContainer>::getSupportedQuerries() const {
	if (!m_trie)
		return StringCompleter::SQ_NONE;
	
	unsigned int sq = StringCompleter::SQ_CASE_INSENSITIVE;
	if (m_trie->isSuffixTrie()) {
		sq |= StringCompleter::SQ_EPSP;
	}
	else {
		sq |= StringCompleter::SQ_EP;
	}
	
	if (m_trie->isCaseSensitive()) {
		sq |= StringCompleter::SQ_CASE_SENSITIVE;
	}
	
	return (StringCompleter::SupportedQuerries) sq;
}

}//end namespace


#endif
