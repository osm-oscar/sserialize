#include <sserialize/containers/KeyValueObjectStore.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Deque.h>
#include <algorithm>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <sserialize/utility/ProgressInfo.h>

namespace sserialize {

KeyValueObjectStore::KeyValueObjectStore() {}
KeyValueObjectStore::~KeyValueObjectStore() {}

void KeyValueObjectStore::clear() {
	m_keyStringTable = KeyStringTable();
	m_valueStringTable = ValueStringTable();
	m_items = ItemDataContainer();
}

uint32_t KeyValueObjectStore::keyId(const std::string & str) {
	return m_keyStringTable.insert(str);
}

uint32_t KeyValueObjectStore::valueId(const std::string & str) {
	return m_valueStringTable.insert(str);
}

void KeyValueObjectStore::push_back(const std::vector< std::pair<std::string, std::string> > & extItem) {
	m_items.push_back(ItemData());
	ItemData & item = m_items.back();
	item.reserve(extItem.size());
	for(const std::pair<std::string, std::string> & kv : extItem) {
		item.push_back(KeyValue(keyId(kv.first), valueId(kv.second)));
	}
}

template<typename T>
std::unordered_map<uint32_t, uint32_t> createRemap(const T & src) {
	typename T::const_iterator it(src.begin());
	typename T::const_iterator end(src.end());
	typedef typename T::value_type ValueType;
	typedef const ValueType * ValueTypePtr;
	std::vector<ValueTypePtr> tmp;
	tmp.reserve(src.size());
	for(const auto & x  : src) {
		tmp.push_back(&x);
	}

	auto sortFn = [](ValueTypePtr a, ValueTypePtr b) {
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

void KeyValueObjectStore::serialize(const sserialize::KeyValueObjectStore::ItemData & item, sserialize::UByteArrayAdapter & dest) const {
	if(item.size() > (std::numeric_limits<uint32_t>::max() >> 10)) {
		throw sserialize::CreationException("Out of bounds in KeyValueObjectStore::serialize(item)");
	}
	
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

	uint32_t keyBits = sserialize::CompactUintArray::minStorageBits((maxKey - minKey) + 1);
	uint32_t valueBits = sserialize::CompactUintArray::minStorageBits((maxValue - minValue) + 1);
	
	uint32_t countBPKV = item.size() << 10;
	countBPKV |= (keyBits-1) << 5;
	countBPKV |= (valueBits-1);

	dest.putVlPackedUint32(countBPKV);
	dest.putVlPackedUint32(minKey);
	dest.putVlPackedUint32(minValue);
	
	CompactUintArray carr(UByteArrayAdapter::createCache(0, false), keyBits+valueBits);
	carr.reserve(item.size());
	for(uint32_t i = 0; i < item.size(); ++i) {
		uint64_t pv = ( static_cast<uint64_t>(item.at(i).key-minKey) << valueBits) | (item.at(i).value-minValue);
		carr.set64(i, pv);
	}
	dest.put(carr.data());
}

void KeyValueObjectStore::sort() {
	std::unordered_map<uint32_t, uint32_t> keyRemap, valueRemap;
	m_keyStringTable.sort(keyRemap);
	m_valueStringTable.sort(valueRemap);

	auto remapFunc = [&keyRemap, &valueRemap](const KeyValue & kv) {
		return KeyValue(keyRemap.at(kv.key), valueRemap.at(kv.value));
	};

	for(ItemData & item : m_items) {
		std::transform(item.begin(), item.end(), item.begin(), remapFunc);
		std::sort(item.begin(), item.end());
	}
}

UByteArrayAdapter::OffsetType KeyValueObjectStore::serialize(sserialize::UByteArrayAdapter & dest) const {
	UByteArrayAdapter::OffsetType dataBegin = dest.tellPutPtr();
	sserialize::TimeMeasurer tm;
	tm.begin();
	std::cout << "KeyValueObjectStore::serialize: Serializing string tables..." << std::flush;
	m_keyStringTable.serialize(dest);
	m_valueStringTable.serialize(dest);
	tm.end();
	std::cout << "took " << tm.elapsedSeconds() << " seconds" << std::endl;

	Static::DequeCreator<UByteArrayAdapter> creator(dest);
	
	sserialize::ProgressInfo pinfo;
	pinfo.begin(m_items.size(), "KeyValueObjectStore::serialize: items");
	for(uint32_t i = 0, s = m_items.size(); i < s; ++i) {
		const ItemData & item = m_items[i];
		creator.beginRawPut();
		serialize(item, creator.rawPut());
		creator.endRawPut();
		pinfo(i);
	}
	pinfo.end();
	creator.flush();
	return dest.tellPutPtr()-dataBegin;
}

std::pair<std::string, std::string> KeyValueObjectStore::keyValue(uint32_t keyId, uint32_t valueId) const {
	return std::pair<std::string, std::string>(m_keyStringTable.at(keyId), m_valueStringTable.at(valueId));
}

}//end namespace
