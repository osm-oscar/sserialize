#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Deque.h>
#include <algorithm>
#include <sserialize/utility/exceptions.h>
#include "SetOpTreePrivateSimple_parser.rl"

namespace sserialize {

KeyValueObjectStore::KeyValueObjectStore() {}
KeyValueObjectStore::~KeyValueObjectStore() {}


uint32_t KeyValueObjectStore::keyId(const std::string & str) {
	KeyStringTable::const_iterator it = m_keyStringTable.find(str);
	if(it != m_keyStringTable.end()) {
			return *it;
		}
	else {
			uint32_t id = m_keyStringTable.size();
			m_keyStringTable[str] = id;
			return id;
		}
}

uint32_t KeyValueObjectStore::valueId(const std::string & str) {
	ValueStringTable::const_iterator it = m_valueStringTable.find(str);
	if(it != m_valueStringTable.end()) {
			return *it;
		}
	else {
			uint32_t id = m_valueStringTable.size();
			m_valueStringTable[str] = id;
			return id;
		}
}

void KeyValueObjectStore::add(const std::vector< std::pair<std::string, std::string> > & extItem) {
	m_items.push_back(Item());
	Item & item = m_items.back();
	item.reserve(extItem.size());
	for(const std::pair<std::string, std::string> & kv : extItem) {
		item.push_back(KeyValue(keyId(kv.first), valueId(kv.second)));
	}
}

template<typename T>
std::unordered_map<uint32_t, uint32_t> createRemap(const T & src) {
	T::const_iterator it(src.begin());
	T::const_iterator end(src.end());
	typename std::vector<const T::value_type *> tmp;
	tmp.reserve(src.size());
	for(const auto & x  : src) {
		tmp.push_back(&x);
	}

	auto sortFn = [](const T::value_type * a, const T::value_type * b) {
		return a->first < b->first;
	};
	std::sort(tmp.begin(), tmp.end(), sortFn);
	std::unordered_map<uint32_t, uint32_t> remap;
	remap.reserve(tmp.size());
	for(uint32_t i = 0; i < tmp.size(); ++i) {
			remap[tmp[i]->second] = i;
		}
	return remap;
}

void KeyValueObjectStore::serialize(const Item & item, UByteArrayAdapter & dest) {
	uint32_t minKey = std::numeric_limits<uint32_t>::max();
	uint32_t minValue = std::numeric_limits<uint32_t>::max();
	uint32_t maxKey = std::numeric_limits<uint32_t>::min();
	uint32_t maxValue = std::numeric_limits<uint32_t>::min();
	for(const KeyValue & kv : item) {
			minKey = std::min(kv.key, minKey);
			minValue = std::min(kv.value, minValue);
			maxKey = std::max(kv.key, maxKey);
			maxValue = std::max(kv.value, maxValue);
		}

	if(item.size() > (std::numeric_limits<uint32_t>::max() >> 10)) {
			throw sserialize::Exception("Out of boundsin KeyValueObjectStore::serialize(item)");
	}

	uint32_t keyBits = sserialize::CompactUintArray::minStorageBits((maxKey - minKey) + 1);
	uint32_t valueBits = sserialize::CompactUintArray::minStorageBits((maxValue - minValue) + 1);
	
	uint32_t countBPKV = item.size() << 10;
	countBPKV |= (keyBits-1) << 5;
	countBPKV |= (valueBits-1);

	dest.putVlPackedUint32(countBPKV);
	
	UByteArrayAdapter d(dest);
	d.shrinkToPutPtr();
	
	uint32_t storageNeed = sserialize::MultiVarBitArray::minStorageBytes(keyBits+valueBits, item.size());
	UByteArrayAdapter mvbData = UByteArrayAdapter::createCache(0);
	std::vector<uint8_t> bitConfig;
	bitConfig.push_back(keyBits);
	bitConfig.push_back(valueBits);
	MultiVarBitArrayCreator mvbCreator(bitConfig, mvbData);
	mvbCreator.reserve(item.size());
	for(uint32_t i = 0; i < item.size(); ++i) {
		mvbCreator.set(i, 0, item[i].key);
		mvbCreator.set(i, 1, item[i].value);
	}
	mvbData = mvbCreator.flush();
	dest.put(mvbData);
}

void KeyValueObjectStore::serialize(UByteArrayAdapter & dest) {
	std::unordered_map<uint32_t, uint32_t> keyRemap = createRemap(m_keyStringTable);
	std::unordered_map<uint32_t, uint32_t> valueRemap = createRemap(m_valueStringTable);

	Static::DequeCreator<UByteArrayAdapter> creator(dest);
	auto remapFunc = [&keyRemap, &valueRemap](const KeyValue & kv) {
		return KeyValue(keyRemap.at(kv.key), valueRemap.at(kv.value));
	};

	for(const Item & item : m_items) {
		creator.beginRawPut();
		serialize(sserialize::transform<Item>(item.begin(), item.end(), remapFunc), creator.rawPut());
		creator.endRawPut();
	}
	creator.flush();
}

}//end namespace
