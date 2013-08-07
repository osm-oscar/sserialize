#include <sserialize/Static/DynamicKeyValueObjectStore.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <sserialize/utility/ProgressInfo.h>

namespace sserialize {
namespace Static {

DynamicKeyValueObjectStore::DynamicKeyValueObjectStore() : 
m_items(0, 0)
{}

DynamicKeyValueObjectStore::~DynamicKeyValueObjectStore() {}

void DynamicKeyValueObjectStore::clear() {
	m_items = ItemDataContainer(0, 0);
}

void DynamicKeyValueObjectStore::reserve(uint32_t totalItemCount, uint64_t totalItemStrings) {
	m_items = ItemDataContainer(totalItemCount, (uint64_t)totalItemCount*15+4*totalItemStrings);
}

uint32_t DynamicKeyValueObjectStore::keyId(const std::string & str) const {
	return m_keyStringTable.at(str);
}

uint32_t DynamicKeyValueObjectStore::valueId(const std::string & str) const {
	return m_valueStringTable.at(str);
}

uint32_t DynamicKeyValueObjectStore::addKey(const std::string & keyStr) {
	return m_keyStringTable.insert(keyStr);
}

uint32_t DynamicKeyValueObjectStore::addValue(const std::string & valueStr) {
	return m_valueStringTable.insert(valueStr);
}

///This has to be called before adding items, all used item strings have to be added before calling this
void DynamicKeyValueObjectStore::finalizeStringTables() {
	m_keyStringTable.sort();
	m_valueStringTable.sort();
	m_finalizedStringTables = true;
}

///You have to call finalizeStringTables() before adding items
void DynamicKeyValueObjectStore::push_back(const std::vector< std::pair< std::string, std::string > > & extItem) {
	ItemDataPush item;
	for(const std::pair<std::string, std::string> & kv : extItem) {
		item.push_back(KeyValue(keyId(kv.first), valueId(kv.second)));
	}
	m_items.push_back(item, m_itemSerializer);
}

UByteArrayAdapter::OffsetType DynamicKeyValueObjectStore::serialize(sserialize::UByteArrayAdapter & dest) const {
	UByteArrayAdapter::OffsetType dataBegin = dest.tellPutPtr();
	dest.putUint8(0); //Version
	sserialize::TimeMeasurer tm;
	tm.begin();
	std::cout << "DynamicKeyValueObjectStore::serialize: Serializing string tables..." << std::flush;
	m_keyStringTable.serialize(dest);
	m_valueStringTable.serialize(dest);
	tm.end();
	std::cout << "took " << tm.elapsedSeconds() << " seconds" << std::endl;

	Static::DequeCreator<UByteArrayAdapter> creator(dest);
	
	sserialize::ProgressInfo pinfo;
	pinfo.begin(m_items.size(), "KeyValueObjectStore::serialize: items");
	for(uint32_t i = 0, s = m_items.size(); i < s; ++i) {
		creator.beginRawPut();
		creator.rawPut().put(m_items.dataAt(i));
		creator.endRawPut();
		pinfo(i);
	}
	pinfo.end();
	creator.flush();
	return dest.tellPutPtr()-dataBegin;
}

std::pair<std::string, std::string> DynamicKeyValueObjectStore::keyValue(uint32_t keyId, uint32_t valueId) const {
	return std::pair<std::string, std::string>(m_keyStringTable.at(keyId), m_valueStringTable.at(valueId));
}


}}//end namespace