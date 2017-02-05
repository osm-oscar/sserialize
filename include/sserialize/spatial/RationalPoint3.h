#ifndef SSERIALIZE_SPATIAL_RATIONAL_POINT3_H
#define SSERIALIZE_SPATIAL_RATIONAL_POINT3_H
#include <sserialize/utility/Fraction.h>

/** This is a special class to encode Point in 3D with rational coordinates using libratss
  * In libratss all coordinates of a point have the same denominator 
  *
  */

namespace sserialize {
namespace spatial {
namespace ratss {

class RationalPoint3 {
public:
	RationalPoint3();
	RationalPoint3(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom);
	~RationalPoint3();
public:
	int64_t & xnum();
	const int64_t & xnum() const;
	int64_t & ynum();
	const int64_t & ynum() const;
	int64_t & znum();
	const int64_t & znum() const;
	uint64_t & denom();
	const uint64_t & denom() const;
public:
	Fraction x() const;
	Fraction y() const;
	Fraction z() const;
private:
	int64_t m_xnum;
	int64_t m_ynum;
	int64_t m_znum;
	uint64_t m_denom;
};

}}}//end namespace

#endif