#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/log.h>
#include "utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivates.h"
#include <iostream>


#ifndef TEMP_DIR_PATH
#ifdef __ANDROID__
#define TEMP_DIR_PATH "/sdcard/osmfind/tmpfile"
#define PERSISTENT_CACHE_PATH "/sdcard/osmfind/pcachefile"
#else
#define TEMP_DIR_PATH "/tmp/osmfindtmpfile"
#define PERSISTENT_CACHE_PATH "/tmp/pcachefile"
#endif
#endif

#ifdef __ANDROID__
#define MAX_IN_MEMORY_CACHE (10*1024*1024)
#else
#define MAX_IN_MEMORY_CACHE (10*1024*1024)
#endif

#define OFFSET_BYTE_COUNT 5
#define NEGATIVE_OFFSET_BYTE_COUNT 5

namespace sserialize {

std::string UByteArrayAdapter::m_tempDirPath = TEMP_DIR_PATH;

UByteArrayAdapter::UByteArrayAdapter(UByteArrayAdapterPrivate * priv) :
m_priv(priv),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}


UByteArrayAdapter::UByteArrayAdapter() :
m_priv(0),
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
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, bool deleteOnClose) :
m_priv(new UByteArrayAdapterPrivateDeque(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv) {
		m_priv->refInc();
		m_priv->setDeleteOnClose(deleteOnClose);
	}
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, bool deleteOnClose) :
m_priv(new UByteArrayAdapterPrivateVector(data)),
m_offSet(0),
m_len(data->size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv) {
		m_priv->refInc();
		m_priv->setDeleteOnClose(deleteOnClose);
	}
}


UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter) :
m_priv(adapter.m_priv),
m_offSet(adapter.m_offSet),
m_len(adapter.m_len),
m_getPtr(adapter.m_getPtr),
m_putPtr(adapter.m_putPtr)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter, OffsetType addOffset) :
m_priv(0),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
	if (adapter.m_priv) {
		adapter.m_priv->refInc();
	}
	if (m_priv) {
		m_priv->refDec();
	}
	m_priv = adapter.m_priv;
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
m_priv(0),
m_offSet(0),
m_len(0),
m_getPtr(0),
m_putPtr(0)
{
	if (adapter.m_priv) {
		adapter.m_priv->refInc();
	}
	if (m_priv) {
		m_priv->refDec();
	}
	m_priv = adapter.m_priv;
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

// UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter& adapter, int addOffset) {
// 	
// }


UByteArrayAdapter::UByteArrayAdapter(UByteArrayAdapterPrivate * priv, OffsetType offSet, OffsetType len) :
m_priv(priv),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv) {
		m_priv->refInc();
	}
	else {
		m_offSet = 0;
		m_len = 0;
	}
}

UByteArrayAdapter::UByteArrayAdapter(MmappedFile file, OffsetType offSet, OffsetType len) :
m_priv(new UByteArrayAdapterPrivateMmappedFile(file)),
m_offSet(offSet),
m_len(len),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}


UByteArrayAdapter::UByteArrayAdapter(MmappedFile file) :
m_priv(new UByteArrayAdapterPrivateMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(const ChunkedMmappedFile & file) :
m_priv(new UByteArrayAdapterPrivateChunkedMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::UByteArrayAdapter(const CompressedMmappedFile & file) :
m_priv(new UByteArrayAdapterPrivateCompressedMmappedFile(file)),
m_offSet(0),
m_len(file.size()),
m_getPtr(0),
m_putPtr(0)
{
	if (m_priv)
		m_priv->refInc();
}

UByteArrayAdapter::~UByteArrayAdapter() {
	if (m_priv) {
		m_priv->refDec();
	}
}

UByteArrayAdapter & UByteArrayAdapter::operator=(const UByteArrayAdapter & adapter) {
	if (adapter.m_priv) {
		adapter.m_priv->refInc();
	}
	if (m_priv) {
		m_priv->refDec();
	}
	m_priv = adapter.m_priv;
	m_offSet = adapter.m_offSet;
	m_len = adapter.m_len;
	m_getPtr = adapter.m_getPtr;
	m_putPtr = adapter.m_putPtr;
	return *this;
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

void UByteArrayAdapter::resetGetPtr() {
	m_getPtr = 0;
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

void UByteArrayAdapter::resetPtrs() {
	resetGetPtr();
	resetPutPtr();
}


bool UByteArrayAdapter::shrinkStorage(OffsetType byte) {
	if (m_priv) {
		if (byte > m_len)
			byte = m_len;
		bool ok = m_priv->shrinkStorage(m_offSet+m_len-byte);
		if (ok)
			m_len -= byte;
		return ok;
	}
	return false;
}

bool UByteArrayAdapter::growStorage(OffsetType byte) {
	if (m_priv) {
		bool ok;
		if (m_offSet+m_len+byte < m_offSet+m_len) //check for wrapp-around
			ok = false;
		else
			ok = m_priv->growStorage(m_offSet+m_len+byte);
		if (ok)
			m_len += byte;
		return ok;
	}
	return false;
}

bool UByteArrayAdapter::resize(OffsetType byte) {
	if (m_priv) {
		if (byte < m_len) {
			m_len = byte;
			if (m_getPtr < m_len) {
				m_getPtr = m_len;
			}
			if (m_putPtr < m_len) {
				m_putPtr = m_len;
			}
		}
		else {
			growStorage(byte-m_len);
		}
	}
	return false;
}


void UByteArrayAdapter::setDeleteOnClose(bool del) {
	if (m_priv)
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

UByteArrayAdapter UByteArrayAdapter::begin() const {
	return *this;
}

UByteArrayAdapter UByteArrayAdapter::end() const {
	return UByteArrayAdapter(*this, m_len);
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
	if (m_len < pos+OFFSET_BYTE_COUNT) return 0;
	return m_priv->getOffset(m_offSet+pos);
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset(const OffsetType pos) const {
	if (m_len < pos+NEGATIVE_OFFSET_BYTE_COUNT) return 0;
	return m_priv->getNegativeOffset(m_offSet+pos);
}

std::string UByteArrayAdapter::getString(const OffsetType pos, int * length) const {
	int len;
	uint32_t strLen = getVlPackedUint32(pos, &len);
	if (len < 0 || pos+len+strLen > m_len) {
		if (length)
			*length = -1;
		return std::string();
	}
	std::string res;
	res.resize(strLen);

	for(size_t i = 0; i < strLen; i++) {
		res[i] = getUint8(pos+len+i);
	}
	if (length)
		*length = strLen+len;
	return res;
}

UByteArrayAdapter UByteArrayAdapter::getStringData(const OffsetType pos, int * length) const {
	int len;
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
	m_priv->get(pos, dest, len);
	return len;
}

/** If the supplied memory is not writable then you're on your own! **/

bool UByteArrayAdapter::putOffset(const OffsetType pos, const OffsetType value) {
	if (m_len < pos+OFFSET_BYTE_COUNT)
		return false;
	m_priv->putOffset(m_offSet+pos, value);
	return true;
}

bool UByteArrayAdapter::putNegativeOffset(const OffsetType pos, const NegativeOffsetType value) {
	if (m_len < pos+NEGATIVE_OFFSET_BYTE_COUNT) return false;
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
	if (m_len < pos+4) return false;
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
	uint32_t needSize = vl_pack_uint32_t_size(str.size()) + str.size();
	if (m_len < pos+needSize)
		return -1;
	int len = putVlPackedUint32(pos, str.size());
	for(size_t i = 0; i < str.size(); i++) {
		putUint8(pos+len+i, str[i]);
	}
	return needSize;
}

bool UByteArrayAdapter::put(OffsetType pos, const uint8_t * data, uint32_t len) {
	if (m_len < pos+len)
		return false;
	m_priv->put(pos, data, len);
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
	if (m_len < pos+data.size())
		return false;
	for(size_t i = 0; i < data.size(); i++) {
		putUint8(pos+i, data[i]);
	}
	return true;
}

bool UByteArrayAdapter::put(const OffsetType pos, const UByteArrayAdapter & data) {
	if (m_len < pos+data.size())
		return false;
	for(size_t i = 0; i < data.size(); i++) {
		putUint8(pos+i, data.getUint8(i));
	}
	return true;
}


UByteArrayAdapter UByteArrayAdapter::writeToDisk(std::string fileName, bool deleteOnClose) {
	UByteArrayAdapterPrivate * priv = 0;
	if (fileName.empty()) {
		fileName = MmappedFile::findLockFilePath(PERSISTENT_CACHE_PATH, 2048);
	}
	if (fileName.empty()) {
		osmfindlog::err("UByteArrayAdapter::writeToDisk", "Could not find a file");
		return UByteArrayAdapter();
	}
	if (! MmappedFile::createFile(fileName, m_len) ) {
		osmfindlog::err("UByteArrayAdapter::writeToDisk", "Fatal: could not create file: " + fileName);
		return UByteArrayAdapter();
	}
	MmappedFile tempFile = MmappedFile(fileName, true);
	if (! tempFile.open() ) {
		osmfindlog::err("UByteArrayAdapter::writeToDisk", "Fatal: could not open file");
		return UByteArrayAdapter();
	}
	priv = new UByteArrayAdapterPrivateMmappedFile(tempFile);
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

UByteArrayAdapter UByteArrayAdapter::createCache(UByteArrayAdapter::OffsetType size, bool forceFileBase) {
	if (size == 0)
		size = 1;

	UByteArrayAdapterPrivate * priv = 0;
	if (forceFileBase || size > MAX_IN_MEMORY_CACHE) {
		std::string tempFileName = MmappedFile::findLockFilePath(m_tempDirPath, 2048);
		if (tempFileName.empty()) {
			osmfindlog::err("UByteArrayAdapter::createCache", "Could not find a cache file");
			return UByteArrayAdapter();
		}
		if (! MmappedFile::createFile(tempFileName, size) ) {
			osmfindlog::err("UByteArrayAdapter::createCache", "Fatal: could not create temp file: " + tempFileName);
			return UByteArrayAdapter();
		}
		MmappedFile tempFile = MmappedFile(tempFileName, true);
		if (! tempFile.open() ) { //TODO:delete file if it exists
			osmfindlog::err("UByteArrayAdapter::createCache", "Fatal: could not open file");
			return UByteArrayAdapter();
		}
		priv = new UByteArrayAdapterPrivateMmappedFile(tempFile);
	}
	else {
		std::vector<uint8_t> * privData = new std::vector<uint8_t>();
		privData->resize(size, 0);
		if (privData) {
			priv = new UByteArrayAdapterPrivateVector(privData);
		}
		else {
			osmfindlog::err("UByteArrayAdapter::createCache", "Could not allocate memory. Trying file based.");
			return createCache(size, true);
		}
	}
	priv->setDeleteOnClose(true);
	UByteArrayAdapter adap(priv);
	adap.m_len = size;
	adap.m_offSet = 0;
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::createFile(UByteArrayAdapter::OffsetType size, std::string fileName) {
	if (! MmappedFile::createFile(fileName, size) ) {
		osmfindlog::err("UByteArrayAdapter::createFile", "Could not create file: " + fileName);
		return UByteArrayAdapter();
	}
	MmappedFile tempFile = MmappedFile(fileName, true);
	if (! tempFile.open() ) { //TODO:delete file if it exists
		osmfindlog::err("UByteArrayAdapter::createFile", "Fatal: could not open file: " + fileName);
		return UByteArrayAdapter();
	}
	tempFile.setSyncOnClose(true);
	UByteArrayAdapterPrivate * priv = new UByteArrayAdapterPrivateMmappedFile(tempFile);
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


std::string UByteArrayAdapter::getTempDirPath() {
	return m_tempDirPath;
}

void UByteArrayAdapter::setTempDirPath(const std::string& path) {
	m_tempDirPath = path;
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::getOffset() {
	OffsetType res = getOffset(m_getPtr);
	m_getPtr += OFFSET_BYTE_COUNT;
	return res;
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset() {
	NegativeOffsetType res = getNegativeOffset(m_getPtr);
	m_getPtr += NEGATIVE_OFFSET_BYTE_COUNT;
	return res;
}

uint64_t UByteArrayAdapter::getUint64() {
	uint64_t res = getUint64(m_getPtr);
	m_getPtr += 8;
	return res;
}

int64_t UByteArrayAdapter::getInt64() {
	int64_t res = getInt64(m_getPtr);
	m_getPtr += 8;
	return res;
}

int32_t UByteArrayAdapter::getInt32() {
	int32_t res = getInt32(m_getPtr);
	m_getPtr += 4;
	return res;
}

uint32_t UByteArrayAdapter::getUint32() {
	uint32_t res = getUint32(m_getPtr);
	m_getPtr += 4;
	return res;
}

uint32_t UByteArrayAdapter::getUint24() {
	uint32_t res = getUint24(m_getPtr);
	m_getPtr += 3;
	return res;
}

uint16_t UByteArrayAdapter::getUint16() {
	uint16_t res = getUint16(m_getPtr);
	m_getPtr += 2;
	return res;
}

uint8_t UByteArrayAdapter::getUint8() {
	uint8_t res = getUint8(m_getPtr);
	m_getPtr += 1;
	return res;
}

uint64_t UByteArrayAdapter::getVlPackedUint64() {
	int len;
	uint32_t res = getVlPackedUint64(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

int64_t UByteArrayAdapter::getVlPackedInt64() {
	int len;
	uint32_t res = getVlPackedInt64(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

uint32_t UByteArrayAdapter::getVlPackedUint32() {
	int len;
	uint32_t res = getVlPackedUint32(m_getPtr, &len);
	if (len > 0)
		m_getPtr += len;
	return res;
}

int32_t UByteArrayAdapter::getVlPackedInt32() {
	int len;
	uint32_t res = getVlPackedInt32(m_getPtr, &len);
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
	int len;
	uint32_t strLen = getVlPackedUint32(m_getPtr, &len);
	if (len < 0 || m_getPtr+len+strLen > m_len)
		return std::string();
	std::string res;
	res.resize(strLen);

	for(size_t i = 0; i < strLen; i++) {
		res[i] = getUint8(m_getPtr+len+i);
	}
	m_getPtr += len + res.size();
	return res;
}

UByteArrayAdapter UByteArrayAdapter::getStringData() {
	int len;
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

bool UByteArrayAdapter::putOffset(const OffsetType value) {
	if (!resizeForPush(m_putPtr, 5))
		return false;
	bool ok = putOffset(m_putPtr, value);
	if (ok)
		m_putPtr += 5;
	return ok;
}

bool UByteArrayAdapter::putNegativeOffset(const NegativeOffsetType value) {
	if (!resizeForPush(m_putPtr, 5))
		return false;
	bool ok = putNegativeOffset(m_putPtr, value);
	if (ok)
		m_putPtr += 5;
	return ok;
}

bool UByteArrayAdapter::putInt64(const int64_t value) {
	if (!resizeForPush(m_putPtr, 8))
		return false;
	bool ok = putInt64(m_putPtr, value);
	if (ok)
		m_putPtr += 8;
	return ok;
}

bool UByteArrayAdapter::putUint64(const uint64_t value) {
	if (!resizeForPush(m_putPtr, 8))
		return false;
	bool ok = putUint64(m_putPtr, value);
	if (ok)
		m_putPtr += 8;
	return ok;
}

bool UByteArrayAdapter::putInt32(const int32_t value) {
	if (!resizeForPush(m_putPtr, 4))
		return false;
	bool ok = putInt32(m_putPtr, value);
	if (ok)
		m_putPtr += 4;
	return ok;
}

bool UByteArrayAdapter::putUint32(const uint32_t value) {
	if (!resizeForPush(m_putPtr, 4))
		return false;
	bool ok = putUint32(m_putPtr, value);
	if (ok)
		m_putPtr += 4;
	return ok;
}

bool UByteArrayAdapter::putUint24(const uint32_t value) {
	if (!resizeForPush(m_putPtr, 3))
		return false;
	bool ok = putUint24(m_putPtr, value);
	if (ok)
		m_putPtr += 3;
	return ok;
}

bool UByteArrayAdapter::putUint16(const uint16_t value) {
	if (!resizeForPush(m_putPtr, 2))
		return false;
	bool ok = putUint16(m_putPtr, value);
	if (ok)
		m_putPtr += 2;
	return ok;
}

bool UByteArrayAdapter::putUint8(const uint8_t value) {
	if (!resizeForPush(m_putPtr, 1))
		return false;
	bool ok = putUint8(m_putPtr, value);
	if (ok)
		m_putPtr += 1;
	return ok;
}

int UByteArrayAdapter::putVlPackedUint64(const uint64_t value) {
	int len = vl_pack_uint64_t_size(value);
	if (len < 0)
		return false;
	if (!resizeForPush(m_putPtr, len))
		return false;

	len = putVlPackedUint64(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;

	return len;
}

int UByteArrayAdapter::putVlPackedInt64(const int64_t value) {
	int len = vl_pack_int64_t_size(value);
	if (len < 0)
		return false;
	if (!resizeForPush(m_putPtr, len))
		return false;

	len = putVlPackedInt64(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;

	return len;
}

int UByteArrayAdapter::putVlPackedUint32(const uint32_t value) {
	int len = vl_pack_uint32_t_size(value);
	if (len < 0)
		return false;
	if (!resizeForPush(m_putPtr, len))
		return false;

	len = putVlPackedUint32(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;

	return len;
}

int UByteArrayAdapter::putVlPackedPad4Uint32(const uint32_t value) {
	if (!resizeForPush(m_putPtr, 4))
		return false;

	int len = putVlPackedPad4Uint32(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;
	
	return len;
}

int UByteArrayAdapter::putVlPackedInt32(const int32_t value) {
	int len = vl_pack_int32_t_size(value);
	if (len < 0)
		return false;
	if (!resizeForPush(m_putPtr, len))
		return false;

	len = putVlPackedInt32(m_putPtr, value);

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
	uint32_t needSize = vl_pack_uint32_t_size(str.size()) + str.size();
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
	uint32_t needSize = len;
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
	uint32_t needSize = data.size();
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
	uint32_t needSize = data.size();
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
	uint32_t needSize = data.size();
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
	for(uint32_t i = 0; i < size(); i++) {
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

}//end namespace

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

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const int64_t value) {
	data.putInt64(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const uint64_t value) {
	data.putUint64(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const int32_t value) {
	data.putInt32(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const uint32_t value) {
	data.putUint32(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const uint16_t value) {
	data.putUint16(value);
	return data;
}


UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const uint8_t value) {
	data.putUint8(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const std::string & value) {
	data.put(value);
	return data;
}


UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const std::deque<uint8_t> & value) {
	data.put(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const std::vector<uint8_t> & value) {
	data.put(value);
	return data;
}

UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const UByteArrayAdapter & value) {
	data.put(value);
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, int64_t & value) {
	value = data.getInt64();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, uint64_t & value) {
	value = data.getUint64();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, int32_t & value) {
	value = data.getInt32();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, uint32_t & value) {
	value = data.getUint32();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, uint16_t & value) {
	value = data.getUint16();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, uint8_t & value) {
	value = data.getUint8();
	return data;
}

UByteArrayAdapter& operator>>(UByteArrayAdapter & data, std::string & value) {
	value = data.getString();
	return data;
}
