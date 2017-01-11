#include <sserialize/Static/Fraction.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {

Fraction::Fraction() :
m_num(0),
m_denom(1)
{}

Fraction::Fraction(uint64_t numerator, uint64_t denominator) :
m_num(numerator),
m_denom(denominator)
{
	if (denominator == 0) {
		throw sserialize::MathException("Fraction: denominator is not allowed to be zero.");
	}
}

Fraction::Fraction(const UByteArrayAdapter& src) {
	int len = 0;
	m_num = src.getVlPackedUint64(0, &len);
	if (len < 0) {
		throw sserialize::CorruptDataException("Fraction: Could not parse numerator from data stream");
	}
	m_denom = src.getVlPackedUint64((UByteArrayAdapter::SizeType) len, &len);
	if (len < 0) {
		throw sserialize::CorruptDataException("Fraction: Could not parse denominator from data stream");
	}
}

Fraction::~Fraction() {}

uint64_t Fraction::numerator() const {
	return m_num;
}

uint64_t Fraction::denominator() const {
	return m_denom;
}

#if defined(SSERIALIZE_WITH_GMPXX)
	Fraction::Fraction(const mpq_class& f) {
		if (!f.get_num().fits_ulong_p() || !f.get_den().fits_ulong_p()) {
			throw sserialize::TypeOverflowException("Fraction does not support values that do not fit into an uint64_t");
		}
		m_num = f.get_num();
		m_denom = f.get_den();
	}

	Fraction::operator mpq_class() const {
		return mpq_class(m_num, m_denom);
	}
#endif


}} //end namespace sserialize::Static