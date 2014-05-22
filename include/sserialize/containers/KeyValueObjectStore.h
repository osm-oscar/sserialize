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
		uint32_t m_id;
	protected:
		inline const KeyValueObjectStore * store() const { return m_store; }
		inline uint32_t id() const { return m_id; }
		const ItemData & itemData() const { return store()->itemDataAt(id()); }
	public:
		Item(const KeyValueObjectStore * store, uint32_t id) : m_store(store), m_id(id) {}
		~Item() {}
		inline uint32_t size() const { return itemData().size(); }
		inline std::pair<std::string, std::string> at(uint32_t pos) const {
			return store()->keyValue(itemData()[pos].key, itemData()[pos].value);
		}
	};
	
protected:
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
	KeyValueObjectStore & operator=(const KeyValueObjectStore & other);
protected:
	const KeyStringTable & keyStringTable() const { return m_keyStringTable; }
	const ValueStringTable & valueStringTable() const { return m_valueStringTable; }
public:
	KeyValueObjectStore();
	virtual ~KeyValueObjectStore();
	void clear();
	///Pre-allocate space for items
	void reserve(uint32_t size);
	uint32_t size() const { return m_items.size(); }
	inline const ItemData & itemDataAt(uint32_t pos) const { return m_items[pos]; }
	inline Item at(uint32_t pos) const { return Item(this, pos); }
	void push_back(const std::vector< std::pair< std::string, std::string > > & extItem);
	
	///This remaps the strings ids ot the items and orders the keys of the item in ascending order
	void sort();
	
	///sort() has to be  called before using this!
	UByteArrayAdapter::OffsetType serialize(sserialize::UByteArrayAdapter & dest) const;
	std::pair<std::string, std::string> keyValue(uint32_t keyId, uint32_t valueId) const;
	///@param reorderMap maps new positions to old positions
	template<typename T_REORDER_MAP>
	void reorder(const T_REORDER_MAP & reorderMap) {
		sserialize::reorder(m_items, reorderMap);
	}
public:
	static void serialize(const sserialize::KeyValueObjectStore::ItemData & item, sserialize::UByteArrayAdapter & dest);
};


}//end namespace

#endif