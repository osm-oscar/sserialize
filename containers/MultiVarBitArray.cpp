#include <sserialize/containers/MultiVarBitArray.h>
#include <numeric>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/utility/utilmath.h>

namespace sserialize {

MultiVarBitArrayPrivate::MultiVarBitArrayPrivate() : RefCountObject() {}

MultiVarBitArrayPrivate::MultiVarBitArrayPrivate(const std::vector< uint8_t >& bitConfig, const sserialize::UByteArrayAdapter& data) : RefCountObject(), m_data(data) {
	uint32_t sum = 0;
	m_bitSums.resize(bitConfig.size(), 0);
	for(size_t i = 0; i < bitConfig.size(); i++) {
		sum += bitConfig.at(i);
		m_bitSums[i] = sum;
	}
}

MultiVarBitArrayPrivate::MultiVarBitArrayPrivate(const UByteArrayAdapter & data) : RefCountObject(), m_size(data.getUint32(1)) {
	m_size = data.getUint32(1);
	uint16_t bitConfigCount = data.getUint8(5);
	CompactUintArray carr(data+MultiVarBitArray::HEADER_SIZE, 5);
	m_bitSums.reserve(bitConfigCount);
	m_bitSums.push_back(carr.at(0)+1);
	for(uint16_t i = 1; i < bitConfigCount; i++)
		m_bitSums.push_back(m_bitSums[i-1]+1+carr.at(i));
	
	UByteArrayAdapter::OffsetType dataOffset = MultiVarBitArray::HEADER_SIZE + CompactUintArray::minStorageBytes(5, bitConfigCount);
	m_data = data+dataOffset;
}

MultiVarBitArrayPrivate::~MultiVarBitArrayPrivate() {}

uint32_t MultiVarBitArrayPrivate::size() const {
	return m_size;
}

UByteArrayAdapter::OffsetType MultiVarBitArrayPrivate::getSizeInBytes() const {
	return MultiVarBitArray::HEADER_SIZE +
			CompactUintArray::minStorageBytes(5, bitConfigCount()) +
			MultiVarBitArray::minStorageBytes(bitsPerEntry(), size());
}

uint16_t MultiVarBitArrayPrivate::bitsPerEntry() const {
	if (m_bitSums.size())
		return m_bitSums.at(m_bitSums.size()-1);
	return 0;
}

uint8_t MultiVarBitArrayPrivate::bitCount(uint32_t pos) const {
	if (pos == 0)
		return m_bitSums[0];
	return m_bitSums[pos] - m_bitSums[pos-1];
}

uint32_t MultiVarBitArrayPrivate::at(uint32_t pos, uint32_t subPos) const {
	if (pos >= m_size)
		return 0;

	uint32_t mask = createMask(bitCount(subPos));
	uint8_t bitCount = this->bitCount(subPos);

	uint64_t initTotalBitShift = ((uint64_t)m_bitSums.back())*pos + (subPos > 0 ? m_bitSums[subPos-1] : 0);
	uint64_t posStart = initTotalBitShift/8;
	uint8_t initShift = initTotalBitShift - posStart*8;

	uint32_t res = static_cast<uint32_t>(m_data.at(posStart)) >> initShift;

	uint8_t byteShift = 8 - initShift;
	posStart++;
	for(uint8_t bitsRead = 8 - initShift, i  = 0; bitsRead < bitCount; bitsRead+=8, i++) {
		uint32_t newByte = m_data.at(posStart+i);
		newByte = (newByte << (8*i+byteShift));
		res |= newByte;
	}
	return (res & mask);
	
}

uint32_t MultiVarBitArrayPrivate::set(uint32_t pos, uint32_t subPos, uint32_t value) {
	if (pos >= m_size)
		return ~value;

	uint8_t bitCount = this->bitCount(subPos);
	uint32_t mymask = createMask(bitCount);

	value = value & mymask;
	
	uint64_t initTotalBitShift = ((uint64_t)m_bitSums.back())*pos + (subPos > 0 ? m_bitSums[subPos-1] : 0);
	uint64_t startPos = initTotalBitShift/8;
	uint8_t initShift = initTotalBitShift - startPos*8;

	if (initShift+bitCount <= 8) { //everything is within the first
		uint8_t mask = createMask(initShift); //sets the lower bits
		mask |= (~ static_cast<uint8_t>(createMask(initShift+bitCount))); //sets the higher bits
		m_data[startPos] &= mask;
		m_data[startPos] |= (value << initShift);
	}
	else { //we have to set multiple bytes
		uint8_t bitsSet = 0;
		uint8_t mask = createMask(initShift); //sets the lower bits
		m_data[startPos] &= mask;
		m_data[startPos] |= ((value << initShift) & 0xFF);
		bitsSet += 8-initShift;
		uint8_t lastByteBitCount = (bitCount+initShift) % 8;

		//Now set all the midd bytes:
		uint8_t curShift = bitsSet;
		uint8_t count = 1;
		for(; curShift+lastByteBitCount < bitCount; curShift+=8, count++) {
			m_data[startPos+count] = (value >> curShift) & 0xFF;
		}

		//Now set the remaining bits
		if (lastByteBitCount > 0) {
			m_data[startPos+count] &= (~ static_cast<uint8_t>( createMask(lastByteBitCount) ));
			m_data[startPos+count] |= (value >> (bitCount-lastByteBitCount) );
		}

	}
	return value;
}

const UByteArrayAdapter& MultiVarBitArrayPrivate::data() const {
	return m_data;
}

UByteArrayAdapter & MultiVarBitArrayPrivate::data() {
	return m_data;
}

void MultiVarBitArrayPrivate::setSize(uint32_t newSize) {
	m_size = newSize;
}

uint32_t MultiVarBitArrayPrivate::bitConfigCount() const {
	return m_bitSums.size();
}


MultiVarBitArray::MultiVarBitArray() : MyParentClass(new MultiVarBitArrayPrivate()) {}
MultiVarBitArray::MultiVarBitArray(const UByteArrayAdapter & data) : MyParentClass(new MultiVarBitArrayPrivate(data)) {}

MultiVarBitArray::MultiVarBitArray(const MultiVarBitArray& other) : MyParentClass(other) {}

MultiVarBitArray::~MultiVarBitArray() {}

MultiVarBitArray& MultiVarBitArray::operator=(const MultiVarBitArray& other) {
	MyParentClass::operator=(other);
	return *this;
}

uint32_t MultiVarBitArray::size() const {
	return priv()->size();
}

UByteArrayAdapter::OffsetType MultiVarBitArray::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

uint32_t MultiVarBitArray::at(uint32_t pos, uint32_t subPos) const {
	return priv()->at(pos, subPos);
}

uint32_t MultiVarBitArray::set(uint32_t pos, uint32_t subPos, uint32_t value) {
	return priv()->set(pos, subPos, value);
}


void MultiVarBitArray::setSize(uint32_t newSize) {
	priv()->setSize(newSize);
}

uint16_t MultiVarBitArray::totalBitSum() const {
	return priv()->bitsPerEntry();
}

uint32_t MultiVarBitArray::bitConfigCount() const {
	return priv()->bitConfigCount();
}

uint8_t MultiVarBitArray::bitCount(uint32_t pos) const {
	return priv()->bitCount(pos);
}


std::ostream & MultiVarBitArray::printStats(std::ostream & out) const {
	out << "sserialize::MultiVarBitArray::stats--BEGIN" << std::endl;
	out << "size=" << size() << std::endl;
	out << "bits per entry=" << totalBitSum() << std::endl;
	out << "fields per entry=" << bitConfigCount() << std::endl;
	out << "sserialize::MultiVarBitArray::stats--END" << std::endl;
	return out;
}

const UByteArrayAdapter& MultiVarBitArray::data() const {
	return priv()->data();
}

UByteArrayAdapter& MultiVarBitArray::data() {
	return priv()->data();
}

UByteArrayAdapter::OffsetType MultiVarBitArray::minStorageBytes(const std::vector< uint8_t >& bitConfig, const uint32_t count) {
	uint32_t sum = std::accumulate(bitConfig.begin(), bitConfig.end(), static_cast<uint32_t>(0));
	return minStorageBytes(sum, count);
}

UByteArrayAdapter::OffsetType MultiVarBitArray::minStorageBytes(const uint32_t bitSum, const UByteArrayAdapter::OffsetType count) {
	return (bitSum/8)*count + ( (bitSum%8)*count )/8 + + ( ( (bitSum%8)*count )%8 > 0 ? 1 : 0);
}

MultiVarBitArrayCreator::MultiVarBitArrayCreator(const std::vector<uint8_t> & bitConfig, UByteArrayAdapter& data) : m_data(data), m_header(m_data) {
	m_header.shrinkToPutPtr();
	m_headerSize = MultiVarBitArray::HEADER_SIZE + CompactUintArray::minStorageBytes(5, bitConfig.size());
	if (m_header.size() < m_headerSize)
		m_header.growStorage( m_headerSize - m_header.size() );
	m_header.putUint8(0, 0);
	m_header.putUint32(1, 0);
	m_header.putUint8(5, bitConfig.size());
	CompactUintArray carr(m_header+MultiVarBitArray::HEADER_SIZE, 5);
	for(size_t i = 0; i < bitConfig.size(); i++) {
		if (bitConfig[i]  == 0 || bitConfig[i] > 32)
			carr.set(i, 31);
		else
			carr.set(i, bitConfig[i]-1);
	}
	m_arr = MultiVarBitArray(m_header);
}

MultiVarBitArrayCreator::~MultiVarBitArrayCreator() {}

bool MultiVarBitArrayCreator::reserve(uint32_t count) {
	if (m_arr.size() >= count)
		return true;
	UByteArrayAdapter::OffsetType storageNeed = MultiVarBitArray::minStorageBytes(m_arr.totalBitSum(), count);
	if (m_arr.data().size() < storageNeed)
		if (!m_arr.data().growStorage( storageNeed - m_arr.data().size() ))
			return false;
	m_arr.setSize(count);
	return true;
}

bool MultiVarBitArrayCreator::set(uint32_t pos, uint32_t subPos, uint32_t value) {
	if (m_arr.size() <= pos)
		if( !reserve(pos+1) )
			return false;
	return value == m_arr.set(pos, subPos, value);
}

uint32_t MultiVarBitArrayCreator::at(uint32_t pos, uint32_t subPos) const {
	return m_arr.at(pos, subPos);
}

UByteArrayAdapter MultiVarBitArrayCreator::flush() {
	m_header.putUint32(1, m_arr.size());

	UByteArrayAdapter::OffsetType storageNeed = MultiVarBitArray::minStorageBytes(m_arr.totalBitSum(), m_arr.size());
	if (m_header.size() < m_headerSize+storageNeed) {//no flush has happened yet
		m_header.growStorage(storageNeed+m_headerSize-m_header.size());
	}
	m_data.reserveFromPutPtr(m_headerSize+storageNeed);
	m_data.incPutPtr(m_headerSize+storageNeed);
	return m_header;
}


}//end namespace