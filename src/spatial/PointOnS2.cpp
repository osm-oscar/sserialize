#include <sserialize/spatial/PointOnS2.h>
#include <sserialize/utility/checks.h>
#include <gmp.h>


namespace sserialize {
namespace spatial {
namespace ratss {

PointOnS2::PointOnS2() :
m_xnum(0),
m_ynum(0),
m_znum(0),
m_denom(1)
{}

PointOnS2::PointOnS2(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom) :
m_xnum(xnum),
m_ynum(ynum),
m_znum(znum),
m_denom(denom)
{}


static mpz_class lcm(const mpz_class & a, const mpz_class & b) {
	mpz_t tmp;
	::mpz_init(tmp);
	mpz_lcm(tmp, a.get_mpz_t(), b.get_mpz_t());
	mpz_class ret(tmp);
	::mpz_clear(tmp);
	return ret;
}

PointOnS2::PointOnS2(const mpq_class & x, const mpq_class & y, const mpq_class & z) {
	mpz_class myLcm = lcm(lcm(x.get_den(), y.get_den()), z.get_den());

	if (!myLcm.fits_ulong_p()) {
		throw std::overflow_error("sserialize::spatial::ratss::RationalPoint3: denominators are too large");
	}
	mpz_class xnum = x.get_num() * (myLcm / x.get_den());
	mpz_class ynum = y.get_num() * (myLcm / y.get_den());
	mpz_class znum = z.get_num() * (myLcm / z.get_den());
	
	if(!xnum.fits_ulong_p() || !ynum.fits_ulong_p() || !znum.fits_ulong_p()) {
		throw std::overflow_error("sserialize::spatial::ratss::RationalPoint3: numerators are too large");
	}
	m_xnum = xnum.get_ui();
	m_ynum = ynum.get_ui();
	m_znum = znum.get_ui();
	m_denom = myLcm.get_ui();
}

PointOnS2::~PointOnS2() {}

int64_t & PointOnS2::xnum() {
	return m_xnum;
}

const int64_t & PointOnS2::xnum() const {
	return m_xnum;
}

int64_t & PointOnS2::ynum() {
	return m_ynum;
}

const int64_t & PointOnS2::ynum() const {
	return m_ynum;
}

int64_t & PointOnS2::znum() {
	return m_znum;
}

const int64_t & PointOnS2::znum() const {
	return m_znum;
}

uint64_t & PointOnS2::denom() {
	return m_denom;
}

const uint64_t & PointOnS2::denom() const {
	return m_denom;
}

Fraction PointOnS2::x() const {
	return Fraction(xnum(), denom());
}

Fraction PointOnS2::y() const {
	return Fraction(ynum(), denom());
}

Fraction PointOnS2::z() const {
	return Fraction(znum(), denom());
}

}}}//end namespace sserialize