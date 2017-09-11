#ifndef SSERIALIZE_FRACTION_H
#define SSERIALIZE_FRACTION_H


#define SSERIALIZE_WITH_GMPXX
// #define SSERIALIZE_WITH_CGAL

#if defined(SSERIALIZE_WITH_GMPXX)
	#include <gmpxx.h>
#endif

#if defined(SSERIALIZE_WITH_CGAL)
	#include <CGAL/Gmpq.h>
	#include <CGAL/Lazy_exact_nt.h>
	#include <CGAL/CORE/BigRat.h>
	#include <CGAL/CORE/Expr.h>
#endif


namespace sserialize {

/** This is a small wrapper class to represent fractions with up to 64 bits in the numerator/denominator
  *
  */

class Fraction {
public:
	Fraction();
	Fraction(int64_t numerator, uint64_t denominator);
	virtual ~Fraction();
public:
	const int64_t & numerator() const;
	const uint64_t & denominator() const;
	int64_t & numerator();
	uint64_t & denominator();
public:
#if defined(SSERIALIZE_WITH_GMPXX)
	explicit Fraction(const mpq_class & f);
	operator mpq_class() const;
	mpq_class toMpq() const;
	static bool fits_int64(const mpz_class & z);
	static bool fits_uint64(const mpz_class & z);
	static int64_t get_int64(const mpz_class & z);
	static uint64_t get_uint64(const mpz_class & z);
	static mpz_class to_mpz(int64_t v);
	static mpz_class to_mpz(uint64_t v);
#endif
public:
#if defined(SSERIALIZE_WITH_CGAL)
	explicit Fraction(const mpq_class & f);
	operator mpq_class() const;
#endif
private:
	int64_t m_num;
	uint64_t m_denom;
};

}//end namespace sserialize
#endif