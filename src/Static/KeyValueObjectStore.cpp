#include <sserialize/Static/KeyValueObjectStore.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/log.h>

namespace sserialize {
namespace Static {

KeyValueObjectStoreItemBase::KeyValueObjectStoreItemBase() :
m_sizeBPKV(0),
m_keyBegin(0),
m_valueBegin(0)
{}

KeyValueObjectStoreItemBase::KeyValueObjectStoreItemBase(sserialize::UByteArrayAdapter data) :
m_sizeBPKV(data.getVlPackedUint32()),
m_keyBegin(data.getVlPackedUint32()),
m_valueBegin(data.getVlPackedUint32()),
m_kv(data.shrinkToGetPtr(), keyBits() + valueBits())
{
	SSERIALIZE_LENGTH_CHECK(size(), m_kv.maxCount(), sserialize::toString("sserialize::KeyValueObjectStoreItemBase size()=", size(), " >= m_kv.maxCount()=", m_kv.maxCount()));
}

KeyValueObjectStoreItemBase::~KeyValueObjectStoreItemBase() {}

uint32_t KeyValueObjectStoreItemBase::keyId(uint32_t pos) const {
	return m_keyBegin + (m_kv.at64(pos) >> valueBits());
}

uint32_t KeyValueObjectStoreItemBase::valueId(uint32_t pos) const {
	return m_valueBegin + (m_kv.at64(pos) & valueMask());
}

uint32_t KeyValueObjectStoreItemBase::findKey(uint32_t id, uint32_t start) const {
	uint32_t s = size();
	for(uint32_t i = start; i < s; ++i) {
		if (keyId(i) == id)
			return i;
	}
	return npos;
}

uint32_t KeyValueObjectStoreItemBase::findValue(uint32_t id, uint32_t start) const {
	uint32_t s = size();
	for(uint32_t i = start; i < s; ++i) {
		if (valueId(i) == id)
			return i;
	}
	return npos;
}

uint32_t KeyValueObjectStoreItemBase::countKey(uint32_t id) const {
	uint32_t s = size();
	uint32_t count = 0;
	for(uint32_t i = 0; i < s; ++i) {
		if (keyId(i) == id)
			++count;
	}
	return count;
}

uint32_t KeyValueObjectStoreItemBase::countValue(uint32_t id) const {
	uint32_t s = size();
	uint32_t count = 0;
	for(uint32_t i = 0; i < s; ++i) {
		if (valueId(i) == id)
			++count;
	}
	return count;
}

KeyValueObjectStoreItem::KeyValueObjectStoreItem() {}

KeyValueObjectStoreItem::KeyValueObjectStoreItem(const sserialize::RCPtrWrapper< sserialize::Static::KeyValueObjectStorePrivate >& db, const sserialize::UByteArrayAdapter& data) :
KeyValueObjectStoreItemBase(data),
m_db(db)
{}

KeyValueObjectStoreItem::~KeyValueObjectStoreItem() {}

std::pair<std::string, std::string> KeyValueObjectStoreItem::at(uint32_t pos) const {
	return std::pair<std::string, std::string>(key(pos), value(pos));
}

std::string KeyValueObjectStoreItem::key(uint32_t pos) const {
	return m_db->key( keyId(pos) );
}

std::string KeyValueObjectStoreItem::value(uint32_t pos) const {
	return m_db->value( valueId(pos) );
}

std::string KeyValueObjectStoreItem::value(const std::string & key) const {
	uint32_t keyPos = findKey(key, 0);
	if (keyPos != npos) {
		return value(keyPos);
	}
	return std::string();
}

uint32_t KeyValueObjectStoreItem::findKey(const std::string & str, uint32_t start) const {
	uint32_t keyId = m_db->findKeyId(str);
	if (keyId == KeyValueObjectStore::npos)
		return npos;
	return KeyValueObjectStoreItemBase::findKey(keyId, start);
}

uint32_t KeyValueObjectStoreItem::findValue(const std::string & str, uint32_t start) const {
	uint32_t valueId = m_db->findValueId(str);
	if (valueId == KeyValueObjectStore::npos)
		return npos;
	return KeyValueObjectStoreItemBase::findValue(valueId, start);
}

uint32_t KeyValueObjectStoreItem::countKey(const std::string & str) const {
	uint32_t keyId = m_db->findKeyId(str);
	if (keyId == KeyValueObjectStore::npos)
		return 0;
	return KeyValueObjectStoreItemBase::countKey(keyId);
}

uint32_t KeyValueObjectStoreItem::countValue(const std::string & str) const {
	uint32_t valueId = m_db->findValueId(str);
	if (valueId == KeyValueObjectStore::npos)
		return 0;
	return KeyValueObjectStoreItemBase::countValue(valueId);
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

const KeyValueObjectStore::KeyStringTable & KeyValueObjectStore::keyStringTable() const {
	return m_priv->keyStringTable();
}

const KeyValueObjectStore::ValueStringTable & KeyValueObjectStore::valueStringTable() const {
	return m_priv->valueStringTable();
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

std::ostream & KeyValueObjectStore::printStats(std::ostream & out) const {
	return priv()->printStats(out);
}

KeyValueObjectStorePrivate::KeyValueObjectStorePrivate(){}

KeyValueObjectStorePrivate::KeyValueObjectStorePrivate(const UByteArrayAdapter & data) :
m_keyStringTable(data+1),
m_valueStringTable(data+(m_keyStringTable.getSizeInBytes()+1)),
m_items(data+(m_keyStringTable.getSizeInBytes()+m_valueStringTable.getSizeInBytes()+1))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_KEY_VALUE_OBJECTSTORE_VERSION, data.at(0), "sserialize::Static::KeyValueObjectStore");
}

KeyValueObjectStorePrivate::~KeyValueObjectStorePrivate() {}

uint32_t KeyValueObjectStorePrivate::size() const {
	return m_items.size();
}

UByteArrayAdapter::OffsetType KeyValueObjectStorePrivate::getSizeInBytes() const {
	return 1+m_keyStringTable.getSizeInBytes()+m_valueStringTable.getSizeInBytes()+m_items.getSizeInBytes();
}

uint32_t KeyValueObjectStorePrivate::findKeyId(const std::string & str) const {
	uint32_t pos = m_keyStringTable.find(str);
	if (pos == sserialize::Static::StringTable::npos) {
		return KeyValueObjectStore::npos;
	}
	return pos;
}

uint32_t KeyValueObjectStorePrivate::findValueId(const std::string & str) const {
	uint32_t pos = m_valueStringTable.find(str);
	if (pos == sserialize::Static::StringTable::npos) {
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

std::ostream & KeyValueObjectStorePrivate::printStats(std::ostream & out) const {
	out << "sserialize::Static::KeyValueObjectStore::printStats -- BEGIN" << std::endl;
	out << "Size: " << size() << std::endl;
	out << "#Key strings: " << m_keyStringTable.size() << std::endl;
	out << "#Value strings: " << m_valueStringTable.size() << std::endl;
	out << "Size of items: " << m_items.getSizeInBytes() << "(" << ((double)m_items.getSizeInBytes())/getSizeInBytes()*100.0 << "%)" << std::endl;
	out << "Size of key strings: " << m_keyStringTable.getSizeInBytes() << "(" << ((double)m_keyStringTable.getSizeInBytes())/getSizeInBytes()*100.0 << "%)" << std::endl;
	out << "Size of value strings: " << m_valueStringTable.getSizeInBytes() << "(" << ((double)m_valueStringTable.getSizeInBytes())/getSizeInBytes()*100.0 << "%)" << std::endl;
	out << "sserialize::Static::OsmKeyValueObjectStore::printStats -- END" << std::endl;
	return out;
}

}}//end namespace