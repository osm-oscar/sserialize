#include <sserialize/Static/Array.h>

template<>
int32_t
sserialize::Static::Array<int32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getInt32(m_index.at(pos));
}

template<>
uint32_t
sserialize::Static::Array<uint32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint32(m_index.at(pos));
};


template<>
uint16_t
sserialize::Static::Array<uint16_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint16(m_index.at(pos));
}

template<>
uint8_t
sserialize::Static::Array<uint8_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint8(m_index.at(pos));
}

template<>
std::string
sserialize::Static::Array<std::string>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return std::string();
	}
	return m_data.getString(m_index.at(pos));
}

template<>
sserialize::UByteArrayAdapter
sserialize::Static::Array<sserialize::UByteArrayAdapter>::at(uint32_t pos) const {
	return dataAt(pos);
}