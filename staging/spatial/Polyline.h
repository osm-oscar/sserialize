#ifndef SSERIALIZE_SPATIAL_POLY_LINE_H
#define SSERIALIZE_SPATIAL_POLY_LINE_H
#include <vector>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/spatial/Segment.h>

namespace sserialize {
namespace spatial {

class Polyline {
public:
	typedef sserialize::spatial::GeoPoint value_type;
	typedef std::vector<value_type> container_type;
	typedef container_type::const_reference const_reference;
	typedef container_type::reference reference;
	typedef container_type::const_iterator const_iterator;
	typedef container_type::iterator iterator;
public:
	Polyline();
	Polyline(const container_type & data);
	~Polyline();
	const_iterator begin();
	const_iterator end();
private:
	container_type m_d;
};

double length_wgs84(const Polyline & point);
double length_euclidean(const Polyline & point);
bool intersect(const Polyline & line, const GeoPoint & point);
bool intersect(const GeoPoint & point, const Polyline & line);
bool intersect(const PolyLine & line, const Polyline & other);

}}//end namespace

#endif