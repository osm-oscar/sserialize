#ifndef SSERIALIZE_SPATIAL_SEGMENT_H
#define SSERIALIZE_SPATIAL_SEGMENT_H
#include <sserialize/spatial/GeoPoint.h>

namespace sserialize {
namespace spatial {

class Segment {
public:
	typedef sserialize::spatial::GeoPoint point_type;
public:
	Segment();
	Segment(const point_type & begin, const point_type & end);
	~Segment();
private:
	point_type m_begin;
	point_type m_end;
};

}}//end namespace sserialize::spatial

#endif