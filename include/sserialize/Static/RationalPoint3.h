#ifndef SSERIALIZE_STATIC_SPATIAL_RATIONAL_POINT3_H
#define SSERIALIZE_STATIC_SPATIAL_RATIONAL_POINT3_H
#include <sserialize/spatial/RationalPoint3.h>
#include <sserialize/storage/UByteArrayAdapter.h>

/** This is a special class to encode Point in 3D with rational coordinates using libratss
  * In libratss all coordinates of a point have the same denominator 
  *
  */

namespace sserialize {
namespace Static {
namespace spatial {
namespace ratss {

class RationalPoint3: public sserialize::spatial::ratss::RationalPoint3 {
public:
	typedef sserialize::spatial::ratss::RationalPoint3 MyBaseClass;
public:
	RationalPoint3();
	RationalPoint3(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom);
	RationalPoint3(const sserialize::UByteArrayAdapter & src);
	~RationalPoint3();
};

}}}}//end namespace

#endif