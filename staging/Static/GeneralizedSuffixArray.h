#ifndef SSERIALIZE_STATIC_GENERALIZED_SUFFIX_ARRAY_H
#define SSERIALIZE_STATIC_GENERALIZED_SUFFIX_ARRAY_H
#include <sserialize/Static/StringTable.h>
#include <sserialize/Static/Pair.h>
#include <sserialize/Static/Set.h>
#include <sserialize/completers/StringCompleter.h>

namespace sserialize {
namespace Static {

/* FileFormat
 *
 *-----------------------------------------------
 *VERSION|FLAGS|StringTable|GSA|ValueStore
 *-----------------------------------------------
 * 1 byte|1byte|    *      | * |    *
 *-----------------------------------------------
 *
 *FLAGS:
 *-------
 *00000sc
 *-------
 * s = suffix array, if not present, then this is a "prefix-array"
 * c = case-sensitive
 *
 */


class GeneralizedSuffixArray {
public:
	enum Flags {F_NONE=0, F_CASE_SENSITIVE=1, F_SUFFIX_ARRAY=2};
private:
	typedef Array< Pair<ItemIndex, ItemIndex>  > ValueStore;
	typedef Array< Pair<uint32_t, uint16_t> > GSAStore;
private:
	Flags m_flags;
	StringTable m_stable;
	ValueStore m_vStore;
	GSAStore m_gsa;
private:
	/** @return -1 if strA is smaller than strB, 0 if equal, 1 if strA is larger than strB 
	  * @param lcp offset from beginning of strA/strB, lcp is updated accordingly
	  */
	int8_t compare(const sserialize::UByteArrayAdapter& strA, const std::string& strB, uint16_t& lcp) const;
	
	UByteArrayAdapter gsaStringAt(uint32_t pos) const;

	/** @return position of the first matching string */
	int32_t lowerBound(const std::string& str, sserialize::StringCompleter::QuerryType qt) const;
	/** @return position one after the last matching string */
	int32_t upperBound(const std::string& str, sserialize::StringCompleter::QuerryType qt) const;
public:
	GeneralizedSuffixArray();
	GeneralizedSuffixArray(const UByteArrayAdapter & data);
	~GeneralizedSuffixArray();
	std::unordered_set<uint32_t> match(const std::string & str, sserialize::StringCompleter::QuerryType qt) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;
};


}}//end namespace


#endif