#include <sserialize/completers/StringCompleterPrivateMulti.h>

namespace sserialize {

StringCompleterPrivateMulti::StringCompleterPrivateMulti() :
StringCompleterPrivate(),
m_sq(sserialize::StringCompleter::SQ_ALL)
{}

StringCompleterPrivateMulti::~StringCompleterPrivateMulti() {}


void StringCompleterPrivateMulti::addCompleter(const RCPtrWrapper<sserialize::StringCompleterPrivate> & completer) {
	m_sq = (sserialize::StringCompleter::SupportedQuerries)(m_sq & completer->getSupportedQuerries());
	m_completers.push_back(completer);
}

ItemIndex StringCompleterPrivateMulti::complete(const std::string & str, StringCompleter::QuerryType qtype) const {
	std::vector<ItemIndex> idxs;
	idxs.reserve(m_completers.size());
	ItemIndex idx;
	for(std::size_t i = 0; i < m_completers.size(); ++i) {
		idx = m_completers[i]->complete(str, qtype);
		if (idx.size())
			idxs.push_back(idx);
	}
	return ItemIndex::unite(idxs);
}

ItemIndexIterator StringCompleterPrivateMulti::partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	std::vector<ItemIndexIterator> idxs;
	idxs.reserve(m_completers.size());
	ItemIndexIterator idx;
	for(std::size_t i = 0; i < m_completers.size(); ++i) {
		idx = m_completers[i]->partialComplete(str, qtype);
		if (idx.maxSize())
			idxs.push_back(idx);
	}
	return ItemIndexIterator::createTree(idxs.begin(), idxs.end(), &createMergeItemIndexIterator);
}

StringCompleter::SupportedQuerries StringCompleterPrivateMulti::getSupportedQuerries() const {
	return m_sq;
}

std::ostream& StringCompleterPrivateMulti::printStats(std::ostream& out) const {
	out << "StringCompleterPrivateMulti::BEGIN" << std::endl;
	for(std::size_t i = 0; i < m_completers.size(); ++i) {
		out << m_completers[i]->printStats(out) << std::endl;
	}
	out << "StringCompleterPrivateMulti::END" << std::endl;
	return out;
}

std::string StringCompleterPrivateMulti::getName() const {
	std::string myName("StringCompleterPrivateMulti: ");
	for(std::size_t i = 0; i < m_completers.size(); ++i) {
		myName += m_completers[i]->getName();
		myName += ":";
	}
	return myName;
}


}//end namespace