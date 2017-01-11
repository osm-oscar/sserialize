#ifndef SSERIALIZE_STATIC_FRACTION_H
#define SSERIALIZE_STATIC_FRACTION_H
#include <sserialize/storage/UByteArrayAdapter.h>

#define SSERIALIZE_WITH_GMPXX
#define SSERIALIZE_WITH_CGAL

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
namespace Static {

/** This is a small wrapper class to represent fractions with up to 64 bits in the numerator/denominator
  */

class Fraction {
public:
	Fraction();
	Fraction(uint64_t numerator, uint64_t denominator);
	Fraction(const sserialize::UByteArrayAdapter & src);
	~Fraction();
	uint64_t numerator() const;
	uint64_t denominator() const;
public:
#if defined(SSERIALIZE_WITH_GMPXX)
	explicit Fraction(const mpq_class & f);
	operator mpq_class() const;
#endif
public:
#if defined(SSERIALIZE_WITH_CGAL)
	explicit Fraction(const mpq_class & f);
	operator mpq_class() const;
#endif
private:
	uint64_t m_num;
	uint64_t m_denom;
};


sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Fraction & src);
sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & dest, Fraction & src);


}}//end namespace sserialize::Static

#endif