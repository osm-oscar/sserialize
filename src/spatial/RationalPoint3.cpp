#include <sserialize/spatial/RationalPoint3.h>

namespace sserialize {
namespace spatial {
namespace ratss {

RationalPoint3::RationalPoint3() :
m_xnum(0),
m_ynum(0),
m_znum(0),
m_denom(1)
{}

RationalPoint3::RationalPoint3(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom) :
m_xnum(xnum),
m_ynum(ynum),
m_znum(znum),
m_denom(denom)
{}

RationalPoint3::~RationalPoint3() {}

int64_t & RationalPoint3::xnum() {
	return m_xnum;
}

const int64_t & RationalPoint3::xnum() const {
	return m_xnum;
}

int64_t & RationalPoint3::ynum() {
	return m_ynum;
}

const int64_t & RationalPoint3::ynum() const {
	return m_ynum;
}

int64_t & RationalPoint3::znum() {
	return m_znum;
}

const int64_t & RationalPoint3::znum() const {
	return m_znum;
}

uint64_t & RationalPoint3::denom() {
	return m_denom;
}

const uint64_t & RationalPoint3::denom() const {
	return m_denom;
}

Fraction RationalPoint3::x() const {
	return Fraction(xnum(), denom());
}

Fraction RationalPoint3::y() const {
	return Fraction(ynum(), denom());
}

Fraction RationalPoint3::z() const {
	return Fraction(znum(), denom());
}

}}}//end namespace sserialize