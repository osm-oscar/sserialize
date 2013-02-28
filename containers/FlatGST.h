#ifndef SSERIALIZE_SUFFIX_ARRAY_H
#define SSERIALIZE_SUFFIX_ARRAY_H
#include <string>
#include <deque>
#include <set>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/containers/StringsItemDBWrapper.h>
#include <sserialize/containers/GeneralizedTrieHelpers.h>

namespace sserialize { namespace dynamic {


/** IDEA:
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  */

template<class ItemType>
class FlatGST: public StringCompleterPrivate {
private:
	typedef enum {SC_SMALLER, SC_EQUAL, SC_LARGER} StringComparisonType;
	struct FlatGSTEntry {
		uint32_t strId;
		uint16_t pos;
		uint16_t len;
		uint16_t lcp;
		std::deque<uint32_t> exactStrIds;
		std::deque<uint32_t> subStrIds;
		void swap(FlatGSTEntry & other) {
			std::swap(strId, other.strId);
			std::swap(pos, other.pos);
			std::swap(len, other.len);
			std::swap(lcp, other.lcp);
			exactStrIds.swap(other.exactStrIds);
			subStrIds.swap(other.subStrIds);
		}
	};
	
	class FlatGSTEntryComparator {
		const std::deque<std::string> & m_stringIdMap;
	public:
		FlatGSTEntryComparator(const std::deque<std::string> & stringIdMap) : m_stringIdMap(stringIdMap) {}
		~FlatGSTEntryComparator() {}
		bool operator()(const FlatGSTEntry & a, const FlatGSTEntry & b) {
			const std::string & aStr = m_stringIdMap.at(a.strId);
			const std::string & bStr = m_stringIdMap.at(b.strId);
			std::string::const_iterator aIt = a.begin();
			std::string::const_iterator aEnd = aIt + a.len;
			std::string::const_iterator bIt = b.begin();
			std::string::const_iterator bEnd = bIt + b.len;
			
			while (aIt != aEnd && bIt != bEnd) {
				if (*aIt == *bIt) {
					++aIt;
					++bIt;
				}
				else if (*aIt < *bIt)
					return true;
				else
					return false;
			}
			return !(bIt == bEnd);
		}
		
	};
	
	typedef std::set<FlatGST::FlatGSTEntry> FlatGSTConstructionSet;
private:
	StringsItemDBWrapper<ItemType> m_db;
	bool m_suffixTrie;
	bool m_caseSensitive;
	bool m_stringIds;
	std::deque<FlatGSTEntry> m_fgst;
	std::deque<std::string> m_strings;
private:
	bool insertIntoCsSet(FlatGSTConstructionSet & cs);
public:
	FlatGST() : StringCompleterPrivate(), m_suffixTrie(false), m_caseSensitive(false), m_stringIds(false) {}
	FlatGST(const StringsItemDBWrapper<ItemType> & db, bool suffixTrie, bool caseSensitive, bool stringIds);
	virtual ~FlatGST();
	bool create();
	
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const;
		
	virtual StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const { return std::string("dynamic::FlatGST"); }
	
	void serialize(FlatGSTConfig & config);
};

template<class ItemType>
FlatGST<ItemType>::FlatGST(const StringsItemDBWrapper< ItemType >& db, bool suffixTrie, bool caseSensitive, bool stringIds) :
StringCompleterPrivate(),
m_db(db),
m_suffixTrie(suffixTrie),
m_caseSensitive(caseSensitive),
m_stringIds(stringIds)
{}


template<class ItemType>
FlatGST<ItemType>::~FlatGST()
{}


/** This will insert all StringsIds from the db into cs */
template<class ItemType>
bool
FlatGST<ItemType>::insertIntoCsSet(FlatGSTConstructionSet & cs) {
	const std::map<unsigned int, std::string> & sourceStrings = m_db.strIdToStr();
	FlatGSTEntryComparator cmp(m_strings);
	for(std::map<unsigned int, std::string>::const_iterator srcIt = sourceStrings.begin(); srcIt != sourceStrings.end(); ++srcIt) {
		std::string curSrcString;
		if (m_caseSensitive)
			curSrcString = srcIt->second;
		else
			curSrcString = unicode_to_lower(srcIt->second);
		
		uint32_t curSrcStringId = m_strings.size();
		m_strings.push_back(curSrcString);
		bool keepString = false;
		
		FlatGSTEntry e;
		e.strId = curSrcStringId;
		e.pos = 0;
		e.len = curSrcString.length();
		typename FlatGSTConstructionSet::iterator csIt;// = cs.find(e, cmp);
		if (csIt != cs.end()) {
// 			csIt->exactStrIds.push_back(srcIt->first);
		}
		if (m_suffixTrie) {
			std::string::const_iterator strIt = curSrcString.begin();
			std::string::const_iterator strBegin = curSrcString.begin();
			std::string::const_iterator strEnd = curSrcString.end();
			
		}
		
		
		if (!keepString)
			m_strings.pop_back();
	}
}

template<class ItemType>
bool
FlatGST<ItemType>::create() {
	FlatGSTConstructionSet cs;
	if (!insertIntoCsSet(cs))
		return false;
	
	
	typename FlatGSTConstructionSet::iterator csIt = cs.begin();
	while(cs.size()) {
		m_fgst.push_back(*csIt);
// 		m_fgst.back().swap(*csIt);
		cs.erase(csIt++);
	}
	return true;
}

template<class ItemType>
ItemIndex
FlatGST<ItemType>::complete(const std::string & str, StringCompleter::QuerryType qt) const {
	return ItemIndex();
}

template<class ItemType>
StringCompleter::SupportedQuerries
FlatGST<ItemType>::getSupportedQuerries() const {
	uint8_t sq = StringCompleter::SQ_EP;
	if (m_caseSensitive)
		sq |= StringCompleter::SQ_CASE_SENSITIVE;
	else
		sq |= StringCompleter::SQ_CASE_INSENSITIVE;
	
	if (m_suffixTrie)
		sq |= StringCompleter::SQ_SSP;
	return (StringCompleter::SupportedQuerries) sq;
}

template<class ItemType>
std::ostream&
FlatGST<ItemType>::printStats(std::ostream& out) const {
	return out;
}


}}//end namespaces

#endif