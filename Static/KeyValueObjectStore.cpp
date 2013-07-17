#include <sserialize/Static/KeyValueObjectStore.h>

namespace sserialize {
namespace Static {

KeyValueObjectStoreItem::KeyValueObjectStoreItem() {}

KeyValueObjectStoreItem::KeyValueObjectStoreItem(const std::shared_ptr< sserialize::Static::KeyValueObjectStorePrivate >& db, const sserialize::UByteArrayAdapter & data)
{}

KeyValueObjectStoreItem::~KeyValueObjectStoreItem() {}

uint32_t KeyValueObjectStoreItem::size() const {
	return m_size;
}

std::pair<std::string, std::string> KeyValueObjectStoreItem::at(uint32_t pos) const {
	return std::pair<std::string, std::string>(key(pos), value(pos));
}

std::string KeyValueObjectStoreItem::key(uint32_t pos) const {
	return m_db->key( keyId(pos) );
}

std::string KeyValueObjectStoreItem::value(uint32_t pos) const {
	return m_db->value( valueId(pos) );
}

uint32_t KeyValueObjectStoreItem::keyId(uint32_t pos) const {
	return m_keyBegin + m_kv.at(pos, 0);
}

uint32_t KeyValueObjectStoreItem::valueId(uint32_t pos) const {
	return m_valueBegin + m_kv.at(pos, 1);
}

uint32_t KeyValueObjectStoreItem::findKey(const std::string & str, uint32_t start) const {
	uint32_t keyId = m_db->findKeyId(str);
	if (keyId == KeyValueObjectStore::npos)
		return npos;
	return findKey(keyId, start);
}

uint32_t KeyValueObjectStoreItem::findKey(uint32_t id, uint32_t start) const {
	uint32_t s = size();
	for(uint32_t i = start; i < s; ++i) {
		if (keyId(i) == id)
			return i;
	}
	return npos;
}

uint32_t KeyValueObjectStoreItem::findValue(const std::string & str, uint32_t start) const {
	uint32_t valueId = m_db->findValueId(str);
	if (valueId == KeyValueObjectStore::npos)
		return npos;
	return findValue(valueId, start);
}

uint32_t KeyValueObjectStoreItem::findValue(uint32_t id, uint32_t start) const {
	uint32_t s = size();
	for(uint32_t i = start; i < s; ++i) {
		if (valueId(i) == id)
			return i;
	}
	return npos;
}


uint32_t KeyValueObjectStoreItem::countKey(const std::string & str) const {
	uint32_t keyId = m_db->findKeyId(str);
	if (keyId == KeyValueObjectStore::npos)
		return 0;
	return countKey(keyId);
}

uint32_t KeyValueObjectStoreItem::countKey(uint32_t id) const {
	uint32_t s = size();
	uint32_t count = 0;
	for(uint32_t i = 0; i < s; ++i) {
		if (keyId(i) == id)
			++count;
	}
	return count;
}

uint32_t KeyValueObjectStoreItem::countValue(const std::string & str) const {
	uint32_t valueId = m_db->findValueId(str);
	if (valueId == KeyValueObjectStore::npos)
		return 0;
	return countValue(valueId);
}


uint32_t KeyValueObjectStoreItem::countValue(uint32_t id) const {
	uint32_t s = size();
	uint32_t count = 0;
	for(uint32_t i = 0; i < s; ++i) {
		if (valueId(i) == id)
			++count;
	}
	return count;
}

bool KeyValueObjectStoreItem::matchValues(const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const {
	uint32_t s = size();
	for(uint32_t i = 0; i < s; ++i) {
		if (matchValue(i, query))
			return true;
	}
	return false;
}

bool KeyValueObjectStoreItem::matchValue(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const {
	return m_db->matchValue(valueId(pos), query);
}

KeyValueObjectStoreItem::const_iterator KeyValueObjectStoreItem::begin() const {
	return const_iterator(0, this);
}

KeyValueObjectStoreItem::const_iterator KeyValueObjectStoreItem::end() const {
	return const_iterator(size(), this);
}

KeyValueObjectStore::KeyValueObjectStore() : m_priv(new KeyValueObjectStorePrivate()) {}

KeyValueObjectStore::KeyValueObjectStore(const sserialize::UByteArrayAdapter & data) :
m_priv(new KeyValueObjectStorePrivate(data))
{}

KeyValueObjectStore::~KeyValueObjectStore() {}


uint32_t KeyValueObjectStore::size() const {
	return priv()->size();
}

UByteArrayAdapter::OffsetType KeyValueObjectStore::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

uint32_t KeyValueObjectStore::findKeyId(const std::string & str) const {
	return priv()->findKeyId(str);
}

uint32_t KeyValueObjectStore::findValueId(const std::string & str) const {
	return priv()->findValueId(str);
}

std::string KeyValueObjectStore::key(uint32_t id) const {
	return priv()->key(id);
}

std::string KeyValueObjectStore::value(uint32_t id) const {
	return priv()->value(id);
}

KeyValueObjectStoreItem KeyValueObjectStore::at(uint32_t pos) const {
	return KeyValueObjectStoreItem(priv(), priv()->dataAt(pos) );
}

KeyValueObjectStore::const_iterator KeyValueObjectStore::begin() const {
	return const_iterator(0, *this);
}

KeyValueObjectStore::const_iterator KeyValueObjectStore::end() const {
	return const_iterator(size(), *this);
}

bool KeyValueObjectStore::matchValues(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const {
	if (size() >= pos)
		return false;
	return at(pos).matchValues(query);
}

sserialize::StringCompleter::SupportedQuerries KeyValueObjectStore::getSupportedQuerries() const {
	return sserialize::StringCompleter::SQ_ALL;
}

KeyValueObjectStorePrivate::KeyValueObjectStorePrivate(){}

KeyValueObjectStorePrivate::KeyValueObjectStorePrivate(const UByteArrayAdapter & data) :
m_keyStringTable(data),
m_valueStringTable(data+m_keyStringTable.getSizeInBytes()),
m_items(data+(m_keyStringTable.getSizeInBytes()+m_valueStringTable.getSizeInBytes()))
{}

KeyValueObjectStorePrivate::~KeyValueObjectStorePrivate() {}

uint32_t KeyValueObjectStorePrivate::size() const {
	return m_items.size();
}

UByteArrayAdapter::OffsetType KeyValueObjectStorePrivate::getSizeInBytes() const {
	return m_keyStringTable.getSizeInBytes()+m_valueStringTable.getSizeInBytes()+m_items.getSizeInBytes();
}

uint32_t KeyValueObjectStorePrivate::findKeyId(const std::string & str) const {
	int32_t pos = m_keyStringTable.find(str);
	if (pos < 0) {
		return KeyValueObjectStore::npos;
	}
	return pos;
}

uint32_t KeyValueObjectStorePrivate::findValueId(const std::string & str) const {
	int32_t pos = m_valueStringTable.find(str);
	if (pos < 0) {
		return KeyValueObjectStore::npos;
	}
	return pos;
}

std::string KeyValueObjectStorePrivate::key(uint32_t id) const {
	return m_keyStringTable.at(id);
}


std::string KeyValueObjectStorePrivate::value(uint32_t id) const {
	return m_valueStringTable.at(id);
}

UByteArrayAdapter KeyValueObjectStorePrivate::dataAt(uint32_t pos) const {
	return m_items.at(pos);
}

bool KeyValueObjectStorePrivate::matchKey(uint32_t id, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const {
	return m_keyStringTable.match(id, query.first, query.second);
}

bool KeyValueObjectStorePrivate::matchValue(uint32_t id, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & query) const {
	return m_valueStringTable.match(id, query.first, query.second);
}

}}//end namespace