#include <sserialize/Static/DenseGeoPointVector.h>

namespace sserialize {
namespace Static {
namespace spatial {



DenseGeoPointVector::ForwardIterator::ForwardIterator() :
m_pos(0),
m_size(0),
m_intLat(0),
m_intLon(0)
{}

DenseGeoPointVector::ForwardIterator::ForwardIterator(const UByteArrayAdapter & d, uint32_t size, bool end) :
m_d(d),
m_pos((end ? size : 0)),
m_size(size),
m_intLat(0),
m_intLon(0)
{
	m_d.resetPtrs();
	if (!end && size) {
		m_intLat = m_d.getVlPackedUint32();
		m_intLon = m_d.getVlPackedUint32();
	}
}

DenseGeoPointVector::ForwardIterator & DenseGeoPointVector::ForwardIterator::operator++() {
	if (m_pos < m_size) {
		int32_t intLatDiff = m_d.getVlPackedInt32();
		int32_t intLonDiff = m_d.getVlPackedInt32();
		
		if (intLatDiff < 0) {
			m_intLat = m_intLat - (static_cast<uint32_t>(-intLatDiff));
		}
		else {
			m_intLat = m_intLat + intLatDiff;
		}

		if (intLonDiff < 0) {
			m_intLon = m_intLon - (static_cast<uint32_t>(-intLonDiff));
		}
		else {
			m_intLon = m_intLon + intLonDiff;
		}
		++m_pos;
	}
	return *this;
}

DenseGeoPointVector::ForwardIterator DenseGeoPointVector::ForwardIterator::operator++(int num) {
	ForwardIterator fw(*this);
	if (num > 0) {
		if (m_pos+num > m_size)
			num = m_size-m_pos;
		while (num) {
			operator++();
			--num;
		}
	}
	return fw;
}

DenseGeoPointVector::ForwardIterator DenseGeoPointVector::ForwardIterator::operator+(uint32_t diff) const {
	DenseGeoPointVector::ForwardIterator t(*this);
	t.operator++(diff);
	return t;
}

}}}//end namespace
