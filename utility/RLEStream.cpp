#include <sserialize/utility/RLEStream.h>

namespace sserialize {

RLEStream::Creator::Creator(UByteArrayAdapter& dest) :
m_dest(&dest),
m_prevId(0),
m_rleDiff(0),
m_rleCount(0)
{}

RLEStream::Creator::~Creator() {
	flush();
}

void RLEStream::Creator::put(uint32_t value) {
	int32_t myDiff = (int64_t)value - (int64_t)m_prevId;
	if (myDiff == m_rleDiff) {
		++m_rleCount;
	}
	else {
		flush();
		m_rleCount = 1;
		m_rleDiff = myDiff;
	}
	m_prevId = value;
}

void RLEStream::Creator::checkpoint(uint32_t value) {
	flush();
	m_prevId = value;
	m_dest->putVlPackedUint64((static_cast<uint64_t>(m_prevId) << 2) | 0x2);
}

void RLEStream::Creator::flush() {
	if (m_rleCount == 1) {
		if (m_prevId > (uint32_t)std::abs(m_rleDiff)) {
			if (m_rleDiff < 0) {
				m_dest->putVlPackedUint64( (static_cast<uint64_t>(-m_rleDiff) << 2) | 0x1);
			}
			else {
				m_dest->putVlPackedUint64( (static_cast<uint64_t>(m_rleDiff) << 2) | 0x0);
			}
		}
		else { //full id
			m_dest->putVlPackedUint64((static_cast<uint64_t>(m_prevId) << 2) | 0x2);
		}
	}
	else if (m_rleCount > 1) {
		m_dest->putVlPackedUint64((m_rleCount << 2) | 0x3);
		m_dest->putVlPackedInt32(m_rleDiff);
	}
	m_rleCount = 0;
	m_rleDiff = 0;
}

RLEStream::RLEStream(const UByteArrayAdapter& begin) :
m_d(begin),
m_curRleCount(0),
m_curRleDiff(0),
m_curId(0)
{
	operator++();
}

RLEStream::RLEStream() :
m_curRleCount(0),
m_curRleDiff(0),
m_curId(0)
{}

RLEStream::RLEStream(const RLEStream& other) :
m_d(other.m_d),
m_curRleCount(other.m_curRleCount),
m_curRleDiff(other.m_curRleDiff),
m_curId(other.m_curId)
{}

RLEStream::RLEStream(RLEStream&& other) :
m_d(std::forward<sserialize::UByteArrayAdapter>(other.m_d)),
m_curRleCount(other.m_curRleCount),
m_curRleDiff(other.m_curRleDiff),
m_curId(other.m_curId)
{}

RLEStream& RLEStream::operator=(const RLEStream& other) {
	m_d = other.m_d;
	m_curRleCount = other.m_curRleCount;
	m_curRleDiff = other.m_curRleDiff;
	m_curId = other.m_curId;
	return *this;
}

RLEStream& RLEStream::operator=(RLEStream&& other) {
	m_d = std::move(other.m_d);
	m_curRleCount = other.m_curRleCount;
	m_curRleDiff = other.m_curRleDiff;
	m_curId = other.m_curId;
	return *this;
}

RLEStream & RLEStream::operator++() {
	if (!m_curRleCount) {
		uint64_t curWord = m_d.getVlPackedUint64();
		uint8_t tmp = curWord & 0x3;
		curWord >>= 2;
		switch (tmp) {
		case 0x0://no rle, positive delta
			m_curId += curWord;
			return *this;
		case 0x1://no rle negative delta
			m_curId -= curWord; 
			return *this;
		case 0x2://no rle, no delta
			m_curId = curWord;
			return *this;
		case 0x3://rle
			m_curRleCount = curWord;
			m_curRleDiff = m_d.getVlPackedInt32();
			break;
		}
	}
	--m_curRleCount;
	//is the following necessary? whats the result of UINT32_MAX-INT32_MAX
	if (m_curRleDiff < 0) {
		m_curId -= (uint32_t)(-m_curRleDiff);
	}
	else {
		m_curId += m_curRleDiff;
	}
	return *this;
}

}//end namespace