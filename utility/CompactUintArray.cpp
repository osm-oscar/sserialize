#include <sserialize/utility/CompactUintArray.h>
#include <stdint.h>
#include <iostream>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/SerializationInfo.h>

namespace sserialize {

void CompactUintArrayPrivate::calcBegin(const uint32_t pos, UByteArrayAdapter::OffsetType & posStart, uint8_t & initShift, uint8_t bpn) const {
	posStart = sserialize::multiplyDiv64(pos, bpn, 8);
	initShift = (pos == 0 ? 0 : sserialize::multiplyMod64(pos, bpn, 8));
}

CompactUintArrayPrivate::CompactUintArrayPrivate() : RefCountObject() {}

CompactUintArrayPrivate::CompactUintArrayPrivate(const UByteArrayAdapter& adap) :
RefCountObject(),
m_data(adap)
{}

CompactUintArrayPrivate::~CompactUintArrayPrivate() {}

uint8_t CompactUintArrayPrivate::bpn() const {
	return 0;
}


uint64_t CompactUintArrayPrivate::at64(uint32_t pos) const {
	return at(pos);
}


uint64_t CompactUintArrayPrivate::set64(const uint32_t pos, uint64_t value) {
	return set(pos, value);
}

CompactUintArrayPrivateEmpty::CompactUintArrayPrivateEmpty(): CompactUintArrayPrivate() {}

uint8_t CompactUintArrayPrivateEmpty::bpn() const {
	return 0;
}

uint32_t CompactUintArrayPrivateEmpty::at(uint32_t /*pos*/) const {
	return 0;
}


uint32_t CompactUintArrayPrivateEmpty::set(const uint32_t /*pos*/, uint32_t value) {
	return ~value;
}


CompactUintArrayPrivateVarBits::CompactUintArrayPrivateVarBits(const UByteArrayAdapter & array, uint8_t bitsPerNumber) :
CompactUintArrayPrivate(array),
m_bpn(bitsPerNumber)
{
	if (m_bpn == 0)
		m_bpn = 1;
	else if (m_bpn > 32)
		m_bpn = 32;
	if (m_bpn == 32)
		m_mask = 0xFFFFFFFF;
	else m_mask = (static_cast<uint32_t>(1) << m_bpn) - 1;
}

uint8_t CompactUintArrayPrivateVarBits::bpn() const {
	return m_bpn;
}

uint32_t CompactUintArrayPrivateVarBits::at(const uint32_t pos) const {
	UByteArrayAdapter::OffsetType posStart;
	uint8_t initShift;
	calcBegin(pos, posStart, initShift, m_bpn);
	uint32_t res = static_cast<uint32_t>(m_data.at(posStart)) >> initShift;

	uint8_t byteShift = 8 - initShift;
	posStart++;
	for(uint8_t bitsRead = 8 - initShift, i  = 0; bitsRead < m_bpn; bitsRead+=8, i++) {
		uint32_t newByte = m_data.at(posStart+i);
		newByte = (newByte << (8*i+byteShift));
		res |= newByte;
	}
	return (res & m_mask);
}


//Idee: zunächst müssen alle betroffenen bits genullt werden.
//anschließend den neuen wert mit einem ODER setzen
uint32_t CompactUintArrayPrivateVarBits::set(const uint32_t pos, uint32_t value) {
	value = value & m_mask;
	
	UByteArrayAdapter::OffsetType startPos;
	uint8_t initShift;
	calcBegin(pos, startPos, initShift, m_bpn);

	if (initShift+m_bpn <= 8) { //everything is within the first
		uint8_t mask = createMask(initShift); //sets the lower bits
		mask |= (~ static_cast<uint8_t>(createMask(initShift+m_bpn))); //sets the higher bits
		m_data[startPos] &= mask;
		m_data[startPos] |= (value << initShift);
	}
	else { //we have to set multiple bytes
		uint8_t bitsSet = 0;
		uint8_t mask = createMask(initShift); //sets the lower bits
		m_data[startPos] &= mask;
		m_data[startPos] |= ((value << initShift) & 0xFF);
		bitsSet += 8-initShift;
		uint8_t lastByteBitCount = (m_bpn+initShift) % 8;

		//Now set all the midd bytes:
		uint8_t curShift = bitsSet;
		uint8_t count = 1;
		for(; curShift+lastByteBitCount < m_bpn; curShift+=8, count++) {
			m_data[startPos+count] = (value >> curShift) & 0xFF;
		}

		//Now set the remaining bits
		if (lastByteBitCount > 0) {
			m_data[startPos+count] &= (~ static_cast<uint8_t>( createMask(lastByteBitCount) ));
			m_data[startPos+count] |= (value >> (m_bpn-lastByteBitCount) );
		}

	}
	return value;
}

CompactUintArrayPrivateVarBits64::CompactUintArrayPrivateVarBits64(const UByteArrayAdapter & adap, uint8_t bpn) :
CompactUintArrayPrivate(adap),
m_bpn(bpn)
{
	if (m_bpn == 0)
		m_bpn = 1;
	else if (m_bpn > 64)
		m_bpn = 64;
	m_mask = createMask64(m_bpn);
}

uint8_t CompactUintArrayPrivateVarBits64::bpn() const {
	return m_bpn;
}

uint32_t CompactUintArrayPrivateVarBits64::at(const uint32_t pos) const {
	return CompactUintArrayPrivateVarBits64::at64(pos);
}

uint32_t CompactUintArrayPrivateVarBits64::set(const uint32_t pos, uint32_t value) {
	return CompactUintArrayPrivateVarBits64::set64(pos, value);
}

uint64_t CompactUintArrayPrivateVarBits64::at64(uint32_t pos) const {
	UByteArrayAdapter::OffsetType posStart;
	uint8_t initShift;
	calcBegin(pos, posStart, initShift, m_bpn);
	uint64_t res = static_cast<uint64_t>(m_data.at(posStart)) >> initShift;

	uint8_t byteShift = 8 - initShift;
	posStart++;
	for(uint8_t bitsRead = 8 - initShift, i  = 0; bitsRead < m_bpn; bitsRead+=8, i++) {
		uint64_t newByte = m_data.at(posStart+i);
		newByte = (newByte << (8*i+byteShift));
		res |= newByte;
	}
	return (res & m_mask);
}

uint64_t CompactUintArrayPrivateVarBits64::set64(const uint32_t pos, uint64_t value) {
	value = value & m_mask;
	
	UByteArrayAdapter::OffsetType startPos;
	uint8_t initShift;
	calcBegin(pos, startPos, initShift, m_bpn);

	if (initShift+m_bpn <= 8) { //everything is within the first
		uint8_t mask = createMask(initShift); //sets the lower bits
		mask |= (~ static_cast<uint8_t>(createMask(initShift+m_bpn))); //sets the higher bits
		m_data[startPos] &= mask;
		m_data[startPos] |= (value << initShift);
	}
	else { //we have to set multiple bytes
		uint8_t bitsSet = 0;
		uint8_t mask = createMask(initShift); //sets the lower bits
		m_data[startPos] &= mask;
		m_data[startPos] |= ((value << initShift) & 0xFF);
		bitsSet += 8-initShift;
		uint8_t lastByteBitCount = (m_bpn+initShift) % 8;

		//Now set all the midd bytes:
		uint8_t curShift = bitsSet;
		uint8_t count = 1;
		for(; curShift+lastByteBitCount < m_bpn; curShift+=8, count++) {
			m_data[startPos+count] = (value >> curShift) & 0xFF;
		}

		//Now set the remaining bits
		if (lastByteBitCount > 0) {
			m_data[startPos+count] &= (~ static_cast<uint8_t>( createMask(lastByteBitCount) ));
			m_data[startPos+count] |= (value >> (m_bpn-lastByteBitCount) );
		}

	}
	return value;
}


CompactUintArrayPrivateU8::CompactUintArrayPrivateU8(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

uint8_t CompactUintArrayPrivateU8::bpn() const{
	return 8;
}


uint32_t CompactUintArrayPrivateU8::at(uint32_t pos) const {
    return m_data.getUint8(pos);
}

uint32_t CompactUintArrayPrivateU8::set(const uint32_t pos, uint32_t value) {
	m_data.putUint8(pos, value);
	return value;
}

CompactUintArrayPrivateU16::CompactUintArrayPrivateU16(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

uint8_t CompactUintArrayPrivateU16::bpn() const {
	return 16;
}


uint32_t CompactUintArrayPrivateU16::at(uint32_t pos) const {
    return m_data.getUint16( SerializationInfo<uint16_t>::length*pos);
}

uint32_t CompactUintArrayPrivateU16::set(const uint32_t pos, uint32_t value) {
    m_data.putUint16(SerializationInfo<uint16_t>::length*pos, value);
	return value;
}

CompactUintArrayPrivateU24::CompactUintArrayPrivateU24(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

uint8_t CompactUintArrayPrivateU24::bpn() const {
	return 24;
}


uint32_t CompactUintArrayPrivateU24::at(uint32_t pos) const
{
    return m_data.getUint24(static_cast<UByteArrayAdapter::OffsetType>(3)*pos);
}

uint32_t CompactUintArrayPrivateU24::set(const uint32_t pos, uint32_t value) {
	m_data.putUint24(static_cast<UByteArrayAdapter::OffsetType>(3)*pos, value);
	return value;
}

CompactUintArrayPrivateU32::CompactUintArrayPrivateU32(const UByteArrayAdapter& adap): CompactUintArrayPrivate(adap)
{}

uint8_t CompactUintArrayPrivateU32::bpn() const {
	return 32;
}


uint32_t CompactUintArrayPrivateU32::at(uint32_t pos) const {
    return m_data.getUint32(SerializationInfo<uint32_t>::length*pos);
}

uint32_t CompactUintArrayPrivateU32::set(const uint32_t pos, uint32_t value) {
    m_data.putUint32(SerializationInfo<uint32_t>::length*pos, value);
	return value;
}

CompactUintArray::CompactUintArray(CompactUintArrayPrivate * priv) :
RCWrapper< sserialize::CompactUintArrayPrivate >(priv)
{}

void CompactUintArray::setPrivate(const UByteArrayAdapter & array, uint8_t bitsPerNumber) {
	if (bitsPerNumber == 0)
		bitsPerNumber = 1;
	else if (bitsPerNumber > 64)
		bitsPerNumber = 64;

	m_maxCount = (static_cast<uint64_t>(array.size())*8)/bitsPerNumber;


	switch(bitsPerNumber) {
	case (8):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU8(array));
		break;
	case (16):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU16(array));
		break;
	case (24):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU24(array));
		break;
	case (32):
		MyBaseClass::setPrivate(new CompactUintArrayPrivateU32(array));
		break;
	default:
		if (bitsPerNumber <= 32)
			MyBaseClass::setPrivate(new CompactUintArrayPrivateVarBits(array, bitsPerNumber));
		else
			MyBaseClass::setPrivate(new CompactUintArrayPrivateVarBits64(array, bitsPerNumber));
		break;
	}
}

CompactUintArray::CompactUintArray() :
RCWrapper< sserialize::CompactUintArrayPrivate > (new CompactUintArrayPrivateEmpty()),
m_maxCount(0)
{
}

CompactUintArray::CompactUintArray(const UByteArrayAdapter & array, uint8_t bitsPerNumber) :
RCWrapper< sserialize::CompactUintArrayPrivate >(0)
{
	setPrivate(array, bitsPerNumber);
}

CompactUintArray::CompactUintArray(const UByteArrayAdapter & array, uint8_t bitsPerNumber, uint32_t max_count) :
RCWrapper< sserialize::CompactUintArrayPrivate >(0)
{
	setPrivate(array, bitsPerNumber);
	m_maxCount = max_count;
}

CompactUintArray::CompactUintArray(const CompactUintArray & other) :
RCWrapper< sserialize::CompactUintArrayPrivate >(other),
m_maxCount(other.m_maxCount)
{}

CompactUintArray::~CompactUintArray() {}

UByteArrayAdapter::OffsetType CompactUintArray::getSizeInBytes() const {
	uint64_t tmp = (uint64_t)m_maxCount*bpn();
	return tmp/8 + (tmp%8 ? 1 : 0);
}

CompactUintArray& CompactUintArray::operator=(const CompactUintArray & other) {
	RCWrapper< sserialize::CompactUintArrayPrivate >::operator=(other);
	m_maxCount = other.m_maxCount;
	return *this;
}

uint8_t CompactUintArray::bpn() const{
	return priv()->bpn();
}

int32_t CompactUintArray::findSorted(uint32_t key, int32_t len) const {
	if (len == 0)
		return npos;
	int32_t left = 0;
	int32_t right = len-1;
	int32_t mid = (right-left)/2+left;
	uint32_t tk = priv()->at(mid);
	while( left < right ) {
		if (tk == key)
			return mid;
		if (tk < key) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
		tk = priv()->at(mid);
	}
	return (tk == key ? mid : npos);
}


uint32_t CompactUintArray::at(uint32_t pos) const {
	if (m_maxCount == 0)
		return 0;
	if (pos >= m_maxCount)
		pos = m_maxCount-1;
	return priv()->at(pos);
}

uint64_t CompactUintArray::at64(uint32_t pos) const {
	if (m_maxCount == 0)
		return 0;
	if (pos >= m_maxCount)
		pos = m_maxCount-1;
	return priv()->at64(pos);
}

uint32_t CompactUintArray::set(const uint32_t pos, const uint32_t value) {
	if (pos >= m_maxCount)
		return ~pos;
	return priv()->set(pos, value);
}

uint64_t CompactUintArray::set64(const uint32_t pos, const uint64_t value) {
	if (pos >= m_maxCount)
		return ~pos;
	return priv()->set64(pos, value);
}

bool CompactUintArray::reserve(uint32_t newMaxCount) {
	if (maxCount() >= newMaxCount)
		return true;

	uint32_t mybpn = bpn();
	UByteArrayAdapter::OffsetType neededByteCount = minStorageBytes(mybpn, newMaxCount);
	UByteArrayAdapter::OffsetType haveByteCount = priv()->data().size();
	if (priv()->data().growStorage(neededByteCount-haveByteCount)) {
		m_maxCount = newMaxCount;
		return true;
	}
	return false;
}


UByteArrayAdapter & CompactUintArray::data() {
	return priv()->data();
}

const UByteArrayAdapter & CompactUintArray::data() const {
	return priv()->data();
}

std::ostream & CompactUintArray::dump(std::ostream& out, uint32_t len) {
	if (maxCount() < len)
		len = maxCount();

	out << "(";
	for(size_t i = 0; i < len; i++) {
		out << at(i) << ", ";
	}
	out << ")" << std::endl;
	return out;
}

void CompactUintArray::dump() {
	dump(std::cout, maxCount());
}

uint8_t CompactUintArray::minStorageBits(const uint32_t number) {
	return std::max<uint8_t>(1, msb(number)+1);
}

uint8_t CompactUintArray::minStorageBits64(const uint64_t number) {
	return std::max<uint8_t>(1, msb(number)+1);
}


uint8_t CompactUintArray::minStorageBitsFullBytes(const uint32_t number) {
	if (number <= 0xFF)
		return 8;
	else if (number <= 0xFFFF)
		return 16;
	else if (number <= 0xFFFFFF)
		return 24;
	else return 32;
}

uint8_t CompactUintArray::minStorageBitsFullBytes64(uint64_t number) {
	uint8_t res = 0; 
	do {
		number >>= 8;
		res += 8;
	} while (number);
	return res;
}

UByteArrayAdapter::OffsetType CompactUintArray::minStorageBytes(uint8_t bpn, uint32_t count) {
	uint64_t bits = static_cast<uint64_t>(count)*bpn;
	return bits/8 + (bits % 8 ? 1 : 0);
}

BoundedCompactUintArray::BoundedCompactUintArray(const sserialize::UByteArrayAdapter & d) :
CompactUintArray(0)
{
	int len;
	m_size = d.getVlPackedUint64(0, &len);
	uint8_t bits = (m_size & 0x3F) +1;
	m_size >>= 6;
	UByteArrayAdapter::OffsetType dSize = minStorageBytes(bits, m_size);
	if (m_size) {
		CompactUintArray::setPrivate(UByteArrayAdapter(d, len, dSize), bits);
	}
	else {
		CompactUintArray::MyBaseClass::setPrivate(new CompactUintArrayPrivateEmpty());
	}
	
}

BoundedCompactUintArray::BoundedCompactUintArray(const BoundedCompactUintArray & other) :
CompactUintArray(other),
m_size(other.m_size)
{}

BoundedCompactUintArray::~BoundedCompactUintArray() {}

BoundedCompactUintArray & BoundedCompactUintArray::operator=(const BoundedCompactUintArray & other) {
	CompactUintArray::operator=(other);
	m_size = other.m_size;
	return *this;
}

UByteArrayAdapter::OffsetType  BoundedCompactUintArray::getSizeInBytes() const {
	uint8_t bits = bpn();
	UByteArrayAdapter::OffsetType sb = (m_size << 6) | (bits-1);
	return psize_vu64(sb) + minStorageBytes(bits, m_size);
}

}//end namespace