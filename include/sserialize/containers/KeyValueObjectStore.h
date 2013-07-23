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
		bool operator<(const KeyValue & other) const {
			return (key < other.key ? true : (key > other.key ? false : (value < other.value)));
		}
	};
	typedef std::vector< KeyValue > ItemData;

	class Item {
		const KeyValueObjectStore * m_store;
		const ItemData * m_item;
	protected:
		inline const KeyValueObjectStore * store() const { return m_store; }
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
	typedef std::vector<ItemData> ItemDataContainer;
private:
	KeyStringTable m_keyStringTable;
	ValueStringTable m_valueStringTable;
	ItemDataContainer m_items;
private:
	uint32_t keyId(const std::string & str);
	uint32_t valueId(const std::string & str);
	void serialize(const sserialize::KeyValueObjectStore::ItemData & item, sserialize::UByteArrayAdapter & dest);
	KeyValueObjectStore & operator=(const KeyValueObjectStore & other);
public:
	KeyValueObjectStore();
	virtual ~KeyValueObjectStore();
	void clear();
	uint32_t size() const { return m_items.size(); }
	inline Item at(uint32_t pos) const { return Item(this, &m_items.at(pos)); }
	void push_back(const std::vector< std::pair< std::string, std::string > > & extItem);
	
	///This remaps the strings ids ot the items and orders the keys of the item in ascending order
	void sort();
	
	///sort() has to be  called before using this!
		UByteArrayAdapter::OffsetType serialize(sserialize::UByteArrayAdapter & dest);
	std::pair<std::string, std::string> keyValue(uint32_t keyId, uint32_t valueId) const;
	///@param reorderMap maps new positions to old positions
	template<typename T_REORDER_MAP>
	void reorder(const T_REORDER_MAP & reorderMap) {
		sserialize::reorder(m_items, reorderMap);
	}
};


}//end namespace

#endif