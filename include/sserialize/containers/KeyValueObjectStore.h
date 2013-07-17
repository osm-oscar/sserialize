#ifndef SSERIALIZE_KEY_VALUE_STORE_H
#define SSERIALIZE_KEY_VALUE_STORE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/StringTable.h>
namespace sserialize {

class KeyValueObjectStore {
public:
	struct KeyValue {
		KeyValue(uint32_t key, uint32_t value) : key(key), value(value) {}
		KeyValue() {}
		uint32_t key;
		uint32_t value;
	};
	typedef std::vector< KeyValue > ItemData;

	class Item {
		const KeyValueObjectStore * m_store;
		const ItemData * m_item;
	public:
		Item(const KeyValueObjectStore * store, const ItemData * item) : m_store(store), m_item(item) {}
		~Item() {}
		inline uint32_t size() const { return m_item->size(); }
		inline std::pair<std::string, std::string> at(uint32_t pos) const {
			return m_store->keyValue((*m_item)[pos].key, (*m_item)[pos].value);
		}
	};
	
private:
	typedef sserialize::StringTable KeyStringTable;
	typedef sserialize::StringTable ValueStringTable;
private:
	KeyStringTable m_keyStringTable;
	ValueStringTable m_valueStringTable;
	std::vector<ItemData> m_items;
private:
	uint32_t keyId(const std::string & str);
	uint32_t valueId(const std::string & str);
	void serialize(const sserialize::KeyValueObjectStore::ItemData & item, sserialize::UByteArrayAdapter & dest);
protected:
	
public:
	KeyValueObjectStore();
	virtual ~KeyValueObjectStore();
	uint32_t size() const { return m_items.size(); }
	inline Item at(uint32_t pos) const { return Item(this, &m_items.at(pos)); }
	void push_back(const std::vector< std::pair< std::string, std::string > > & extItem);
	void serialize(sserialize::UByteArrayAdapter & dest);
	std::pair<std::string, std::string> keyValue(uint32_t keyId, uint32_t valueId) const;
	void reorder(const std::unordered_map<uint32_t, uint32_t> & reorderMap);
};


}//end namespace

#endif