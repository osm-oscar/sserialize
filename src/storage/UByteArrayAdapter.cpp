#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/log.h>
#include "UByteArrayAdapterPrivates/UByteArrayAdapterPrivates.h"
#include <iostream>
#include <sserialize/utility/types.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/utility/assert.h>


namespace sserialize {


std::string UByteArrayAdapter::m_tempFilePrefix = SSERIALIZE_TEMP_FILE_PREFIX;
std::string UByteArrayAdapter::m_fastTempFilePrefix = SSERIALIZE_TEMP_FILE_PREFIX;
std::string UByteArrayAdapter::m_logFilePrefix = SSERIALIZE_TEMP_FILE_PREFIX;

namespace detail {
namespace __UByteArrayAdapter {

MemoryView::MemoryViewImp::MemoryViewImp(uint8_t* ptr, sserialize::OffsetType off, sserialize::OffsetType size, bool isCopy, sserialize::detail::__UByteArrayAdapter::MemoryView::MyPrivate* base) :
m_dataBase(base),
m_d(ptr),
m_off(off),
m_size(size),
m_copy(isCopy)
{}

MemoryView::MemoryViewImp::~MemoryViewImp() {
	if (isCopy()) {
		delete[] m_d;
	}
}

bool MemoryView::MemoryViewImp::flush(OffsetType len, OffsetType off) {
	if (isCopy()) {
		if (off > m_size) {
			return false;
		}
		len = std::min<OffsetType>(len, m_size-off);
		m_dataBase->put(m_off+off, m_d+off, len);
	}
	return true;
}

sserialize::UByteArrayAdapter MemoryView::MemoryViewImp::dataBase() const {
	return sserialize::UByteArrayAdapter(m_dataBase, m_off, m_size);
}

sserialize::UByteArrayAdapter MemoryView::dataBase() const {
	return m_priv->dataBase();
}

}}//end namespace detail::__UByteArrayAdapter


UByteArrayAdapter::OpenFlags::OpenFlags() :
m_v(static_cast<underlying_type>(Values::None))
{}

UByteArrayAdapter::OpenFlags::~OpenFlags()
{}

UByteArrayAdapter::OpenFlags
UByteArrayAdapter::OpenFlags::None() {
	return OpenFlags(Values::None);
}

UByteArrayAdapter::OpenFlags
UByteArrayAdapter::OpenFlags::DirectIo() {
	return OpenFlags(Values::DirectIo);
}

UByteArrayAdapter::OpenFlags
UByteArrayAdapter::OpenFlags::Compressed() {
	return OpenFlags(Values::Compressed);
}

UByteArrayAdapter::OpenFlags
UByteArrayAdapter::OpenFlags::Writable() {
	return OpenFlags(Values::Writable);
}

UByteArrayAdapter::OpenFlags
UByteArrayAdapter::OpenFlags::Chunked() {
	return OpenFlags(Values::Chunked);
}

//CTORS

UByteArrayAdapter::UByteArrayAdapter(const MyPrivatePtr & priv) :
UByteArrayAdapter(priv, 0, priv->size())
{}

UByteArrayAdapter::UByteArrayAdapter(const MyPrivatePtr & priv, OffsetType offSet, OffsetType len) :
UByteArrayAdapter(static_cast<MyPrivate*>(0), offSet, len)
{
	if (!priv.get()) {
		m_offSet = 0;
		m_len = 0;
	}
	else {
		m_priv = priv;
	}
}

UByteArrayAdapter::UByteArrayAdapter(MyPrivate * priv, OffsetType offSet, OffsetType len, OffsetType getPtr, OffsetType putPtr) :
m_priv(priv),
m_offSet(offSet),
m_len(len),
m_getPtr(getPtr),
m_putPtr(putPtr)
{}


UByteArrayAdapter::UByteArrayAdapter() :
UByteArrayAdapter(new UByteArrayAdapterPrivateEmpty())
{}

UByteArrayAdapter::UByteArrayAdapter(uint8_t * data, OffsetType offSet, OffsetType len) :
UByteArrayAdapter(new UByteArrayAdapterPrivateArray(data), offSet, len)
{}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, OffsetType offSet, OffsetType len) :
UByteArrayAdapter(new UByteArrayAdapterPrivateVector(data), offSet, len)
{
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data) :
UByteArrayAdapter(new UByteArrayAdapterPrivateVector(data), 0, data->size())
{
}

UByteArrayAdapter::UByteArrayAdapter(std::vector< uint8_t >* data, bool deleteOnClose) :
UByteArrayAdapter(new UByteArrayAdapterPrivateVector(data), 0, data->size())
{
	m_priv->setDeleteOnClose(deleteOnClose);
}


UByteArrayAdapter::UByteArrayAdapter(const UByteArrayAdapter & adapter) :
m_priv(adapter.m_priv),
m_offSet(adapter.m_offSet),
m_len(adapter.m_len),
m_getPtr(adapter.m_getPtr),
m_putPtr(adapter.m_putPtr)
{}

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

UByteArrayAdapter::UByteArrayAdapter(const MmappedFile & file, OffsetType offSet, OffsetType len) :
UByteArrayAdapter(new UByteArrayAdapterPrivateMmappedFile(file), offSet, len)
{
	if (file.size() < m_offSet+m_len) {
		if (file.size() > m_offSet) {
			m_len = file.size() - m_offSet;
		}
		else {
			m_len = 0;
		}
	}
}

UByteArrayAdapter::UByteArrayAdapter(const MmappedFile & file) :
UByteArrayAdapter(new UByteArrayAdapterPrivateMmappedFile(file), 0, file.size())
{}

UByteArrayAdapter::UByteArrayAdapter(const MmappedMemory<uint8_t> & mem) :
UByteArrayAdapter(new UByteArrayAdapterPrivateMM(mem), 0, mem.size())
{}


UByteArrayAdapter::UByteArrayAdapter(OffsetType size, sserialize::MmappedMemoryType mmt) :
UByteArrayAdapter( UByteArrayAdapter::createCache(size, mmt) )
{}

UByteArrayAdapter::UByteArrayAdapter(sserialize::MmappedMemoryType mmt) :
UByteArrayAdapter(0, mmt)
{}

UByteArrayAdapter::UByteArrayAdapter(OffsetType size, std::string fileName) :
UByteArrayAdapter( UByteArrayAdapter::createFile(size, fileName) )
{}

UByteArrayAdapter::UByteArrayAdapter(const MemoryView & mem) :
UByteArrayAdapter(new UByteArrayAdapterPrivateMV(mem), 0, mem.size())
{}

#ifdef SSERIALIZE_UBA_NON_CONTIGUOUS
UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, OffsetType offSet, OffsetType len) :
UByteArrayAdapter(new UByteArrayAdapterPrivateDeque(data), offSet, len)
{}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data) :
UByteArrayAdapter(new UByteArrayAdapterPrivateDeque(data), 0, data->size())
{}

UByteArrayAdapter::UByteArrayAdapter(std::deque< uint8_t >* data, bool deleteOnClose) :
UByteArrayAdapter(new UByteArrayAdapterPrivateDeque(data), 0, data->size())
{
	m_priv->setDeleteOnClose(deleteOnClose);
}

UByteArrayAdapter::UByteArrayAdapter(const ChunkedMmappedFile & file) :
UByteArrayAdapter(new UByteArrayAdapterPrivateChunkedMmappedFile(file), 0, file.size())
{}

UByteArrayAdapter::UByteArrayAdapter(const CompressedMmappedFile & file) :
UByteArrayAdapter(new UByteArrayAdapterPrivateCompressedMmappedFile(file), 0, file.size())
{}
#endif

#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL
UByteArrayAdapter::UByteArrayAdapter(std::deque<uint8_t> * /*data*/, OffsetType /*offSet*/, OffsetType /*len*/) :
UByteArrayAdapter()
{
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous access only");
}

UByteArrayAdapter::UByteArrayAdapter(std::deque<uint8_t> * /*data*/) :
UByteArrayAdapter()
{
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous access only");
}

UByteArrayAdapter::UByteArrayAdapter(std::deque<uint8_t> * /*data*/, bool /*deleteOnClose*/) :
UByteArrayAdapter()
{
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous access only");
}

UByteArrayAdapter::UByteArrayAdapter(const ChunkedMmappedFile & /*file*/) :
UByteArrayAdapter()
{
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous access only");
}

UByteArrayAdapter::UByteArrayAdapter(const CompressedMmappedFile & /*file*/) :
UByteArrayAdapter()
{
	throw sserialize::UnsupportedFeatureException("sserialize was compiled with contiguous access only");
}
#endif


UByteArrayAdapter::~UByteArrayAdapter() {}

UByteArrayAdapter & UByteArrayAdapter::operator=(const UByteArrayAdapter & adapter) {
	m_priv = adapter.m_priv;
	m_offSet = adapter.m_offSet;
	m_len = adapter.m_len;
	m_getPtr = adapter.m_getPtr;
	m_putPtr = adapter.m_putPtr;
	return *this;
}

#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
void UByteArrayAdapter::disableRefCounting() {
	m_priv.disableRC();
}

void UByteArrayAdapter::enableRefCounting() {
	m_priv.enableRC();
}
#endif

void UByteArrayAdapter::swap(UByteArrayAdapter& other) {
	using std::swap;
	swap(m_priv, other.m_priv);
	swap(m_offSet, other.m_offSet);
	swap(m_len, other.m_len);
	swap(m_getPtr, other.m_getPtr);
	swap(m_putPtr, other.m_putPtr);
}


UByteArrayAdapter
UByteArrayAdapter::fromGetPtr() const {
	UByteArrayAdapter result(*this);
	result.shrinkToGetPtr();
	return result;
}

UByteArrayAdapter
UByteArrayAdapter::fromPutPtr() const {
	UByteArrayAdapter result(*this);
	result.shrinkToPutPtr();
	return result;
}

void UByteArrayAdapter::advice(UByteArrayAdapter::AdviseType type, UByteArrayAdapter::SizeType count) {
	if (count > m_len) {
		count = m_len;
	}
	m_priv->advice(type, m_offSet, count);
}

void UByteArrayAdapter::advice(AdviseType type) {
	advice(type, size());
}

void UByteArrayAdapter::sync() {
	m_priv->sync();
}

void UByteArrayAdapter::zero() {
	if (isContiguous()) {
		::memset(&operator[](0), 0, sizeof(uint8_t)*m_len);
	}
	else {
		uint32_t bufLen = 1024*1024;
		uint8_t * zeros = new uint8_t[bufLen];
		memset(zeros, 0, bufLen);
		for(OffsetType i = 0; i < m_len; i += bufLen) {
			putData(i, zeros, std::min<OffsetType>(m_len-i, bufLen));
		}
		delete[] zeros;
	}
}

void UByteArrayAdapter::fill(uint8_t value, SizeType begin, SizeType length) {
	if (begin > m_len) {
		return;
	}
	if (isContiguous()) {
		::memset(&operator[](begin), value, std::min<SizeType>(length, m_len-begin));
	}
	else {
		uint32_t bufLen = std::min<SizeType>(length, 4*1024*1024);
		uint8_t * values = new uint8_t[bufLen];
		::memset(values, value, bufLen);
		for(OffsetType i = begin; i < m_len; i += bufLen) {
			putData(i, values, std::min<OffsetType>(m_len-i, bufLen));
		}
		delete[] values;
	}
}

bool UByteArrayAdapter::equal(const UByteArrayAdapter& b) const {
	return (m_offSet == b.m_offSet && m_len == b.m_len && m_priv == b.m_priv && m_getPtr == b.m_getPtr && m_putPtr == b.m_putPtr);
}

bool UByteArrayAdapter::equalContent(const UByteArrayAdapter& b) const {
	if (size() != b.size())
		return false;
	for(OffsetType i = 0; i < size(); ++i) {
		if (at(i) != b.at(i))
			return false;
	}
	return true;
}

bool UByteArrayAdapter::equalContent(const std::deque< uint8_t >& b) const {
	if (size() != b.size())
		return false;
	for(OffsetType i = 0; i < size(); ++i) {
		if (at(i) != b.at(i))
			return false;
	}
	return true;
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
	if (ok) {
		m_len -= byte;
	}
	else {
		throw sserialize::AllocationException("Could not allocate " + std::to_string(byte) + " Bytes");
	}
	return ok;
}

bool UByteArrayAdapter::growStorage(OffsetType byte) {
	bool ok;
	if (m_offSet+m_len+byte < m_offSet+m_len) //check for wrapp-around, BUG:do this correctly 
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
		if (m_len < m_getPtr) {
			m_getPtr = m_len;
		}
		if (m_len < m_putPtr) {
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
	range_check(pos, 1);

	return (*m_priv)[m_offSet+pos];
}


const uint8_t & UByteArrayAdapter::operator[](const OffsetType pos) const {
	range_check(pos, 1);
	
	return (*m_priv)[m_offSet+pos];
}

uint8_t& UByteArrayAdapter::operator*() {
	return operator[](0);
}

const uint8_t& UByteArrayAdapter::operator*() const {
	return operator[](0);
}

uint8_t UByteArrayAdapter::at(OffsetType pos) const {
	range_check(pos, 1);

	return m_priv->getUint8(m_offSet+pos);
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

bool UByteArrayAdapter::isContiguous() const {
	return m_priv->isContiguous();
}

UByteArrayAdapter UByteArrayAdapter::begin() const {
	return *this;
}

UByteArrayAdapter UByteArrayAdapter::end() const {
	return UByteArrayAdapter(*this, m_len);
}

UByteArrayAdapter & UByteArrayAdapter::makeContigous(UByteArrayAdapter & d) {
	//this may be only relevant on ILP32 
// 	#ifndef __LP64__
	if(!d.m_priv->isContiguous()) {
		OffsetType pP = d.m_putPtr;
		OffsetType gP = d.m_getPtr;
		d = UByteArrayAdapter(d.asMemView());
		d.m_putPtr = pP;
		d.m_getPtr = gP;
	}
// 	#endif
	return d;
}

UByteArrayAdapter UByteArrayAdapter::makeContigous(const UByteArrayAdapter & d) {
	//this may be only relevant on ILP32 
// 	#ifndef __LP64__
	if(!d.m_priv->isContiguous()) {
		UByteArrayAdapter tmp(d.asMemView());
		tmp.m_putPtr = d.m_putPtr;
		tmp.m_getPtr = d.m_getPtr;
		return tmp;
	}
// 	#endif
	return d;
}

UByteArrayAdapter::MemoryView UByteArrayAdapter::getMemView(const OffsetType pos, OffsetType size) {
	range_check(pos, size);
	uint8_t * data = 0;
	bool isCopy = !m_priv->isContiguous();
	if (size) {
		if (isCopy) {
			data = new uint8_t[size];
			getData(pos, data, size);
		}
		else {
			data = &operator[](pos);
		}
	}
	return MemoryView(data, m_offSet+pos, size, isCopy, m_priv.get());
}

const UByteArrayAdapter::MemoryView UByteArrayAdapter::getMemView(const OffsetType pos, OffsetType size) const {
	return const_cast<UByteArrayAdapter*>(this)->getMemView(pos, size);
}

INLINE_WITH_LTO
int64_t UByteArrayAdapter::getInt64(const OffsetType pos) const {
	range_check(pos, 8);

	return m_priv->getInt64(m_offSet+pos);
}

INLINE_WITH_LTO
uint64_t UByteArrayAdapter::getUint64(const OffsetType pos) const {
	range_check(pos, 8);

	return m_priv->getUint64(m_offSet+pos);
}

INLINE_WITH_LTO
int32_t UByteArrayAdapter::getInt32(const OffsetType pos) const {
	range_check(pos, 4);

	return m_priv->getInt32(m_offSet+pos);
}

INLINE_WITH_LTO
uint32_t UByteArrayAdapter::getUint32(const OffsetType pos) const {
	range_check(pos, 4);

	return m_priv->getUint32(m_offSet+pos);
}

INLINE_WITH_LTO
uint32_t UByteArrayAdapter::getUint24(const OffsetType pos) const {
	range_check(pos, 3);

	return m_priv->getUint24(m_offSet+pos);
}

INLINE_WITH_LTO
uint16_t UByteArrayAdapter::getUint16(const OffsetType pos) const {
	range_check(pos, 2);
	
	return m_priv->getUint16(m_offSet+pos);
}

INLINE_WITH_LTO
uint8_t UByteArrayAdapter::getUint8(const OffsetType pos) const {
	range_check(pos, 1);

	return m_priv->getUint8(m_offSet+pos);
}

INLINE_WITH_LTO
double UByteArrayAdapter::getDouble(const OffsetType pos) const {
	range_check(pos, 8);

	return unpack_double_from_uint64_t(m_priv->getUint64(m_offSet+pos));
}

INLINE_WITH_LTO
float UByteArrayAdapter::getFloat(const OffsetType pos) const {
	range_check(pos, 4);

	return unpack_float_from_uint32_t(m_priv->getUint32(m_offSet+pos));
}

uint64_t UByteArrayAdapter::getVlPackedUint64(const OffsetType pos, int * length) const {
	range_check(pos, 1);

	int len = (int) std::min<SizeType>(10, m_len - pos);
	uint64_t res = m_priv->getVlPackedUint64(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		throw OutOfBoundsException();
	return res;
}

int64_t UByteArrayAdapter::getVlPackedInt64(const OffsetType pos, int * length) const {
	range_check(pos, 1);

	int len = (int) std::min<SizeType>(10, m_len - pos);
	int64_t res = m_priv->getVlPackedInt64(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		throw OutOfBoundsException();
	return res;
}

uint32_t UByteArrayAdapter::getVlPackedUint32(const OffsetType pos, int * length) const {
	range_check(pos, 1);

	int len = (int) std::min<SizeType>(5, m_len - pos);
	uint32_t res = m_priv->getVlPackedUint32(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		throw OutOfBoundsException();
	return res;
}

int32_t UByteArrayAdapter::getVlPackedInt32(const OffsetType pos, int * length) const {
	range_check(pos, 1);
	
	int len = (int) std::min<SizeType>(5, m_len - pos);
	uint32_t res = m_priv->getVlPackedInt32(m_offSet+pos, &len);
	if (length)
		*length = len;
	if (len < 0)
		throw OutOfBoundsException();
	return res;
}

INLINE_WITH_LTO
UByteArrayAdapter::OffsetType UByteArrayAdapter::getOffset(const OffsetType pos) const {
	range_check(pos, SSERIALIZE_OFFSET_BYTE_COUNT);

	return m_priv->getOffset(m_offSet+pos);
}

INLINE_WITH_LTO
UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset(const OffsetType pos) const {
	range_check(pos, SSERIALIZE_NEGATIVE_OFFSET_BYTE_COUNT);

	return m_priv->getNegativeOffset(m_offSet+pos);
}

std::string UByteArrayAdapter::getString(const OffsetType pos, int * length) const {
	int len = -1;
	uint32_t strLen = getVlPackedUint32(pos, &len);

	range_check(pos, len+strLen);

	if (length)
		*length = strLen+len;
	return m_priv->getString(m_offSet+pos+len, strLen);
}

UByteArrayAdapter UByteArrayAdapter::getStringData(const OffsetType pos, int * length) const {
	int len = -1;
	uint32_t strLen = getVlPackedUint32(pos, &len);

	range_check(pos, len+strLen);

	if (length)
		*length = strLen+len;
	//no need to adjust get pointer here, as in case of streaming, the get pointer is BEFORE pos
	return UByteArrayAdapter(*this, pos+len, strLen);
}

UByteArrayAdapter::OffsetType UByteArrayAdapter::getData(const UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const {
	range_check(pos, 0);

	if (pos+len > m_len)
		len = m_len - pos;
	m_priv->get(m_offSet+pos, dest, len);
	return len;
}

INLINE_WITH_LTO
void UByteArrayAdapter::putOffset(const OffsetType pos, const OffsetType value) {
	range_check(pos, SSERIALIZE_OFFSET_BYTE_COUNT);
	
	m_priv->putOffset(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putNegativeOffset(const OffsetType pos, const NegativeOffsetType value) {
	range_check(pos, SSERIALIZE_NEGATIVE_OFFSET_BYTE_COUNT);
	
	m_priv->putNegativeOffset(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putUint64(const OffsetType pos, const uint64_t value) {
	range_check(pos, 8);
	
	m_priv->putUint64(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putInt64(const OffsetType pos, const int64_t value) {
	range_check(pos, 8);
	
	m_priv->putInt64(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putInt32(const OffsetType pos, const int32_t value) {
	range_check(pos, 4);
	
	m_priv->putInt32(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putUint32(const OffsetType pos, const uint32_t value) {
	range_check(pos, 4);
	
	m_priv->putUint32(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putUint24(const OffsetType pos, const uint32_t value) {
	range_check(pos, 3);
	
	m_priv->putUint24(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putUint16(const OffsetType pos, const uint16_t value) {
	range_check(pos, 2);

	m_priv->putUint16(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putUint8(const OffsetType pos, const uint8_t value) {
	range_check(pos, 1);
	
	m_priv->putUint8(m_offSet+pos, value);
}

INLINE_WITH_LTO
void UByteArrayAdapter::putDouble(const OffsetType pos, const double value) {
	range_check(pos, 8);
	
	m_priv->putUint64(m_offSet+pos, pack_double_to_uint64_t(value));
}

INLINE_WITH_LTO
void UByteArrayAdapter::putFloat(const OffsetType pos, const float value) {
	range_check(pos, 4);
	
	m_priv->putUint32(m_offSet+pos, pack_float_to_uint32_t(value));
}

/** @return: Length of the number, -1 on failure **/

int UByteArrayAdapter::putVlPackedUint64(const OffsetType pos, const uint64_t value) {
	range_check(pos, 1);

	int len = m_priv->putVlPackedUint64(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putVlPackedInt64(const OffsetType pos, const int64_t value) {
	range_check(pos, 1);

	int len = m_priv->putVlPackedInt64(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putVlPackedUint32(const OffsetType pos, const uint32_t value) {
	range_check(pos, 1);
	
	int len = m_priv->putVlPackedUint32(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putVlPackedPad4Uint32(const OffsetType pos, const uint32_t value) {
	range_check(pos, 1);
	
	int len = m_priv->putVlPackedPad4Uint32(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putVlPackedInt32(const OffsetType pos, const int32_t value) {
	range_check(pos, 1);

	int len = m_priv->putVlPackedInt32(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putVlPackedPad4Int32(const OffsetType pos, const int32_t value) {
	range_check(pos, 1);

	int len = m_priv->putVlPackedPad4Int32(m_offSet+pos, value, m_len-pos);
	if (UNLIKELY_BRANCH(len < 0)) {
		throw OutOfBoundsException();
	}
	return len;
}

int UByteArrayAdapter::putString(const OffsetType pos, const std::string & str) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(str.size(), (std::size_t) std::numeric_limits<int>::max());
	UByteArrayAdapter::OffsetType needSize = psize_vu32((uint32_t) str.size()) + str.size();
	
	range_check(pos, needSize);
	
	
	int len = putVlPackedUint32(pos, (uint32_t) str.size());
	
	static_assert(sizeof(std::string::value_type) == sizeof(uint8_t));
	putData(pos+len, reinterpret_cast<const uint8_t*>(str.data()), str.size());
	return (int) needSize;
}

void UByteArrayAdapter::putData(OffsetType pos, const uint8_t * data, OffsetType len) {
	if (!len) {
		return;
	}
	
	range_check(pos, len);

	m_priv->put(m_offSet+pos, data, len);
}

void UByteArrayAdapter::putData(const OffsetType pos, const std::deque<uint8_t>& data) {
	if (!data.size()) {
		return;
	}
	
	range_check(pos, data.size());

	for(size_t i = 0; i < data.size(); i++) {
		putUint8(pos+i, data[i]);
	}
}

void UByteArrayAdapter::putData(const OffsetType pos, const std::vector< uint8_t >& data) {
	if (data.size()) {
		putData(pos, &data[0], data.size());
	}
}

void UByteArrayAdapter::putData(const OffsetType pos, const UByteArrayAdapter & data) {
	if (!data.size()) {
		return;
	}

	range_check(pos, data.size());
	
	if (m_priv->isContiguous()) {
		putData(pos, &data[0], data.size());
	}

	SizeType bufLen = std::min<OffsetType>(data.size(), 4*1024*1024);
	uint8_t * buf = new uint8_t[bufLen];
	for(SizeType i = 0, s = data.size(); i < s;) {
		OffsetType len = std::min<OffsetType>(s-i, bufLen);
		data.getData(i, buf, len);
		putData(pos+i, buf, len);
		i += len;
	}
	delete[] buf;
}

void UByteArrayAdapter::putData(const OffsetType pos, const MemoryView & data) {
	putData(pos, data.get(), data.size());
}

UByteArrayAdapter UByteArrayAdapter::writeToDisk(std::string fileName, bool deleteOnClose) {
	if (fileName.empty()) {
		fileName = MmappedFile::findLockFilePath(SSERIALIZE_PERSISTENT_CACHE_PATH, 2048);
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
	MyPrivatePtr priv(new UByteArrayAdapterPrivateMmappedFile(tempFile));
	priv->setDeleteOnClose(deleteOnClose);
	UByteArrayAdapter adap(priv);
	adap.m_len = m_len;
	adap.m_offSet = 0;

	//Now copy contents
	uint8_t* tempFileData = tempFile.data();
	m_priv->get(0, tempFileData, m_len);
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::createCache(UByteArrayAdapter::OffsetType size, sserialize::MmappedMemoryType mmt) {
	if (size == 0)
		size = 1;

	MyPrivatePtr priv;
	switch(mmt) {
	case sserialize::MM_SLOW_FILEBASED:
		{
			MmappedFile tempFile;
			if (!MmappedFile::createTempFile(getTempFilePrefix(), size, tempFile)) {
				throw sserialize::CreationException("UByteArrayAdapter::createCache: could not open file");
				return UByteArrayAdapter();
			}
			priv.reset( new UByteArrayAdapterPrivateMmappedFile(tempFile) );
		}
		break;
	case sserialize::MM_FAST_FILEBASED:
		{
			MmappedFile tempFile;
			if (!MmappedFile::createTempFile(getFastTempFilePrefix(), size, tempFile)) {
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
	if (! tempFile.open() ) {
		sserialize::err("UByteArrayAdapter::createFile", "Fatal: could not open file: " + fileName);
		return UByteArrayAdapter();
	}
	tempFile.setSyncOnClose(true);
	MyPrivatePtr priv( new UByteArrayAdapterPrivateMmappedFile(tempFile) );
	priv->setDeleteOnClose(false);
	UByteArrayAdapter adap(priv);
	adap.m_len = size;
	adap.m_offSet = 0;
	return adap;
}

UByteArrayAdapter UByteArrayAdapter::open(
	const std::string& fileName,
	OpenFlags flags,
	#ifndef SSERIALIZE_UBA_ONLY_CONTIGUOUS
	uint8_t chunkSizeExponent
	#else
	uint8_t
	#endif
	)
{
	if (flags & OpenFlags::Compressed()) {
		#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
		throw sserialize::UnsupportedFeatureException("File is compressed and sserialize was compiled with contiguous UByteArrayAdapter only.");
		#else
		CompressedMmappedFile file(fileName);
		if (file.open()) {
			return UByteArrayAdapter(file);
		}
		else {
			throw sserialize::IOException("Could not open file " + fileName);
		}
		#endif
	}
	else if (MmappedFile::fileSize(fileName) > SSERIALIZE_MAX_SIZE_FOR_FULL_MMAP || (flags & (OpenFlags::DirectIo() | OpenFlags::Chunked()))) {
		#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
		if ((flags | OpenFlags::DirectIo())) {
			throw sserialize::UnsupportedFeatureException("Requesting direct I/O but sserialize was compiled with contiguous UByteArrayAdapter only.");
		}
		else {
			throw sserialize::UnsupportedFeatureException("File is too large and sserialize was compiled with contiguous UByteArrayAdapter only.");
		}
		#else
		if (chunkSizeExponent == 0 || (flags & OpenFlags::DirectIo())) {
			return UByteArrayAdapter( MyPrivatePtr(
				new UByteArrayAdapterPrivateThreadSafeFile(fileName, (flags & OpenFlags::Writable()), (flags & OpenFlags::DirectIo()))
			));
		}
		else {
			ChunkedMmappedFile file(fileName, chunkSizeExponent, (flags & OpenFlags::Writable()));
			if (file.open()) {
				return UByteArrayAdapter(file);
			}
			else {
				throw sserialize::IOException("Could not open file " + fileName);
			}
		}
		#endif
	}
	else {
		MmappedFile file(fileName, (flags & OpenFlags::Writable()));
		if (file.open()) {
			return UByteArrayAdapter(file);
		}
		else {
			throw sserialize::IOException("Could not open file " + fileName);
		}
	}
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
	m_getPtr += SSERIALIZE_OFFSET_BYTE_COUNT;
	return res;
}

UByteArrayAdapter::NegativeOffsetType UByteArrayAdapter::getNegativeOffset() {
	NegativeOffsetType res = getNegativeOffset(m_getPtr);
	m_getPtr += SSERIALIZE_NEGATIVE_OFFSET_BYTE_COUNT;
	return res;
}
#define UBA_STREAMING_GET_FUNC(__NAME, __TYPE, __LENGTH) \
INLINE_WITH_LTO __TYPE UByteArrayAdapter::__NAME() { \
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
	range_check(m_getPtr, 1);

	int len = (int) std::min<OffsetType>(5, m_len - m_getPtr);
	uint32_t res = m_priv->getVlPackedUint32(m_offSet+m_getPtr, &len);
	if (len < 0)
		throw OutOfBoundsException();
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

UByteArrayAdapter::OffsetType UByteArrayAdapter::getData(uint8_t * dest, UByteArrayAdapter::OffsetType len) {
	len = getData(m_getPtr, dest, len);
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

INLINE_WITH_LTO
bool UByteArrayAdapter::resizeForPush(OffsetType pos, OffsetType length) {
	if ( UNLIKELY_BRANCH( pos+length > m_len) ) {
		return growStorage(pos+length-m_len);
	}
	return true;
}

INLINE_WITH_LTO
void UByteArrayAdapter::range_check(OffsetType pos, OffsetType length) const {
	if ( UNLIKELY_BRANCH(m_len < pos+length) ) {
		throw OutOfBoundsException(pos, length, m_len);
	}
}

#define UBA_PUT_STREAMING_FUNC(__NAME, __TYPE, __LENGTH) \
void UByteArrayAdapter::__NAME(const __TYPE value) { \
	if (!resizeForPush(m_putPtr, __LENGTH)) { \
		throw IOException("Resizing by " #__LENGTH " bytes failed"); \
	} \
	__NAME(m_putPtr, value); \
	m_putPtr += __LENGTH; \
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
	int len = __SERFUNC(value, tmp, tmp+__BUFSIZE); \
	if (len < 0) \
		return -1; \
	putData(tmp, len); \
	return len; \
} \

UBA_PUT_VL_STREAMING_FUNC(putVlPackedUint64, 10, p_vu64, uint64_t);
UBA_PUT_VL_STREAMING_FUNC(putVlPackedInt64, 10, p_vs64, int64_t);
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
	if (!resizeForPush(m_putPtr, 4)) {
		throw IOException("Resizing by 4 bytes failed");
	}

	int len = putVlPackedInt32(m_putPtr, value);

	if (len > 0)
		m_putPtr += len;
	return len;
}

void UByteArrayAdapter::putString(const std::string& str) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(str.size(), (std::size_t) std::numeric_limits<uint32_t>::max());
	UByteArrayAdapter::OffsetType needSize = psize_vu32((uint32_t) str.size()) + str.size();
	if (!resizeForPush(m_putPtr, needSize)) {
		throw IOException("Resizing by " + std::to_string(needSize) + " failed");
	}
	int pushedBytes = putString(m_putPtr, str);
	if (pushedBytes >= 0) {
		m_putPtr += needSize;
	}
}

void UByteArrayAdapter::putData(const uint8_t * data, UByteArrayAdapter::OffsetType len) {
	UByteArrayAdapter::OffsetType needSize = len;
	if (!resizeForPush(m_putPtr, needSize)) {
		throw IOException("Resizing by " + std::to_string(needSize) + " failed");
	}
	putData(m_putPtr, data, len);
	m_putPtr += needSize;
}

void UByteArrayAdapter::putData(const std::deque< uint8_t >& data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize)) {
		throw IOException("Resizing by " + std::to_string(needSize) + " failed");
	}
	putData(m_putPtr, data);
	m_putPtr += needSize;
}

void UByteArrayAdapter::putData(const std::vector< uint8_t >& data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize)) {
		throw IOException("Resizing by " + std::to_string(needSize) + " failed");
	}
	putData(m_putPtr, data);
	m_putPtr += needSize;
}

void UByteArrayAdapter::putData(const UByteArrayAdapter & data) {
	UByteArrayAdapter::OffsetType needSize = data.size();
	if (!resizeForPush(m_putPtr, needSize)) {
		throw IOException("Resizing by " + std::to_string(needSize) + " failed");
	}
	putData(m_putPtr, data);
	m_putPtr += needSize;
}

std::string UByteArrayAdapter::toString() const {
	std::string str;
	str.resize(size());
	for(OffsetType i(0), s(size()); i < s; ++i) {
		str[i] = at(i);
	}
	return str;
}

void UByteArrayAdapter::dump(OffsetType byteCount) const {
	OffsetType dumpLen = byteCount;
	if (m_len < dumpLen)
		dumpLen = m_len;
	for(OffsetType i = 0; i < dumpLen; i++) {
		std::cout << static_cast<uint32_t>(at(i)) << ":";
	}
	std::cout << std::endl;
}

void UByteArrayAdapter::dumpAsString(OffsetType byteCount) const {
	OffsetType dumpLen = byteCount;
	if (m_len < dumpLen)
		dumpLen = m_len;
	for(OffsetType i = 0; i < dumpLen; i++) {
		std::cout << at(i) << ":";
	}
	std::cout << std::endl;
}

#define STATIC_PUT_FUNCS_MAKRO(__NAME, __TYPE) \
INLINE_WITH_LTO void UByteArrayAdapter::__NAME(UByteArrayAdapter & dest, __TYPE src) { dest.__NAME(src); }
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

bool operator==(const sserialize::UByteArrayAdapter& a, const sserialize::UByteArrayAdapter& b) {
	return a.equal(b);
}

bool operator!=(const sserialize::UByteArrayAdapter & a, const sserialize::UByteArrayAdapter & b) {
	return ! a.equal(b);
}

//Streaming operators

#define UBA_OPERATOR_PUT_STREAMING_FUNC(__NAME, __TYPE) \
INLINE_WITH_LTO UByteArrayAdapter& operator<<(UByteArrayAdapter & data, const __TYPE value) { \
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
UBA_OPERATOR_PUT_STREAMING_FUNC(put, sserialize::UByteArrayAdapter &);

#undef UBA_OPERATOR_PUT_STREAMING_FUNC


#define UBA_OPERATOR_GET_STREAMING_FUNC(__NAME, __TYPE) \
INLINE_WITH_LTO sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & data, __TYPE & value) { \
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

INLINE_WITH_LTO OffsetType SerializationInfo<std::string>::sizeInBytes(const std::string & value) {
	return psize_vu32((uint32_t) value.size()) + value.size();

}

}//end namespace
