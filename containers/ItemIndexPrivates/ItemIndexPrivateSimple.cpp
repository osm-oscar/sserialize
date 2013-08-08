#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

ItemIndexPrivateSimpleCreator::ItemIndexPrivateSimpleCreator(uint32_t lowestId, uint32_t highestId, uint32_t reserveCount, UByteArrayAdapter & destination) :
m_destination(destination), m_beginning(m_destination.tellPutPtr()), m_lowestId(lowestId), m_pos(0) {
	UByteArrayAdapter data(m_destination + m_destination.tellPutPtr());
	uint8_t bpn = CompactUintArray::minStorageBitsFullBytes(highestId-lowestId+1);
	uint32_t storageSize = CompactUintArray::minStorageBytes(bpn, reserveCount) + ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE;
	uint32_t haveStorage = data.size();
	if (storageSize > haveStorage)
		data.growStorage(storageSize - haveStorage);
	m_carr = CompactUintArray(data+ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE, bpn);
}

void ItemIndexPrivateSimpleCreator::push_back(uint32_t id) {
	m_carr.reserve(m_pos+1);
	m_carr.set(m_pos, id-m_lowestId);
	++m_pos;
}

/** This needs to be called before you can user the data */
void ItemIndexPrivateSimpleCreator::flush() {
	UByteArrayAdapter data(m_destination, m_destination.tellPutPtr(), ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE);
	if (data.size() < ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE)
		data.growStorage(ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE-data.size());
	ItemIndexPrivateSimple::addHeader(m_pos, m_carr.bpn()/8, m_lowestId, data);
	//adjust putptr
	uint32_t neededData = ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE + CompactUintArray::minStorageBytes(m_carr.bpn(), m_pos);
	if (m_destination.size()-m_destination.tellPutPtr() < neededData)
		m_destination.growStorage(m_destination.tellPutPtr() + neededData - m_destination.size()); //this should not inc the underlying storage
	m_destination.incPutPtr(neededData);
}

ItemIndexPrivate * ItemIndexPrivateSimpleCreator::getPrivateIndex() {
	return privateIndex();
}

ItemIndex ItemIndexPrivateSimpleCreator::getIndex() const {
	return ItemIndex(m_destination+m_beginning, ItemIndex::T_SIMPLE);
}

ItemIndexPrivateSimple * ItemIndexPrivateSimpleCreator::privateIndex() {
	return new ItemIndexPrivateSimple(m_destination+m_beginning);
}

UByteArrayAdapter ItemIndexPrivateSimpleCreator::createCache(uint32_t lowestId, uint32_t highestId, uint32_t maxCount, bool forceFileBase) {
	uint8_t bpn = CompactUintArray::minStorageBitsFullBytes(highestId-lowestId+1);
	uint32_t storageSize = CompactUintArray::minStorageBytes(bpn, maxCount);
	return UByteArrayAdapter::createCache(ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE+storageSize, forceFileBase);
}

ItemIndexPrivateSimple::ItemIndexPrivateSimple() : ItemIndexPrivate(), m_size(0), m_bpn(0), m_yintercept(0) {}

ItemIndexPrivateSimple::ItemIndexPrivateSimple(const UByteArrayAdapter & index) :
ItemIndexPrivate(),
m_bpn(0),
m_yintercept(0)
{
	m_size = index.getUint32(0);
	m_bpn = ((m_size & 0x3)+1)*8;
	m_size = m_size >> 2;
	if (m_size > 0) {
		m_yintercept = index.getUint32(4);
		m_idStore = CompactUintArray(index+8, m_bpn);
	}
}

ItemIndexPrivateSimple::~ItemIndexPrivateSimple() {}

ItemIndex::Types ItemIndexPrivateSimple::type() const {
	return ItemIndex::T_SIMPLE;
}

uint32_t ItemIndexPrivateSimple::at(uint32_t pos) const {
	return m_idStore.at(pos) + m_yintercept;
}

uint32_t ItemIndexPrivateSimple::first() const {
	if (m_size)
		return m_idStore.at(0) + m_yintercept;
	else
		return 0;
}

uint32_t ItemIndexPrivateSimple::last() const {
	if (m_size)
		return m_idStore.at(m_size-1) + m_yintercept;
	else
		return 0;
}

uint32_t ItemIndexPrivateSimple::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateSimple::bpn() const {
	return m_bpn;
}

uint32_t ItemIndexPrivateSimple::slopenom() const {
	return 0;
}


int32_t ItemIndexPrivateSimple::yintercept() const {
	return m_yintercept;
}

uint32_t ItemIndexPrivateSimple::idOffSet() const {
	return 0;
}


uint32_t ItemIndexPrivateSimple::rawIdAt(const uint32_t pos) const {
	if (pos >= m_size)
		return 0;
	return m_idStore.at(pos);
}

uint32_t ItemIndexPrivateSimple::getSizeInBytes() const {
	return getHeaderbytes() + getRegressionLineBytes() + getIdBytes();
}

uint32_t ItemIndexPrivateSimple::getHeaderbytes() const {
	return 4;
}

uint32_t ItemIndexPrivateSimple::getRegressionLineBytes() const {
	return 4;
}

uint32_t ItemIndexPrivateSimple::getIdBytes() const {
	return m_bpn/8*m_size;
}


bool ItemIndexPrivateSimple::addItemIndexFromIds(const std::set<uint32_t> & ids, UByteArrayAdapter & adap) {
	uint32_t lowestId = *(ids.begin());
	uint32_t highestId = *(ids.rbegin());
	uint8_t neededBits = CompactUintArray::minStorageBitsFullBytes(highestId-lowestId);
	uint32_t neededBytes = 8 + CompactUintArray::minStorageBytes(neededBits, ids.size());
	if (adap.size() < neededBytes && !adap.growStorage(neededBytes))
		return false;

	addHeader(ids.size(), neededBits/8, lowestId, adap);
	CompactUintArray store(adap+8, neededBits);
	uint32_t count = 0;
	for(std::set<uint32_t>::const_iterator it = ids.begin(); it != ids.end(); it++) {
		store.set(count, *it-lowestId);
		count++;
	}
	return true;
}

void ItemIndexPrivateSimple::addHeader(uint32_t count, uint8_t bytesPerId, uint32_t lowestId, UByteArrayAdapter& adap) {
	if (count > 0x3FFFFFFF) {
		throw sserialize::OutOfBoundsException("sserialize::ItemIndexPrivateSimple::creation");
	}
	uint32_t countBits = (count << 2);
	countBits |= (bytesPerId-1);
	adap.putUint32(0, countBits);
	adap.putUint32(4, lowestId);
}

uint32_t ItemIndexPrivateSimple::storageSize(uint32_t count, uint8_t bytesPerId) {
	return headerSize()+count*bytesPerId;
}

uint32_t ItemIndexPrivateSimple::headerSize() {
	return 8;
}

CompactUintArray ItemIndexPrivateSimple::initStorage(const UByteArrayAdapter& destination, uint8_t bytesPerId) {
	return CompactUintArray(destination+headerSize(), bytesPerId*8);
}

ItemIndexPrivate * ItemIndexPrivateSimple::fromBitSet(const DynamicBitSet & bitSet) {
	if (!bitSet.data().size())
		return new ItemIndexPrivateEmpty();

	const UByteArrayAdapter & bitSetData(bitSet.data());
	uint32_t dataOffset = 0;
	uint32_t myDataSize = bitSetData.size()-1;
	
	//first find the first an last occurence of a bit
	while (! bitSetData.at(dataOffset) && dataOffset <= myDataSize)
		++dataOffset;
	if (dataOffset > myDataSize)
		return new ItemIndexPrivateEmpty();

	while (! bitSetData.at(myDataSize)) //this will at least stop at dataOffset
		--myDataSize;
	++myDataSize; //readjust to set end one behind the last
	
	uint32_t minId = dataOffset*8;
	uint32_t maxId = myDataSize*8;
	
	UByteArrayAdapter cacheData( UByteArrayAdapter::createCache(bitSetData.size(), false));
	ItemIndexPrivateSimpleCreator creator(minId, maxId, 2, cacheData);
	uint32_t curId = minId;
	for(; dataOffset < myDataSize; ++dataOffset, curId += 8) {
		uint8_t data = bitSetData.at(dataOffset);
		for(uint32_t i = 0; i < 8 && data; ++i) {
			if (data & 0x1)
				creator.push_back(curId+i);
			data >>= 1;
		}
	}
	creator.flush();
	return creator.getPrivateIndex();
}


}//end namespace