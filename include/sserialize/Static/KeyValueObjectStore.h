#ifndef SSERIALIZE_STATIC_KEY_VALUE_OBJECT_STORE_H
#define SSERIALIZE_STATIC_KEY_VALUE_OBJECT_STORE_H
#include <sserialize/Static/StringTable.h>
#include <sserialize/utility/CompactUintArray.h>
#include <sserialize/utility/AtStlInputIterator.h>
#include <sserialize/completers/StringCompleter.h>
#include <memory>
#define SSERIALIZE_STATIC_KEY_VALUE_OBJECTSTORE_VERSION 0

namespace sserialize {
namespace Static {

class KeyValueObjectStorePrivate;

/** File layout
  *----------------------------------------------------------
  *VERSION|      KeyStrings |   ValueStrings   |Item
  *----------------------------------------------------------
  * 1 Byte|SortedStringTable|SortedStringTable |*
  *
  * Layout of a single Item
  *----------------------------------------------------
  *count |bpk|bpv|KeyBegin|ValueBegin|KeyValuePairs
  *----------------------------------------------------
  *22 Bit|5b |5b|   v32   |   v32    | CompactUintArray
  *
  *
  * The keys/values are sorted in ascending order
  *
  */
  
class KeyValueObjectStoreItemBase {
public:
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
protected:
	uint32_t m_sizeBPKV;
	uint32_t m_keyBegin;
	uint32_t m_valueBegin;
	CompactUintArray m_kv;
protected:
	inline uint8_t keyBits() const { return ((m_sizeBPKV >> 5) & 0x1F)+1;}
	inline uint8_t valueBits() const { return (m_sizeBPKV & 0x1F)+1;}
	inline uint32_t valueMask() const { return createMask(valueBits()); }
public:
	KeyValueObjectStoreItemBase();
	KeyValueObjectStoreItemBase(sserialize::UByteArrayAdapter data);
	virtual ~KeyValueObjectStoreItemBase();
	inline uint32_t size() const { return m_sizeBPKV >> 10; }
	uint32_t keyId(uint32_t pos) const;
	uint32_t valueId(uint32_t pos) const;
	uint32_t findKey(uint32_t id, uint32_t start = 0) const;
	uint32_t findValue(uint32_t id, uint32_t start = 0) const;
	uint32_t countKey(uint32_t id) const;
	uint32_t countValue(uint32_t id) const;
};


class KeyValueObjectStoreItem: public KeyValueObjectStoreItemBase {
public:
	typedef KeyValueObjectStoreItemBase MyBaseClass;
	typedef ReadOnlyAtStlIterator<const KeyValueObjectStoreItem*, std::pair<std::string, std::string> > const_iterator;
	typedef const_iterator iterator;
private:
	std::shared_ptr<KeyValueObjectStorePrivate> m_db;
public:
	KeyValueObjectStoreItem();
	KeyValueObjectStoreItem(const std::shared_ptr< sserialize::Static::KeyValueObjectStorePrivate >& db, const sserialize::UByteArrayAdapter & data);
	virtual ~KeyValueObjectStoreItem();
	std::pair<std::string, std::string> at(uint32_t pos) const;
	std::string key(uint32_t pos) const;
	std::string value(uint32_t pos) const;
	using MyBaseClass::findKey;
	using MyBaseClass::findValue;
	uint32_t findKey(const std::string & str, uint32_t start = 0) const;
	uint32_t findValue(const std::string & str, uint32_t start = 0) const;
	uint32_t countKey(const std::string & str) const;
	uint32_t countValue(const std::string & str) const;
	
	bool matchValues(const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const;
	bool matchValue(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const;
	const_iterator begin() const;
	const_iterator end() const;
};


class KeyValueObjectStore {
public:
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
	typedef ReadOnlyAtStlIterator<KeyValueObjectStore, KeyValueObjectStoreItem > iterator;
	typedef ReadOnlyAtStlIterator<KeyValueObjectStore, KeyValueObjectStoreItem > const_iterator;
	
	typedef sserialize::Static::SortedStringTable KeyStringTable;
	typedef sserialize::Static::SortedStringTable ValueStringTable;
private:
	std::shared_ptr<KeyValueObjectStorePrivate> m_priv;
	inline std::shared_ptr<KeyValueObjectStorePrivate> & priv() { return m_priv; }
	inline const std::shared_ptr<KeyValueObjectStorePrivate> & priv() const { return m_priv; }
public:
	KeyValueObjectStore();
	KeyValueObjectStore(const sserialize::UByteArrayAdapter & data);
	virtual ~KeyValueObjectStore();
	uint32_t size() const;
	const KeyStringTable & keyStringTable() const;
	const ValueStringTable & valueStringTable() const;
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t findKeyId(const std::string & str) const;
	uint32_t findValueId(const std::string & str) const;
	std::string key(uint32_t id) const;
	std::string value(uint32_t id) const;
	KeyValueObjectStoreItem at(uint32_t pos) const;
	const_iterator begin() const;
	const_iterator end() const;
	
	bool matchValues(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;
	
	std::ostream & printStats(std::ostream & out) const;
	
};

class KeyValueObjectStorePrivate {
public:
	typedef sserialize::Static::KeyValueObjectStore::ValueStringTable ValueStringTable;
	typedef sserialize::Static::KeyValueObjectStore::KeyStringTable KeyStringTable;
private:
	KeyStringTable m_keyStringTable;
	ValueStringTable m_valueStringTable;
	Static::Deque<UByteArrayAdapter> m_items;
public:
	KeyValueObjectStorePrivate();
	KeyValueObjectStorePrivate(const sserialize::UByteArrayAdapter & data);
	virtual ~KeyValueObjectStorePrivate();
	uint32_t size() const;
	inline const KeyStringTable & keyStringTable() const { return m_keyStringTable; }
	inline const ValueStringTable & valueStringTable() const { return m_valueStringTable; }
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	uint32_t findKeyId(const std::string & str) const;
	uint32_t findValueId(const std::string & str) const;
	std::string key(uint32_t id) const;
	std::string value(uint32_t id) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
	
	bool matchKey(uint32_t id, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const;
	bool matchValue(uint32_t id, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const;

	std::ostream & printStats(std::ostream & out) const;

};


}}//end namespace

#endif