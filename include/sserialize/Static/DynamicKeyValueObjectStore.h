#ifndef SSERIALIZE_STATIC_DYNAMIC_KEY_VALUE_STORE_H
#define SSERIALIZE_STATIC_DYNAMIC_KEY_VALUE_STORE_H
#include <sserialize/containers/StringTable.h>
#include <sserialize/Static/DynamicVector.h>
#include <sserialize/Static/KeyValueObjectStore.h>
#include <sserialize/containers/KeyValueObjectStore.h>

namespace sserialize {
namespace Static {

class DynamicKeyValueObjectStore {
public:
	typedef sserialize::KeyValueObjectStore::KeyValue KeyValue;
	typedef sserialize::KeyValueObjectStore::ItemData ItemDataPush;
	typedef Static::KeyValueObjectStoreItemBase ItemDataGet;

	class Item: public Static::KeyValueObjectStoreItemBase {
		const DynamicKeyValueObjectStore * m_store;
	protected:
		inline const DynamicKeyValueObjectStore * store() const { return m_store; }
	public:
		Item() : m_store(0) {}
		Item(const DynamicKeyValueObjectStore * store, const UByteArrayAdapter & data) : Static::KeyValueObjectStoreItemBase(data), m_store(store) {}
		~Item() {}
		inline std::pair<std::string, std::string> at(uint32_t pos) const {
			return store()->keyValue(keyId(pos), valueId(pos));
		}
	};
	
protected:
	typedef sserialize::StringTable KeyStringTable;
	typedef sserialize::StringTable ValueStringTable;
	typedef sserialize::Static::DynamicVector<ItemDataPush, ItemDataGet> ItemDataContainer;
	struct ItemDataPushSerializer {
		inline UByteArrayAdapter & operator()(const ItemDataPush & data, UByteArrayAdapter & dest) const {
			sserialize::KeyValueObjectStore::serialize(data, dest);
			return dest;
		}
	};
	friend struct ItemDataPushSerializer;
private:
	KeyStringTable m_keyStringTable;
	ValueStringTable m_valueStringTable;
	ItemDataContainer m_items;
	bool m_finalizedStringTables;
	ItemDataPushSerializer m_itemSerializer;
private:
	uint32_t keyId(const std::string & str) const;
	uint32_t valueId(const std::string & str) const;
	DynamicKeyValueObjectStore & operator=(const DynamicKeyValueObjectStore & other);
protected:
	const KeyStringTable & keyStringTable() const { return m_keyStringTable; }
	const ValueStringTable & valueStringTable() const { return m_valueStringTable; }
public:
	DynamicKeyValueObjectStore();
	virtual ~DynamicKeyValueObjectStore();
	void reserve(uint32_t totalItemCount, uint64_t totalItemStrings);
	void clear();
	uint32_t addKey(const std::string & keyStr);
	uint32_t addValue(const std::string & valueStr);
	///This has to be called before adding items, all used item strings have to be added before calling this
	void finalizeStringTables();
	inline uint32_t size() const { return m_items.size(); }
	inline Item at(uint32_t pos) const { return Item(this, m_items.dataAt(pos)); }
	///You have to call finalizeStringTables() and reserve() before adding items
	void push_back(const std::vector< std::pair< std::string, std::string > > & extItem);
	///TODO:implement the reorder mapper
	UByteArrayAdapter::OffsetType serialize(sserialize::UByteArrayAdapter & dest) const;
	std::pair<std::string, std::string> keyValue(uint32_t keyId, uint32_t valueId) const;
};


}}//end namespace

#endif