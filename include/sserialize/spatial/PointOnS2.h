#ifndef SSERIALIZE_SPATIAL_POINT_ON_S2_H
#define SSERIALIZE_SPATIAL_POINT_ON_S2_H
#include <sserialize/utility/Fraction.h>
#include <sserialize/spatial/GeoPoint.h>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

/** This is a special class to encode Point in 3D with rational coordinates using libratss
  * In libratss all coordinates of a point have the same denominator
  * This has a precision of about 1 cm on earth
  *
  */

namespace sserialize {
namespace spatial {
namespace ratss {

class PointOnS2 {
public:
	PointOnS2();
	explicit PointOnS2(const sserialize::spatial::GeoPoint & gp);
	explicit PointOnS2(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom);
	explicit PointOnS2(const mpq_class & x, const mpq_class & y, const mpq_class & z);
	~PointOnS2();
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
public:
	explicit operator sserialize::spatial::GeoPoint() const;
public:
	explicit operator CGAL::Exact_predicates_exact_constructions_kernel::Point_3() const;
private:
	void init(const mpq_class & x, const mpq_class & y, const mpq_class & z);
private:
	int64_t m_xnum;
	int64_t m_ynum;
	int64_t m_znum;
	uint64_t m_denom;
};

}}}//end namespace

#endif