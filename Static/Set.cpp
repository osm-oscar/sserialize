#include "Set.h"

namespace sserialize {
namespace Static {


template<>
Set<int32_t>::Set(const UByteArrayAdapter & data)
{
	m_data = UByteArrayAdapter(data, 5, data.getUint32(1));
}

template<>
Set<uint32_t>::Set(const UByteArrayAdapter & data)
{
	m_data = UByteArrayAdapter(data, 5, data.getUint32(1));
}

template<>
Set<uint16_t>::Set(const UByteArrayAdapter & data)
{
	m_data = UByteArrayAdapter(data, 5, data.getUint32(1));
}

template<>
Set<uint8_t>::Set(const UByteArrayAdapter & data)
{
	m_data = UByteArrayAdapter(data, 5, data.getUint32(1));
}


template<>
uint32_t
Set<int32_t>::size() const {
	return m_data.size()/4;
}

template<>
uint32_t
Set<uint32_t>::size() const {
	return m_data.size()/4;
}

template<>
uint32_t
Set<uint16_t>::size() const {
	return m_data.size()/2;
}

template<>
uint32_t
Set<uint8_t>::size() const {
	return m_data.size();
}

template<>
UByteArrayAdapter::OffsetType
Set<int32_t>::getSizeInBytes() const {
	return 5+m_data.size();
}

template<>
UByteArrayAdapter::OffsetType
Set<uint32_t>::getSizeInBytes() const {
	return 5+m_data.size();
}

template<>
UByteArrayAdapter::OffsetType
Set<uint16_t>::getSizeInBytes() const {
	return 5+m_data.size();
}

template<>
UByteArrayAdapter::OffsetType
Set<uint8_t>::getSizeInBytes() const {
	return 5+m_data.size();
}

template<>
int32_t
Set<int32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getInt32(4*pos);
}

template<>
uint32_t
Set<uint32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint32(4*pos);
}


template<>
uint16_t
Set<uint16_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint16(2*pos);
}

template<>
uint8_t
Set<uint8_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint8(4*pos);
}

template<>
std::string
Set<std::string>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return std::string();
	}
	return m_data.getString(m_index.at(pos));
}

}}//end namespace

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<int32_t> & source) {
	return streamNumberSetInUByteArrayAdapter<int32_t>(destination, source);
}

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint32_t> & source) {
	return streamNumberSetInUByteArrayAdapter<uint32_t>(destination, source);
}

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint16_t> & source) {
	return streamNumberSetInUByteArrayAdapter<uint16_t>(destination, source);
}

sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::set<uint8_t> & source) {
	return streamNumberSetInUByteArrayAdapter<uint8_t>(destination, source);
}