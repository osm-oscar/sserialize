#include <sserialize/utility/Fraction.h>
#include <sserialize/utility/exceptions.h>
#include <stdexcept>

#ifndef __LP64__
#include <sstream>
#endif

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
		if (!fits_int64(f.get_num())|| !fits_uint64(f.get_den())) {
			throw std::overflow_error("Fraction does not support values that do not fit into an (u)int64_t");
		}
		m_num = get_int64(f.get_num());
		m_denom = get_uint64(f.get_den());
	}

	Fraction::operator mpq_class() const {
		return toMpq();
	}
	
	mpq_class Fraction::toMpq() const {
		return mpq_class(to_mpz(m_num), to_mpz(m_denom));
	}
	
	bool Fraction::fits_int64(const mpz_class & z) {
		#ifdef __LP64__
			return z.fits_slong_p();
		#else
			return ::mpz_sizeinbase(z.get_mpz_t(), 2) <= 63;
		#endif
	}
	
	bool Fraction::fits_uint64(const mpz_class & z) {
		#ifdef __LP64__
			return z.fits_slong_p();
		#else
			return ::mpz_sizeinbase(z.get_mpz_t(), 2) <= 64;
		#endif
	}
	

	int64_t Fraction::get_int64(const mpz_class & z) {
		#ifdef __LP64__
			return z.get_si();
		#else
			if (z < 0) {
				return - int64_t(get_uint64(-z));
			}
			else {
				return int64_t(get_uint64(z));
			}
		#endif
	}
	
	uint64_t Fraction::get_uint64(const mpz_class & z) {
		#ifdef __LP64__
			return z.get_ui();
		#else
			return (uint64_t( mpz_class((z >> 32) & 0xFFFFFFFF).get_ui()) << 32) | uint64_t( mpz_class(z & 0xFFFFFFFF).get_ui() );
		#endif
	}
	
	mpz_class Fraction::to_mpz(int64_t v) {
		#ifdef __LP64__
			return mpz_class(v);
		#else
			if (v < 0) {
				return -to_mpz(uint64_t(-v));
			}
			else {
				return to_mpz(uint64_t(v));
			}
		#endif
	}
	
	mpz_class Fraction::to_mpz(uint64_t v) {
		#ifdef __LP64__
			return mpz_class(v);
		#else
			return (mpz_class(uint32_t(v >> 32)) << 32) + uint32_t(v);
		#endif
	}
	
#endif

}//end namespace sserialize