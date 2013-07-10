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


}}//end namespace