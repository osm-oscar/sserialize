#ifndef SSERIALIZE_KEY_VALUE_STORE_H
#define SSERIALIZE_KEY_VALUE_STORE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <unordered_map>
namespace sserialize {

class KeyValueObjectStore {
public:
	struct KeyValue {
		KeyValue(uint32_t key, uint32_t value) : key(key), value(value) {}
		KeyValue() {}
		uint32_t key;
		uint32_t value;
	};
	typedef std::vector< KeyValue > Item;
private:
	typedef std::unordered_map<std::string, uint32_t> KeyStringTable;
	typedef std::unordered_map<std::string, uint32_t> ValueStringTable;
private:
	std::unordered_map<std::string, uint32_t> m_keyStringTable;
	std::unordered_map<std::string, uint32_t> m_valueStringTable;
	std::vector<Item> m_items;
private:
	uint32_t keyId(const std::string & str);
	uint32_t valueId(const std::string & str);
	void serialize(const Item & item, UByteArrayAdapter & dest);
public:
	KeyValueObjectStore();
	virtual ~KeyValueObjectStore();
	void add(const std::vector< std::pair<std::string, std::string> > & item);
	void serialize(sserialize::UByteArrayAdapter & dest);
};


}//end namespace

#endif