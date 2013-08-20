#ifndef SSERIALIZE_STATIC_STRING_TABLE_H
#define SSERIALIZE_STATIC_STRING_TABLE_H
#include <unordered_set>
#include <sserialize/Static/Deque.h>
#include <sserialize/completers/StringCompleter.h>

namespace sserialize {
namespace Static {

/** Abstracts access to a string table,
 * strings should be in the order of most usage,
 * so that the smallest ids are most frequently used
 */
class StringTable: public RCWrapper< sserialize::Static::Deque<std::string> > {
protected:
	inline UByteArrayAdapter & data() { return priv()->data();}
	inline const UByteArrayAdapter & data() const { return priv()->data(); }
public:
    StringTable();
    StringTable(const UByteArrayAdapter& data);
    StringTable(Static::Deque< std::string > * data);
    StringTable(const StringTable & other);
	virtual ~StringTable();
	StringTable & operator=(const StringTable & other);
	/** @param qtype type of the query as */
	std::unordered_set< uint32_t > find(const std::string& searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	void find(std::unordered_set< uint32_t >& ret, std::string searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	bool match(uint32_t stringId, const std::string & searchStr, sserialize::StringCompleter::QuerryType qtype) const;
	/** strings should be sorted in usage frequency, strid will be the position **/
	static bool create(UByteArrayAdapter & destination, const std::map<unsigned int, std::string> & strs);
	std::ostream& printStats(std::ostream & out) const;
	
	inline uint32_t size() const { return priv()->size();}
	inline uint32_t getSizeInBytes() const { return priv()->getSizeInBytes();}
	inline std::string at(uint32_t pos) const { return priv()->at(pos); }
	inline UByteArrayAdapter dataAt(uint32_t pos) const { return priv()->dataAt(pos);}
	inline int32_t find(const std::string & value) const { return priv()->find(value);}
	inline bool count(const std::string & value) const { return find(value) >= 0; }
	inline std::string front() const { return priv()->front();}
	inline std::string back() const { return priv()->back();}
};

}}//end namespace

#endif