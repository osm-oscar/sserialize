#ifndef SSERIALIZE_SPATIAL_SHAPE_ALGORITHM_H
#define SSERIALIZE_SPATIAL_SHAPE_ALGORITHM_H
#include <sserialize/spatial/GeoPoint.h>

namespace sserialize {
namespace spatial {

struct SegmentTraits {
	struct SegmentIntersects {
		bool operator()(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s);
	};
};

template<typename T_POLYGON_TYPE>
struct PolygonTraits {
	typedef T_POLYGON_TYPE PolygonType;
	typedef typename T_POLYGON_TYPE::Point Point;
	
	//and further typedefs for
	
	struct PolygonEncloses {
		bool operator()(const PolygonType & poly, const Point & point) const;
	};
	struct PolygonIntersects {
		bool operator()(const PolygonType & poly, const Point & point) const;
	};
};

namespace detail {
	
	template<typename T_POLYGON_A, typename T_POINT_TYPE>
	bool polygon_point_encloses(const T_POLYGON_A & poly, const T_POINT_TYPE & p) {
		
	}
	
	template<typename T_POLYGON_A, typename T_POINT_TYPE>
	bool polygon_polyline_encloses(const T_POLYGON_A & poly, const T_POINT_TYPE & p) {
		
	}
	
	template<typename T_POLYGON_A, typename T_POINT_TYPE>
	bool polygon_polygon_encloses(const T_POLYGON_A & poly, const T_POINT_TYPE & p) {
		
	}
	
	template<typename T_POLYGON_A, typename T_POINT_TYPE>
	bool polygon_polygon_encloses(const T_POLYGON_A & poly, const T_POINT_TYPE & p) {
		
	}

	///Polygon-polyline intersection test, 
	///Concepts:
	///{
	///   typedef PointType Point;
	///   PointIterator begin();
	///   PointIterator end();
	///   GeoRect boundary();
	///}
	template<typename T_POLYGON_A, typename T_POLYGON_B>
	bool polygon_polyline_intersect(const T_POLYGON_A & apoly, const T_POLYGON_B & bpoly) {
		
	}
	
	///Polygon-polygon intersection test, a polygon has to provide the functions begin(), end() and a typedef Point
	/// (polygons need to have the same point type)
	template<typename T_POLYGON_A, typename T_POLYGON_B, typename T_POLYGON_A_TRAITS, typename T_POLYGON_B_TRAITS>
	bool polygon_polygon_intersect(const T_POLYGON_A & apoly, const T_POLYGON_B & bpoly, const T_POLYGON_A_TRAITS & at, const T_POLYGON_B_TRAITS & bt) {
		if (! apoly.boundary().overlap(bpoly.boundary())) {
			return false;
		}
		
		{
			typename T_POLYGON_A_TRAITS::Encloses aEnc(at.encloses());
			for(auto it(bpoly.begin()), end(bpoly.end()); it != end; ++it) {
				if (aEnc(apoly, *it)) {
					return true;
				}
			}
		}
		{
			typename T_POLYGON_B_TRAITS::Encloses bEnc(bt.encloses());
			for(auto it(bpoly.begin()), end(bpoly.end()); it != end; ++it) {
				if (bEnc(bpoly, *it)) {
					return true;
				}
			}
		}
		{
			const_iterator oIt(bpoly.cbegin());
			++oIt;
			for(const_iterator oPrev(bpoly.cbegin()), oEnd(bpoly.cend()); oIt != oEnd; ++oIt, ++oPrev) {
				const_iterator it(apoly.begin());
				++it;
				for(const_iterator prev(apoly.cbegin()), end(apoly.cend()); it != end; ++prev, ++it) {
					if (sserialize::spatial::GeoPoint::intersect(*prev, *it, *oPrev, *oIt)) {
						return true;
					}
				}
			}
		}
		return false;
	}
	
	
}//end namespace


}}//end namespace

#endif
