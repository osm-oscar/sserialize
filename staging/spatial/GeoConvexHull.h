#ifndef SSERIALIZE_SPATIAL_GEO_CONVEX_HULL_H
#define SSERIALIZE_SPATIAL_GEO_CONVEX_HULL_H
#include <sserialize/spatial/GeoPolygon.h>


namespace sserialize {
namespace spatial {
namespace detail {

/** This is a special polygon type representing a convex hull for faster point-in-polygon queries
  *
  * The point-in-polygon-test is speed-up by the idea presented in:
  * http://geomalgorithms.com/a03-_inclusion.html#Monotone-Polygons
  * We demand that the first point in the points-container is the lowest point of the convex-hull
  *
  *
  *
  */

template<typename TPointsContainer>
class GeoConvexHull: public sserialize::spatial::detail::GeoPolygon<TPointsContainer> {
public:
	typedef sserialize::spatial::detail::GeoPolygon<TPointsContainer> MyBaseClass;
	typedef typename TPointsContainer::reverse_iterator reverse_iterator;
	typedef typename TPointsContainer::const_reverse_iterator const_reverse_iterator;
protected:
	uint32_t m_maxPoint; //offset to point with maximum y-coordinate
public:
	GeoConvexHull();
	GeoConvexHull(const sserialize::UByteArrayAdapter & d);
	GeoConvexHull(const sserialize::spatial::GeoRect & boundary, const TPointsContainer & points);
	GeoConvexHull(const sserialize::spatial::GeoRect & boundary, TPointsContainer && points);
	GeoConvexHull(const TPointsContainer & points);
	GeoConvexHull(TPointsContainer && points);
	GeoConvexHull(const GeoConvexHull & other);
	GeoConvexHull(GeoConvexHull && other);
	virtual ~GeoConvexHull();
	GeoConvexHull & operator=(GeoConvexHull && other);
	GeoConvexHull & operator=(const GeoConvexHull & other);
	void swap(GeoConvexHull & other);
	inline reverse_iterator rbegin() { return MyBaseClass::points().rbegin(); }
	inline const_reverse_iterator rbegin() const { return MyBaseClass::points().rbegin(); }
	inline const_reverse_iterator crbegin() const { return MyBaseClass::points().crbegin(); }

	inline reverse_iterator rend() { return MyBaseClass::points().rend(); }
	inline const_reverse_iterator rend() const { return MyBaseClass::points().rend(); }
	inline const_reverse_iterator crend() const { return MyBaseClass::points().crend(); }

	virtual bool contains(const GeoPoint & p);
	
	template<typename T_POINTS_ITERATOR>
	static bool isConvex(T_POINTS_ITERATOR begin, const T_POINTS_ITERATOR & end);
};


}}}//end namespace


#endif