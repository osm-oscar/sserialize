#include <sserialize/utility/Fraction.h>
#include <sserialize/utility/exceptions.h>
#include <stdexcept>

namespace sserialize {


Fraction::Fraction() :
m_num(0),
m_denom(1)
{}

Fraction::Fraction(int64_t numerator, uint64_t denominator) :
m_num(numerator),
m_denom(denominator)
{
	if (denominator == 0) {
		throw sserialize::MathException("Fraction: denominator is not allowed to be zero.");
	}
}

Fraction::~Fraction() {}

const int64_t & Fraction::numerator() const {
	return m_num;
}

const uint64_t & Fraction::denominator() const {
	return m_denom;
}

int64_t & Fraction::numerator() {
	return m_num;
}

uint64_t & Fraction::denominator() {
	return m_denom;
}

#if defined(SSERIALIZE_WITH_GMPXX)
	Fraction::Fraction(const mpq_class& f) {
		if (!f.get_num().fits_slong_p() || !f.get_den().fits_ulong_p()) {
			throw std::overflow_error("Fraction does not support values that do not fit into an (u)int64_t");
		}
		m_num = f.get_num().get_si();
		m_denom = f.get_den().get_ui();
	}

	Fraction::operator mpq_class() const {
		return mpq_class(m_num, m_denom);
	}
#endif

}//end namespace sserialize