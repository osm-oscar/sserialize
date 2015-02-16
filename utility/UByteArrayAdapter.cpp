#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/log.h>
#include "utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivates.h"
#include <iostream>
#include <sserialize/utility/types.h>


namespace sserialize {

std::string UByteArrayAdapter::m_tempFilePrefix = TEMP_FILE_PREFIX;
std::string UByteArrayAdapter::m_fastTempFilePrefix = TEMP_FILE_PREFIX;
std::string UByteArrayAdapter::m_logFilePrefix = TEMP_FILE_PREFIX;


UByteArrayAdapter::MemoryView::MemoryViewImp::MemoryViewImp(uint8_t * ptr, OffsetType off, OffsetType size, bool isCopy, UByteArrayAdapterPrivate * base) :
m_dataBase(base),
m_d(ptr),
m_off(off),
m_size(size),
m_copy(isCopy)
{}

UByteArrayAdapter::MemoryView::MemoryViewImp::~MemoryViewImp() {
	if (isCopy()) {
		delete[] m_d;
	}
}

bool UByteArrayAdapter::MemoryView::MemoryViewImp::flush(OffsetType len, OffsetType off) {
	if (isCopy()) {
		if (off > m_size) {
			return false;
		}
		len = std::min<OffsetType>(len, m_size-off);
		m_dataBase->put(m_off, m_d+off, len);
	}
	return true;
}


UByteArrayAdapter::UByteArrayAdapter(const RCPtrWrapper<UByteArrayAdapterPrivate> & priv) :
m_priv(priv),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
}

UByteArrayAdapter::UByteArrayAdapter(const RCPtrWrapper<UByteArrayAdapterPrivate> & priv, OffsetType offSet, OffsetType len) :
m_priv(priv),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
	if (!priv.get()) {
		m_offSet = 0;
		m_len = 0;
	}
}

UByteArrayAdapter::UByteArrayAdapter() :
m_priv(new UByteArrayAdapterPrivateEmpty()),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::UByteArrayAdapter(uint8_t * data, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateArray(data)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, bool deleteOnClose) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	m_priv->setDeleteOnClose(deleteOnClose);
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, bool deleteOnClose) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	m_priv->setDeleteOnClose(deleteOnClose);
}


UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter) :
m_priv(adapter.m_priv),
m_offSet(adapter.m_offSet),
m_len(adapter.m_len),
m_getPtr(adapter.m_getPtr),
m_putPtr(adapter.m_putPtr)
{
}

UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter, OffsetType addOffset) :
m_priv(adapter.m_priv),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
	m_offSet = adapter.m_offSet;
	m_len = adapter.m_len;

	if (addOffset > m_len) {
		addOffset = m_len;
	}
	m_offSet += addOffset;
	m_len -= addOffset;

	if (addOffset > m_getPtr)
		m_getPtr = 0;
	else {
		m_getPtr = adapter.m_getPtr - addOffset;
		if (m_getPtr >= m_len) {
			m_getPtr = m_len;
		}
	}

	if (addOffset > m_putPtr)
		m_putPtr = 0;
	else {
		m_putPtr = adapter.m_putPtr - addOffset;
		if (m_putPtr >= m_len) {
			m_putPtr = m_len;
		}
	}

}

UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter, OffsetType addOffset, OffsetType smallerLen) :
m_priv(adapter.m_priv),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
	m_offSet = adapter.m_offSet;
	m_len = adapter.m_len;


	if (addOffset > m_len) {
		addOffset = m_len;
	}
	m_offSet += addOffset;
	m_len -= addOffset;

	if (m_len > smallerLen) {
		m_len = smallerLen;
	}

	if (addOffset > m_getPtr)
		m_getPtr = 0;
	else {
		m_getPtr = adapter.m_getPtr - addOffset;
		if (m_getPtr >= m_len) {
			m_getPtr = m_len;
		}
	}

	if (addOffset > m_putPtr)
		m_putPtr = 0;
	else {
		m_putPtr = adapter.m_putPtr - addOffset;
		if (m_putPtr >= m_len) {
			m_putPtr = m_len;
		}
	}
}

UByteArrayAdapter::UByteArrayAdapter(MmappedFile file, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateMmappedFile(file)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{}


UByteArrayAdapter::UByteArrayAdapter(MmappedFile file) :
m_priv(new UByteArrayAdapterPrivateMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::UByteArrayAdapter(const ChunkedMmappedFile & file) :
m_priv(new UByteArrayAdapterPrivateChunkedMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::UByteArrayAdapter(const CompressedMmappedFile & file) :
m_priv(new UByteArrayAdapterPrivateCompressedMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::UByteArrayAdapter(const MmappedMemory<uint8_t> & mem) :
m_priv(new UByteArrayAdapterPrivateMM(mem)),
m_offSet(0),
m_len(mem.size()),
m_getPtr(0),
m_putPtr(0)
{}

UByteArrayAdapter::~UByteArrayAdapter() {}

UByteArrayAdapter & UByteArrayAdapter::operator=(const UByteArrayAdapter & adapter) {
	m_priv = adapter.m_priv;
	m_offSet = adapter.m_offSet;
	m_len = adapter.m_len;
	m_getPtr = adapter.m_getPtr;
	m_putPtr = adapter.m_putPtr;
	return *this;
}

void UByteArrayAdapter::zero() {
	uint32_t bufLen = 1024*1024;
	uint8_t * zeros = new uint8_t[bufLen];
	memset(zeros, 0, bufLen);
	for(OffsetType i = 0; i < m_len; i += bufLen) {
		put(i, zeros, std::min<OffsetType>(m_len-i, bufLen));
	}
	delete[] zeros;
}

bool UByteArrayAdapter::equal(const UByteArrayAdapter& b) const {
	return (m_offSet == b.m_offSet && m_len == b.m_len && m_priv == b.m_priv && m_getPtr == b.m_getPtr && m_putPtr == b.m_putPtr);
}

bool UByteArrayAdapter::equalContent(const UByteArrayAdapter& b) const {
	if (size() != b.size())
		return false;
	for(uint32_t i = 0; i < size(); ++i) {
		if (at(i) != b.at(i))
			return false;
	}
	return true;
}

bool UByteArrayAdapter::equalContent(const std::deque< uint8_t >& b) const {
	if (size() != b.size())
		return false;
	for(uint32_t i = 0; i < size(); ++i) {
		if (at(i) != b.at(i))
			return false;
	}
	return true;
}


OffsetType UByteArrayAdapter::tellPutPtr() const {
	return m_putPtr;
}

void UByteArrayAdapter::incPutPtr(OffsetType num) {
	setPutPtr(m_putPtr+num);
}

void UByteArrayAdapter::decPutPtr(OffsetType num) {
	setPutPtr(m_putPtr-std::min<OffsetType>(num, m_putPtr));
}

void UByteArrayAdapter::setPutPtr(OffsetType pos) {
	if (pos >= m_len)
		pos = m_len;
	m_putPtr = pos;
}

void UByteArrayAdapter::resetPutPtr() {
	m_putPtr = 0;
}

UByteArrayAdapter& UByteArrayAdapter::shrinkToPutPtr() {
	if (m_putPtr > m_len) {
		m_putPtr = m_len;
	}
	m_offSet += m_putPtr;
	m_len -= m_putPtr;
	m_putPtr = 0;
	if (m_getPtr < m_putPtr)
		m_getPtr = 0;
	else
		m_getPtr -= m_putPtr;
	return *this;
}

OffsetType UByteArrayAdapter::tellGetPtr() const {
	return m_getPtr;
}

bool UByteArrayAdapter::getPtrHasNext() const {
	return m_getPtr < m_len;
}

void UByteArrayAdapter::incGetPtr(OffsetType num) {
	setGetPtr(m_getPtr+num);
}

void UByteArrayAdapter::decGetPtr(UByteArrayAdapter::OffsetType num) {
	setGetPtr(m_getPtr-std::min<OffsetType>(num, m_getPtr));
}

void UByteArrayAdapter::setGetPtr(OffsetType pos) {
	if (pos >= m_len)
		pos = m_len;
	m_getPtr = pos;
}

UByteArrayAdapter& UByteArrayAdapter::resetGetPtr() {
	m_getPtr = 0;
	return *this;
}

UByteArrayAdapter& UByteArrayAdapter::shrinkToGetPtr() {
	if (m_getPtr > m_len) {
		m_getPtr = m_len;
	}
	m_offSet += m_getPtr;
	m_len -= m_getPtr;
	m_getPtr = 0;
	if (m_putPtr < m_getPtr)
		m_putPtr = 0;
	else
		m_putPtr -= m_getPtr;
		
	return *this;
}

UByteArrayAdapter& UByteArrayAdapter::resetPtrs() {
	resetGetPtr();
	resetPutPtr();
	return *this;
}


bool UByteArrayAdapter::shrinkStorage(OffsetType byte) {
	if (byte > m_len)
		byte = m_len;
	bool ok = m_priv->shrinkStorage(m_offSet+m_len-byte);
	if (ok)
		m_len -= byte;
	return ok;
}

bool UByteArrayAdapter::growStorage(OffsetType byte) {
	bool ok;
	if (m_offSet+m_len+byte < m_offSet+m_len) //check for wrapp-around
		ok = false;
	else
		ok = m_priv->growStorage(m_offSet+m_len+byte);
	if (ok)
		m_len += byte;
	return ok;
}

bool UByteArrayAdapter::resize(OffsetType byte) {
	if (byte < m_len) {
		m_len = byte;
		if (m_getPtr < m_len) {
			m_getPtr = m_len;
		}
		if (m_putPtr < m_len) {
			m_putPtr = m_len;
		}
		return true;
	}
	else {
		return growStorage(byte-m_len);
	}
}

void UByteArrayAdapter::resetToStorage() {
	m_putPtr = 0;
	m_getPtr = 0;
	m_offSet = 0;
	m_len = m_priv->size();
}

bool UByteArrayAdapter::reserveFromPutPtr(OffsetType bytes) {
	if (m_putPtr < m_len && (m_len - m_putPtr) >= bytes) {
		return true;
	}
	OffsetType storageNeed = (m_putPtr+bytes) - m_len;
	return growStorage(storageNeed);
}

void UByteArrayAdapter::setDeleteOnClose(bool del) {
	m_priv->setDeleteOnClose(del);
}

uint8_t & UByteArrayAdapter::operator[](const OffsetType pos) {
	return (*m_priv)[m_offSet+pos];
}


const uint8_t & UByteArrayAdapter::operator[](const OffsetType pos) const {
	return (*m_priv)[m_offSet+pos];
}

uint8_t& UByteArrayAdapter::operator*() {
	return operator[](0);
}

const uint8_t& UByteArrayAdapter::operator*() const {
	return operator[](0);
}

uint8_t UByteArrayAdapter::at(OffsetType pos) const {
	if (pos < m_len) {
		return (*m_priv)[m_offSet+pos];
	}
	return 0;
}

UByteArrayAdapter UByteArrayAdapter::operator+(OffsetType offSet) const {
	return UByteArrayAdapter(*this, offSet);
}

UByteArrayAdapter UByteArrayAdapter::operator++(int) {
	UByteArrayAdapter cp(*this);
	operator++();
	return cp;
}

UByteArrayAdapter UByteArrayAdapter::operator--(int) {
	UByteArrayAdapter cp(*this);
	operator--();
	return cp;
}

UByteArrayAdapter& UByteArrayAdapter::operator++() {
	if (m_len > 0) {
		m_offSet++;
		m_len--;
		
		if (m_getPtr > 0)
			m_getPtr--;
			
		if (m_putPtr > 0)
			m_putPtr--;
	}
	return *this;
}

UByteArrayAdapter& UByteArrayAdapter::operator--() {
	if (m_offSet > 0) {
		m_len++;
		m_offSet--;
		m_getPtr++;
		m_putPtr++;
	}
	return *this;
}

UByteArrayAdapter& UByteArrayAdapter::operator+=(OffsetType offSet) {
	if (offSet > m_len) {
		offSet = m_len;
	}
	m_offSet += offSet;
	m_len -= offSet;
	
	if (offSet > m_getPtr)
		m_getPtr = 0;
	else
		m_getPtr -= offSet;

	if (offSet > m_putPtr)
		m_putPtr = 0;
	else
		m_putPtr -= offSet;

	return *this;
}

UByteArrayAdapter& UByteArrayAdapter::operator-=(OffsetType offSet) {
	if (offSet > m_offSet) {
		offSet = m_offSet;
	}
	m_offSet -= offSet;
	m_len += offSet;
	m_putPtr += offSet;
	m_getPtr += offSet;

	return *this;
}

UByteArrayAdapter UByteArrayAdapter::begin() const {
	return *this;
}

UByteArrayAdapter UByteArrayAdapter::end() const {
	return UByteArrayAdapter(*this, m_len);
}

UByteArrayAdapter::MemoryView UByteArrayAdapter::getMemView(const OffsetType pos, OffsetType size) {
	if (pos+size > m_len) {
		return MemoryView(0, 0, 0, false, 0);
	}
	if (m_priv->isContiguous()) {
		return MemoryView(&operator[](pos), pos, size, false, m_priv.get());
	}
	else {
		uint8_t * tmp = new uint8_t[size];
		get(pos, tmp, size);
		return MemoryView(tmp, pos, size,true, 0);
	}
}

const UByteArrayAdapter::MemoryView UByteArrayAdapter::getMemView(const OffsetType pos, OffsetType size) const {
	return const_cast<UByteArrayAdapter*>(this)->getMemView(pos, size);
}

int64_t UByteArrayAdapter::getInt64(const OffsetType pos) const {
	if (m_len < pos+8) return 0;
	return m_priv->getInt64(m_offSet+pos);
}

uint64_t UByteArrayAdapter::getUint64(const OffsetType pos) const {
	if (m_len < pos+8) return 0;
	return m_priv->getUint64(m_offSet+pos);
}

int32_t UByteArrayAdapter::getInt32(const OffsetType pos) const {
	if (m_len < pos+4) return 0;
	return m_priv->getInt32(m_offSet+pos);
}

uint32_t UByteArrayAdapter::getUint32(const OffsetType pos) const {
	if (m_len < pos+4) return 0;
	return m_priv->getUint32(m_offSet+pos);
}

uint32_t UByteArrayAdapter::getUint24(const OffsetType pos) const {
	if (m_len < pos+3) return 0;
	return m_priv->getUint24(m_offSet+pos);
}

uint16_t UByteArrayAdapter::getUint16(const OffsetType pos) const {
	if (m_len < pos+2) return 0;
	return m_priv->getUint16(m_offSet+pos);
}

uint8_t UByteArrayAdapter::getUint8(const OffsetType pos) const {
	if (m_len < pos+1) return 0;
	return m_priv->getUint8(m_offSet+pos);
}

//BUG:make this portable!
double UByteArrayAdapter::getDouble(const OffsetType pos) const {
	if (m_len < pos+8) return std::numeric_limits<double>::signaling_NaN();
	return unpack_double_from_uint64_t(m_priv->getUint64(m_offSet+pos));
}

float UByteArrayAdapter::getFloat(const OffsetType pos) const {
	if (m_len < pos+4) return std::numeric_limits<float>::signaling_NaN();
	return unpack_float_from_uint32_t(m_priv->getUint32(m_offSet+pos));
}

uint64_t UByteArrayAdapter::getVlPackedUint64(const OffsetType pos, int * length) const {
	if (m_len < pos+1)
		return 0; //we need to read at least one byte
	int len = std::min<OffsetType>(9, m_len - pos);
	uint64_t res = m_priv->getVlPackedUint64(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		return 0;
	return res;
}

int64_t UByteArrayAdapter::getVlPackedInt64(const OffsetType pos, int * length) const {
	if (m_len < pos+1)
		return 0; //we need to read at least one byte
	int len = std::min<OffsetType>(9, m_len - pos);
	int64_t res = m_priv->getVlPackedInt64(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		return 0;
	return res;
}

uint32_t UByteArrayAdapter::getVlPackedUint32(const OffsetType pos, int * length) const {
	if (m_len < pos+1)
		return 0; //we need to read at least one byte
	int len = std::min<OffsetType>(5, m_len - pos);
	uint32_t res = m_priv->getVlPackedUint32(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		return 0;
	return res;
}

int32_t UByteArrayAdapter::getVlPackedInt32(const OffsetType pos, int * length) const {
	if (m_len < pos+1)
		return 0; //we need to read at least one byte
	int len = std::min<OffsetType>(5, m_len - pos);
	uint32_t res = m_priv->getVlPackedInt32(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		return 0;
	return res;
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::getOffset(const OffsetType pos) const {
	if (m_len < pos+SSERIALIZED_OFFSET_BYTE_COUNT) return 0;
	return m_priv->getOffset(m_offSet+pos);
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset(const OffsetType pos) const {
	if (m_len < pos+SSERIALIZED_NEGATIVE_OFFSET_BYTE_COUNT) return 0;
	return m_priv->getNegativeOffset(m_offSet+pos);
}

std::string UByteArrayAdapter::getString(const OffsetType pos, int * length) const {
	int len = -1;
	uint32_t strLen = getVlPackedUint32(pos, &len);
	if (len < 0 || pos+len+strLen > m_len) {
		if (length)
			*length = -1;
		return std::string();
	}
	if (length)
		*length = strLen+len;
	return m_priv->getString(m_offSet+pos+len, strLen);
}

UByteArrayAdapter UByteArrayAdapter::getStringData(const OffsetType pos, int * length) const {
	int len = -1;
	uint32_t strLen = getVlPackedUint32(pos, &len);
	if (len < 0 || pos+len+strLen > m_len) {
		if (length)
			*length = -1;
		return UByteArrayAdapter();
	}
	if (length)
		*length = strLen+len;
	//no need to adjust get pointer here, as in case of streaming, the get pointer is BEFORE pos
	return UByteArrayAdapter(*this, pos+len, strLen);
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::get(const UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	if (pos > m_len)
		return 0;
	if (pos+len > m_len)
		len = m_len - pos;
	m_priv->get(m_offSet+pos, dest, len);
	return len;
}

/** If the supplied memory is not writable then you're on your own! **/

bool UByteArrayAdapter::putOffset(const OffsetType pos, const OffsetType value) {
	if (m_len < pos+SSERIALIZED_OFFSET_BYTE_COUNT)
		return false;
	m_priv->putOffset(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putNegativeOffset(const OffsetType pos, const NegativeOffsetType value) {
	if (m_len < pos+SSERIALIZED_NEGATIVE_OFFSET_BYTE_COUNT) return false;
	m_priv->putNegativeOffset(m_offSet+pos, value);
	return true;
}


bool UByteArrayAdapter::putUint64(const OffsetType pos, const uint64_t value) {
	if (m_len < pos+8) return false;
	m_priv->putUint64(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putInt64(const OffsetType pos, const int64_t value) {
	if (m_len < pos+8) return false;
	m_priv->putInt64(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putInt32(const OffsetType pos, const int32_t value) {
	if (m_len < pos+4) return false;
	m_priv->putInt32(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putUint32(const OffsetType pos, const uint32_t value) {
	if (m_len < pos+4)
		return false;
	m_priv->putUint32(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putUint24(const OffsetType pos, const uint32_t value) {
	if (m_len < pos+3) return false;
	m_priv->putUint24(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putUint16(const OffsetType pos, const uint16_t value) {
	if (m_len < pos+2) return false;
	m_priv->putUint16(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putUint8(const OffsetType pos, const uint8_t value) {
	if (m_len < pos+1) return false;
	m_priv->putUint8(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putDouble(const OffsetType pos, const double value) {
	if (m_len < pos+8) return false;
	m_priv->putUint64(m_offSet+pos, pack_double_to_uint64_t(value));
	return true;
}

bool UByteArrayAdapter::putFloat(const OffsetType pos, const float value) {
	if (m_len < pos+4) return false;
	m_priv->putUint32(m_offSet+pos, pack_float_to_uint32_t(value));
	return true;
}

/** @return: Length of the number, -1 on failure **/

int UByteArrayAdapter::putVlPackedUint64(const OffsetType pos, const uint64_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedUint64(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::putVlPackedInt64(const OffsetType pos, const int64_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedInt64(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::putVlPackedUint32(const OffsetType pos, const uint32_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedUint32(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::putVlPackedPad4Uint32(const OffsetType pos, const uint32_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedPad4Uint32(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::putVlPackedInt32(const OffsetType pos, const int32_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedInt32(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::putVlPackedPad4Int32(const OffsetType pos, const int32_t value) {
	if (m_len < pos+1) return -1; //we need to write at least one byte
	return m_priv->putVlPackedPad4Int32(m_offSet+pos, value, m_len-pos);
}

int UByteArrayAdapter::put(const OffsetType pos, const std::string & str) {
	UByteArrayAdapter::OffsetType needSize = psize_vu32(str.size()) + str.size();
	if (m_len < pos+needSize)
		return -1;
	int len = putVlPackedUint32(pos, str.size());
	for(size_t i = 0; i < str.size(); i++) {
		putUint8(pos+len+i, str[i]);
	}
	return needSize;
}

bool UByteArrayAdapter::put(OffsetType pos, const uint8_t * data, OffsetType len) {
	if (m_len < pos+len)
		return false;
	m_priv->put(m_offSet+pos, data, len);
	return true;
}

bool UByteArrayAdapter::put(const OffsetType pos, const std::deque< uint8_t >& data) {
	if (m_len < pos+data.size())
		return false;
	for(size_t i = 0; i < data.size(); i++) {
		putUint8(pos+i, data[i]);
	}
	return true;
}

bool UByteArrayAdapter::put(const OffsetType pos, const std::vector< uint8_t >& data) {
	if (data.size())
		return put(pos, &data[0], data.size());
	return true;
}

bool UByteArrayAdapter::put(const OffsetType pos, const UByteArrayAdapter & data) {
	if (m_len < pos+data.size())
		return false;

	if (m_priv->isContiguous()) {
		return put(pos, &data[0], data.size());
	}

	uint32_t bufLen = std::min<OffsetType>(data.size(), 1024*1024);
	uint8_t * buf = new uint8_t[bufLen];
	for(OffsetType i = 0, s = data.size(); i < s;) {
		OffsetType len = std::min<OffsetType>(s-i, bufLen);
		data.get(i, buf, len);
		put(pos+i, buf, len);
		i += len;
	}
	delete[] buf;
	return true;
}

bool UByteArrayAdapter::put(const OffsetType pos, const MemoryView & data) {
	return put(pos, data.get(), data.size());
}

UByteArrayAdapter UByteArrayAdapter::writeToDisk(std::string fileName, bool deleteOnClose) {
	if (fileName.empty()) {
		fileName = MmappedFile::findLockFilePath(PERSISTENT_CACHE_PATH, 2048);
	}
	if (fileName.empty()) {
		sserialize::err("UByteArrayAdapter::writeToDisk", "Could not find a file");
		return UByteArrayAdapter();
	}
	if (! MmappedFile::createFile(fileName, m_len) ) {
		sserialize::err("UByteArrayAdapter::writeToDisk", "Fatal: could not create file: " + fileName);
		return UByteArrayAdapter();
	}
	MmappedFile tempFile = MmappedFile(fileName, true);
	if (! tempFile.open() ) {
		sserialize::err("UByteArrayAdapter::writeToDisk", "Fatal: could not open file");
		return UByteArrayAdapter();
	}
	RCPtrWrapper<UByteArrayAdapterPrivate> priv(new UByteArrayAdapterPrivateMmappedFile(tempFile));
	priv->setDeleteOnClose(deleteOnClose);
	UByteArrayAdapter adap(priv);
	adap.m_len = m_len;
	adap.m_offSet = 0;

	//Now copy contents
	uint8_t* tempFileData = tempFile.data();
	for(uint32_t i = 0; i < m_len; i++) {
		tempFileData[i] = m_priv->getUint8(i);
	}
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::createCache(UByteArrayAdapter::OffsetType size, sserialize::MmappedMemoryType mmt) {
	if (size == 0)
		size = 1;

	sserialize::RCPtrWrapper<UByteArrayAdapterPrivate> priv;
	switch(mmt) {
	case sserialize::MM_FILEBASED:
		{
			MmappedFile tempFile;
			if (!MmappedFile::createTempFile(m_tempFilePrefix, size, tempFile)) {
				throw sserialize::CreationException("UByteArrayAdapter::createCache: could not open file");
				return UByteArrayAdapter();
			}
			priv.reset( new UByteArrayAdapterPrivateMmappedFile(tempFile) );
		}
		break;
	case sserialize::MM_SHARED_MEMORY:
		{
			MmappedMemory<uint8_t> mm(size, mmt);
			if (mm.size() != size) {
				throw sserialize::CreationException("UByteArrayAdapter::createCache: could not create memory maps");
			}
			priv.reset( new UByteArrayAdapterPrivateMM(mm) );
		}
		break;
	case sserialize::MM_PROGRAM_MEMORY:
		{
			priv.reset( new UByteArrayAdapterPrivateVector(new std::vector<uint8_t>(size, 0)) );
		}
		break;
	default:
		throw sserialize::CreationException("UByteArrayAdapter::createCache: unknown allocation type");
		break;
	}
	priv->setDeleteOnClose(true);
	UByteArrayAdapter adap(priv);
	adap.m_len = size;
	adap.m_offSet = 0;
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::createFile(UByteArrayAdapter::OffsetType size, std::string fileName) {
	if (! MmappedFile::createFile(fileName, size) ) {
		sserialize::err("UByteArrayAdapter::createFile", "Could not create file: " + fileName);
		return UByteArrayAdapter();
	}
	MmappedFile tempFile = MmappedFile(fileName, true);
	if (! tempFile.open() ) { //TODO:delete file if it exists
		sserialize::err("UByteArrayAdapter::createFile", "Fatal: could not open file: " + fileName);
		return UByteArrayAdapter();
	}
	tempFile.setSyncOnClose(true);
	sserialize::RCPtrWrapper<UByteArrayAdapterPrivate> priv( new UByteArrayAdapterPrivateMmappedFile(tempFile) );
	priv->setDeleteOnClose(false);
	UByteArrayAdapter adap(priv);
	adap.m_len = size;
	adap.m_offSet = 0;
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::open(const std::string& fileName) {
	MmappedFile f(fileName, false);
	if (!f.open()) {
		return UByteArrayAdapter();
	}
	else {
		return f.dataAdapter();
	}
}

UByteArrayAdapter UByteArrayAdapter::openRo(const std::string & fileName, bool compressed, UByteArrayAdapter::OffsetType maxFullMapSize, uint8_t chunkSizeExponent) {
	UByteArrayAdapter indexAdapter;
	if (compressed) {
		CompressedMmappedFile file(fileName);
		if (file.open()) {
			indexAdapter = UByteArrayAdapter(file);
		}
	}
	else {
		if (MmappedFile::fileSize(fileName) > maxFullMapSize) {
			ChunkedMmappedFile file(fileName, chunkSizeExponent, false);
			if (file.open()) {
				indexAdapter = UByteArrayAdapter(file);
			}
		}
		else {
			MmappedFile file(fileName, false);
			if (file.open()) {
				indexAdapter = UByteArrayAdapter(file);
			}
		}
	}
	return indexAdapter;
}


std::string UByteArrayAdapter::getTempFilePrefix() {
	return m_tempFilePrefix;
}

std::string UByteArrayAdapter::getFastTempFilePrefix() {
	return m_fastTempFilePrefix;
}

std::string UByteArrayAdapter::getLogFilePrefix() {
	return m_logFilePrefix;
}

void UByteArrayAdapter::setTempFilePrefix(const std::string& path) {
	m_tempFilePrefix = path;
}

void UByteArrayAdapter::setLogFilePrefix(const std::string& path) {
	m_logFilePrefix = path;
}

void UByteArrayAdapter::setFastTempFilePrefix(const std::string & path) {
	m_fastTempFilePrefix = path;
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::getOffset() {
	OffsetType res = getOffset(m_getPtr);
	m_getPtr += SSERIALIZED_OFFSET_BYTE_COUNT;
	return res;
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset() {
	NegativeOffsetType res = getNegativeOffset(m_getPtr);
	m_getPtr += SSERIALIZED_NEGATIVE_OFFSET_BYTE_COUNT;
	return res;
}
#define UBA_STREAMING_GET_FUNC(__NAME, __TYPE, __LENGTH) \
__TYPE UByteArrayAdapter::__NAME() { \
	__TYPE res = __NAME(m_getPtr); \
	m_getPtr += __LENGTH; \
	return res; \
} \

UBA_STREAMING_GET_FUNC(getUint64, uint64_t, 8);
UBA_STREAMING_GET_FUNC(getInt64, int64_t, 8);
UBA_STREAMING_GET_FUNC(getUint32, uint32_t, 4);
UBA_STREAMING_GET_FUNC(getInt32, int32_t, 4);
UBA_STREAMING_GET_FUNC(getUint24, uint32_t, 3);
UBA_STREAMING_GET_FUNC(getUint16, uint16_t, 2);
UBA_STREAMING_GET_FUNC(getUint8, uint8_t, 1);
UBA_STREAMING_GET_FUNC(getDouble, double, 8);
UBA_STREAMING_GET_FUNC(getFloat, float, 4);
#undef UBA_STREAMING_GET_FUNC

uint64_t UByteArrayAdapter::getVlPackedUint64() {
	int len = -1;
	uint64_t res = getVlPackedUint64(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

int64_t UByteArrayAdapter::getVlPackedInt64() {
	int len = -1;
	int64_t res = getVlPackedInt64(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

uint32_t UByteArrayAdapter::getVlPackedUint32() {
	if (m_len < m_getPtr+1)
		return 0; //we need to read at least one byte
	int len = std::min<OffsetType>(5, m_len - m_getPtr);
	uint32_t res = m_priv->getVlPackedUint32(m_offSet+m_getPtr, &len);
	if (len < 0)
		return 0;
	m_getPtr += len;
	return res;
}

int32_t UByteArrayAdapter::getVlPackedInt32() {
	int len = -1;
	int32_t res = getVlPackedInt32(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::get(uint8_t * dest, UByteArrayAdapter::OffsetType len) {
	len = get(m_getPtr, dest, len);
	m_getPtr += len;
	return len;
}

std::string UByteArrayAdapter::getString() {
	uint32_t strLen = getStringLength();
	if (!strLen || m_getPtr+strLen > m_len)
		return std::string();
	std::string res;
	res.resize(strLen);

	for(size_t i = 0; i < strLen; i++) {
		res[i] = getUint8(m_getPtr+i);
	}
	m_getPtr += res.size();
	return res;
}

UByteArrayAdapter UByteArrayAdapter::getStringData() {
	int len = -1;
	UByteArrayAdapter s(getStringData(m_getPtr, &len));
	if (len > 0)
		m_getPtr += len;
	return s;
}


bool UByteArrayAdapter::resizeForPush(OffsetType pos, OffsetType length) {
	if (pos+length > m_len) {
		return growStorage(pos+length-m_len);
	}
	return true;
}

#define UBA_PUT_STREAMING_FUNC(__NAME, __TYPE, __LENGTH) \
bool UByteArrayAdapter::__NAME(const __TYPE value) { \
	if (!resizeForPush(m_putPtr, __LENGTH)) \
		return false; \
	bool ok = __NAME(m_putPtr, value); \
	if (ok) \
		m_putPtr += __LENGTH; \
	return ok; \
} \

UBA_PUT_STREAMING_FUNC(putOffset, OffsetType, 5);
UBA_PUT_STREAMING_FUNC(putNegativeOffset, NegativeOffsetType, 5);
UBA_PUT_STREAMING_FUNC(putInt64, int64_t, 8);
UBA_PUT_STREAMING_FUNC(putUint64, uint64_t, 8);
UBA_PUT_STREAMING_FUNC(putInt32, int32_t, 4);
UBA_PUT_STREAMING_FUNC(putUint32, uint32_t, 4);
UBA_PUT_STREAMING_FUNC(putUint24, uint32_t, 3);
UBA_PUT_STREAMING_FUNC(putUint16, uint16_t, 2);
UBA_PUT_STREAMING_FUNC(putUint8, uint8_t, 1);
UBA_PUT_STREAMING_FUNC(putDouble, double, 8);
UBA_PUT_STREAMING_FUNC(putFloat, float, 4);

#undef UBA_PUT_STREAMING_FUNC

#define UBA_PUT_VL_STREAMING_FUNC(__NAME, __BUFSIZE, __SERFUNC, __TYPE) \
int UByteArrayAdapter::__NAME(const __TYPE value) { \
	uint8_t tmp[__BUFSIZE]; \
	int len = __SERFUNC(value, tmp); \
	if (len < 0) \
		return -1; \
	put(tmp, len); \
	return len; \
} \

UBA_PUT_VL_STREAMING_FUNC(putVlPackedUint64, 9, p_vu64, uint64_t);
UBA_PUT_VL_STREAMING_FUNC(putVlPackedInt64, 9, p_vs64, int64_t);
UBA_PUT_VL_STREAMING_FUNC(putVlPackedUint32, 5, p_vu32, uint32_t);
UBA_PUT_VL_STREAMING_FUNC(putVlPackedInt32, 5, p_vs32, int32_t);

#undef UBA_PUT_VL_STREAMING_FUNC

int UByteArrayAdapter::putVlPackedPad4Uint32(const uint32_t value) {
	if (!resizeForPush(m_putPtr, 4))
		return false;

	int len = putVlPackedPad4Uint32(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;
	
	return len;
}

int UByteArrayAdapter::putVlPackedPad4Int32(const int32_t value) {
	if (!resizeForPush(m_putPtr, 4))
		return false;

	int len = putVlPackedInt32(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;
	return len;
}

bool UByteArrayAdapter::put(const std::string& str) {
	UByteArrayAdapter::OffsetType needSize = psize_vu32(str.size()) + str.size();
	if (!resizeForPush(m_putPtr, needSize))
		return false;
	int pushedBytes = put(m_putPtr, str);
	if (pushedBytes >= 0) {
		m_putPtr+= needSize;
		return true;
	}
	return false;
}

bool UByteArrayAdapter::put(const uint8_t * data, UByteArrayAdapter::OffsetType len) {
	UByteArrayAdapter::OffsetType needSize = len;
	if (!resizeForPush(m_putPtr, needSize))
		return false;
	bool ok = put(m_putPtr, data, len);
	if (ok) {
		m_putPtr += needSize;
		return true;
	}
	return false;
}

bool UByteArrayAdapter::put(const std::deque< uint8_t >& data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize))
		return false;
	bool ok = put(m_putPtr, data);
	if (ok) {
		m_putPtr+= needSize;
		return true;
	}
	return false;
}

bool UByteArrayAdapter::put(const std::vector< uint8_t >& data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize))
		return false;
	bool ok = put(m_putPtr, data);
	if (ok) {
		m_putPtr+= needSize;
		return true;
	}
	return false;
}

bool UByteArrayAdapter::put(const UByteArrayAdapter & data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize))
		return false;
	bool ok = put(m_putPtr, data);
	if (ok) {
		m_putPtr+= needSize;
		return true;
	}
	return false;
}

std::string UByteArrayAdapter::toString() const {
	std::string str;
	str.resize(size());
	for(OffsetType i(0), s(size()); i < s; ++i) {
		str[i] = at(i);
	}
	return str;
}

void UByteArrayAdapter::dump(uint32_t byteCount) const {
	uint32_t dumpLen = byteCount;
	if (m_len < dumpLen)
		dumpLen = m_len;
	for(size_t i = 0; i < dumpLen; i++) {
		std::cout << static_cast<uint32_t>(at(i)) << ":";
	}
	std::cout << std::endl;
}

void UByteArrayAdapter::dumpAsString(uint32_t byteCount) const {
	uint32_t dumpLen = byteCount;
	if (m_len < dumpLen)
		dumpLen = m_len;
	for(size_t i = 0; i < dumpLen; i++) {
		std::cout << at(i) << ":";
	}
	std::cout << std::endl;
}

#define STATIC_PUT_FUNCS_MAKRO(__NAME, __TYPE) void UByteArrayAdapter::__NAME(UByteArrayAdapter & dest, __TYPE src) { dest.__NAME(src); }
STATIC_PUT_FUNCS_MAKRO(putUint8, uint8_t);
STATIC_PUT_FUNCS_MAKRO(putUint16, uint16_t);
STATIC_PUT_FUNCS_MAKRO(putUint24, uint32_t);
STATIC_PUT_FUNCS_MAKRO(putUint32, uint32_t);
STATIC_PUT_FUNCS_MAKRO(putInt32, int32_t);
STATIC_PUT_FUNCS_MAKRO(putUint64, uint64_t);
STATIC_PUT_FUNCS_MAKRO(putInt64, int64_t);
STATIC_PUT_FUNCS_MAKRO(putDouble, double);
STATIC_PUT_FUNCS_MAKRO(putFloat, float);
STATIC_PUT_FUNCS_MAKRO(putVlPackedInt32, int32_t);
STATIC_PUT_FUNCS_MAKRO(putVlPackedUint32, uint32_t);
STATIC_PUT_FUNCS_MAKRO(putVlPackedInt64, int64_t);
STATIC_PUT_FUNCS_MAKRO(putVlPackedUint64, uint64_t);
#undef STATIC_PUT_FUNCS_MAKRO

using namespace sserialize;

// sserialize::UByteArrayAdapter& operator--(sserialize::UByteArrayAdapter& a) {
// 	return a.operator--(1);
// }
// 
// sserialize::UByteArrayAdapter& operator++(sserialize::UByteArrayAdapter& a) {
// 	return a.operator--(1);
// }

bool operator==(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b) {
	return a.equal(b);
}

bool operator!=(const sserialize::UByteArrayAdapter & a, const sserialize::UByteArrayAdapter & b) {
	return ! a.equal(b);
}

bool operator==(const std::deque<uint8_t> & a, const sserialize::UByteArrayAdapter& b) {
	return b.equalContent(a);
}

bool operator!=(const std::deque<uint8_t> & a, const sserialize::UByteArrayAdapter& b) {
	return !b.equalContent(a);
}

bool operator==(const UByteArrayAdapter& b, const std::deque< uint8_t >& a) {
	return b.equalContent(a);
}

bool operator!=(const UByteArrayAdapter& b, const std::deque< uint8_t >& a) {
	return !b.equalContent(a);
}


//Streaming operators

#define UBA_OPERATOR_PUT_STREAMING_FUNC(__NAME, __TYPE) \
UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const __TYPE value) { \
	data.__NAME(value); \
	return data; \
} \

UBA_OPERATOR_PUT_STREAMING_FUNC(putInt64, int64_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putInt32, int32_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putUint64, uint64_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putUint32, uint32_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putUint16, uint16_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putUint8, uint8_t);
UBA_OPERATOR_PUT_STREAMING_FUNC(putDouble, double);
UBA_OPERATOR_PUT_STREAMING_FUNC(putFloat, float);
UBA_OPERATOR_PUT_STREAMING_FUNC(put, std::string &);
UBA_OPERATOR_PUT_STREAMING_FUNC(put, std::deque<uint8_t> &);
UBA_OPERATOR_PUT_STREAMING_FUNC(put, std::vector<uint8_t> &);
UBA_OPERATOR_PUT_STREAMING_FUNC(put, UByteArrayAdapter &);

#undef UBA_OPERATOR_PUT_STREAMING_FUNC


#define UBA_OPERATOR_GET_STREAMING_FUNC(__NAME, __TYPE) \
UByteArrayAdapter& operator>>(UByteArrayAdapter & data, __TYPE & value) { \
	value = data.__NAME(); \
	return data; \
} \

UBA_OPERATOR_GET_STREAMING_FUNC(getInt64, int64_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getInt32, int32_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getUint64, uint64_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getUint32, uint32_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getUint16, uint16_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getUint8, uint8_t);
UBA_OPERATOR_GET_STREAMING_FUNC(getDouble, double);
UBA_OPERATOR_GET_STREAMING_FUNC(getFloat, float);
UBA_OPERATOR_GET_STREAMING_FUNC(getString, std::string);

#undef UBA_OPERATOR_STREAMING_FUNC

}//end namespace