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

void RLEStream::Creator::flush() {
	if (m_rleCount == 1) {
		if (m_rleDiff < 0) {
			m_dest->putVlPackedUint64( (static_cast<uint64_t>(-m_rleDiff) << 2) | 0x2);
		}
		else {
			m_dest->putVlPackedUint64( (static_cast<uint64_t>(m_rleDiff) << 2) | 0x0);
		}
	}
	else if (m_rleCount > 1) {
		m_dest->putVlPackedUint64((m_rleCount << 1) | 0x1);
		m_dest->putVlPackedInt32(m_rleDiff);
		m_rleCount = 0;
		m_rleDiff = 0;
	}
}

RLEStream::RLEStream(const UByteArrayAdapter& begin) :
m_d(begin),
m_curRleCount(0),
m_curRleDiff(0),
m_curId(0)
{
	operator++();
}

RLEStream & RLEStream::operator++() {
	if (!m_curRleCount) {
		uint64_t curWord = m_d.getVlPackedUint64();
		if (curWord & 0x1) { //found rle, next encodes the difference as signed integer
			m_curRleCount = curWord >> 1;
			m_curRleDiff = m_d.getVlPackedInt32();
		}
		else {//default delta
			//remaining data represents a signed delta
			curWord >>= 1;
			if (curWord & 0x1) { //negative delta
				m_curId -= (curWord >> 1);
			}
			else {
				m_curId += (curWord >> 1);
			}
			return *this;
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