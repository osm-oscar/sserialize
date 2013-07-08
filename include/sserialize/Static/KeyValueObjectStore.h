#ifndef SSERIALIZE_STATIC_KEY_VALUE_OBJECT_STORE_H
#define SSERIALIZE_STATIC_KEY_VALUE_OBJECT_STORE_H
#include <sserialize/Static/StringTable.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <memory>

namespace sserialize {
namespace Static {

class KeyValueObjectStorePrivate;

class KeyValueObjectStoreItem {
public:
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
private:
	uint32_t m_size;
	uint32_t m_keyBegin;
	uint32_t m_valueBegin;
	MultiVarBitArray m_kv;
	std::shared_ptr<KeyValueObjectStorePrivate> m_db;
public:
	KeyValueObjectStoreItem();
	KeyValueObjectStoreItem(const std::shared_ptr<KeyValueObjectStorePrivate> & db, const UByteArrayAdapter & data);
	virtual ~KeyValueObjectStoreItem();
	uint32_t size() const;
	std::string key(uint32_t pos) const;
	std::string value(uint32_t pos) const;
	uint32_t keyId(uint32_t pos) const;
	uint32_t valueId(uint32_t pos) const;
};


class KeyValueObjectStore {
public:
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
private:
	std::shared_ptr<KeyValueObjectStorePrivate> m_priv;
	std::shared_ptr<KeyValueObjectStorePrivate> & priv();
	const std::shared_ptr<KeyValueObjectStorePrivate> & priv() const;
public:
	KeyValueObjectStore();
	KeyValueObjectStore(const sserialize::UByteArrayAdapter & data);
	virtual ~KeyValueObjectStore();
	uint32_t findKeyId(const std::string & str) const;
	uint32_t findValueId(const std::string & str) const;
	std::string key(uint32_t id) const;
	std::string value(uint32_t id) const;
	KeyValueObjectStoreItem at(uint32_t pos) const;
};


/** File layout
  *
  * Layout of a single Item
  *-------------------------------------------------
  *count |bpk|bpv|KeyBegin|ValueBegin|KeyValuePairs
  *------------------------------------------------
  *22 Bit|5b |5b|   v32   |   v32    | MultiVarBitArray
  *
  *
  *
  *
  */

class KeyValueObjectStorePrivate {
private:
	Static::StringTable m_keyStringTable;
	Static::StringTable m_valueStringTable;
	Static::Deque<UByteArrayAdapter> m_items;
public:
	KeyValueObjectStorePrivate();
	KeyValueObjectStorePrivate(const sserialize::UByteArrayAdapter & data);
	virtual ~KeyValueObjectStorePrivate();
	uint32_t findKeyId(const std::string & str) const;
	uint32_t findValueId(const std::string & str) const;
	std::string key(uint32_t id) const;
	std::string value(uint32_t id) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
};


}}//end namespace

#endif