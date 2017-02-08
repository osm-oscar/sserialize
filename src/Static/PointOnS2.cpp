#include <sserialize/Static/PointOnS2.h>

namespace sserialize {
namespace Static {
namespace spatial {
namespace ratss {

PointOnS2::PointOnS2() {}

PointOnS2::PointOnS2(const sserialize::spatial::GeoPoint & gp) :
sserialize::spatial::ratss::PointOnS2(gp)
{}

PointOnS2::PointOnS2(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom) :
sserialize::spatial::ratss::PointOnS2(xnum, ynum, znum, denom)
{}

PointOnS2::PointOnS2(sserialize::UByteArrayAdapter src)
{
	src.resetGetPtr();
	denom() = src.getUint64();
	xnum() = src.getInt64();
	ynum() = src.getInt64();
	znum() = src.getInt64();
}

PointOnS2::~PointOnS2() {}

}}}}//end namespace