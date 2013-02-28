#include "Deque.h"

template<>
int32_t
sserialize::Static::Deque<int32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getInt32(m_index.at(pos));
}

template<>
uint32_t
sserialize::Static::Deque<uint32_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint32(m_index.at(pos));
};


template<>
uint16_t
sserialize::Static::Deque<uint16_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint16(m_index.at(pos));
}

template<>
uint8_t
sserialize::Static::Deque<uint8_t>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	return m_data.getUint8(m_index.at(pos));
}

template<>
std::string
sserialize::Static::Deque<std::string>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return std::string();
	}
	return m_data.getString(m_index.at(pos));
}