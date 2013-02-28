#ifndef SSERIALIZE_SPATIAL_POLYGON_STORE_H
#define SSERIALIZE_SPATIAL_POLYGON_STORE_H
#include <vector>
#include <set>
#include <sserialize/spatial/RWGeoGrid.h>
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/utility/ProgressInfo.h>
#include <iostream>
#include <fstream>

//TODO: look into sweep line algo

namespace sserialize {
namespace spatial {

/** As of now, this only works for lat,lon > 0.0 **/
template<class TValue>
class PolygonStore {
public:
	typedef GeoPoint Point;
	typedef GeoPolygon Polygon;

private:

	typedef std::vector<uint32_t> PolyRasterElement;
	/** This class holds the tile data of the PolyRaster. Before final deletion, deleteStorage() has to be called on the LAST copy! */
	struct RasterElementPolygons {
		RasterElementPolygons() : enclosing(0), colliding(0) {}
		virtual ~RasterElementPolygons() {}
		PolyRasterElement * enclosing; //Polygons that fully enclose this raster-element
		PolyRasterElement * colliding; //Polygons that collide with this raster element, but don't enclose it
		std::ostream & operator<<(std::ostream & out) const {
			PolyRasterElement::const_iterator end;
			out << "BEGIN-ENCLOSING" << std::endl;
			if (enclosing) {
				end = enclosing->end();
				for(PolyRasterElement::const_iterator it = enclosing->begin(); it != end; ++it) {
					out << *it << std::endl;
				}
			}
			out << "END-ENCLOSING" << std::endl;
			out << "BEGIN-COLLIDING" << std::endl;
			if (colliding) {
				end = colliding->end();
				for(PolyRasterElement::const_iterator it = colliding->begin(); it != end; ++it) {
					out << *it << std::endl;
				}
			}
			out << "END-COLLIDING" << std::endl;
			return out;
		}
		void deleteStorage() {
			delete enclosing;
			enclosing = 0;
			delete colliding;
			colliding = 0;
		}
	};

	class PolyRaster: private RWGeoGrid< RasterElementPolygons, std::vector<RasterElementPolygons> > {
		friend class PolygonStore;
		typedef RWGeoGrid< RasterElementPolygons, std::vector<RasterElementPolygons> > MyRWGeoGrid;
		typedef RGeoGrid<RasterElementPolygons, std::vector<RasterElementPolygons> > MyRGeoGrid;
	private:
		void createCellPoly(const GeoRect & cellRect, Polygon & cellPoly) const {
			cellPoly.points().push_back( Point(cellRect.lat()[0], cellRect.lon()[0]) );
			cellPoly.points().push_back( Point(cellRect.lat()[1], cellRect.lon()[0]) );
			cellPoly.points().push_back( Point(cellRect.lat()[1], cellRect.lon()[1]) );
			cellPoly.points().push_back( Point(cellRect.lat()[0], cellRect.lon()[1]) );
			cellPoly.updateBoundaryRect();
		}
		void createCellPoly(Polygon & cellPoly, uint32_t i , uint32_t j) {
			GeoRect cellRect(MyRWGeoGrid::cellBoundary(i,j));
			createCellPoly(cellRect, cellPoly);
		}
	public:
		PolyRaster(const GeoRect & bounds, uint32_t latcount, uint32_t loncount) :
		RWGeoGrid<RasterElementPolygons>(bounds, latcount, loncount)
		{
			MyRGeoGrid::storage().resize(latcount*loncount);
		}
		~PolyRaster() { 
			for(size_t i = 0; i < MyRGeoGrid::storage().size(); i++) {
				MyRGeoGrid::storage().at(i).deleteStorage();
			}
		}
		
		bool addPolygon(Polygon & p, uint32_t polyId) {
			//First, get the bbox, then test all rasterelements for inclusion
			Point bboxBL = p.getBottomLeft();
			Point bboxTR = p.getTopRight();
			GeoGrid::GridBin blBin = GeoGrid::select(bboxBL.lat, bboxBL.lon);
			GeoGrid::GridBin trBin = GeoGrid::select(bboxTR.lat, bboxTR.lon);
			if (!blBin.valid() || !trBin.valid())
				return false;
			unsigned int xbinStart = blBin.x;
			unsigned int ybinStart = blBin.y;
			unsigned int xbinEnd = trBin.x;
			unsigned int ybinEnd = trBin.y;
			if (xbinStart == xbinEnd && ybinStart == ybinEnd) { //one cell
				if (!MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding)
					MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding = new PolyRasterElement;
				MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding->push_back(polyId);
			}
			else { //polygon spans multiple cells
				std::vector< std::pair<uint32_t, uint32_t> > cells;
				cells.reserve((xbinEnd-xbinStart+1)*(ybinEnd-ybinStart+1));
				for(unsigned int i = xbinStart; i <= xbinEnd; i++) {
					for(unsigned int j = ybinStart; j <= ybinEnd; j++) {
						cells.push_back(std::pair<uint32_t, uint32_t>(i,j));
					}
				}
				size_t cellsSize = cells.size();
#pragma omp parallel for schedule(dynamic, 5)
				for(size_t k=0; k < cellsSize; k++) {
					unsigned int i = cells[k].first;
					unsigned int j = cells[k].second;
					GeoRect cellRect( MyRGeoGrid::cellBoundary(i, j) );
					//test enclosing
					if (p.test( GeoPoint(cellRect.lat()[0], cellRect.lon()[0]) ) && p.test( GeoPoint(cellRect.lat()[1], cellRect.lon()[1]) ) ) {
						if (!MyRWGeoGrid::binAt(i,j).enclosing) {
							MyRWGeoGrid::binAt(i,j).enclosing = new PolyRasterElement;
						}
						MyRWGeoGrid::binAt(i,j).enclosing->push_back(polyId);
					}
					else {
						Polygon cellPoly;
						createCellPoly(cellRect, cellPoly);
						if (cellPoly.collidesWithPolygon(p)) {
							if (!MyRWGeoGrid::binAt(i,j).colliding) {
								MyRWGeoGrid::binAt(i,j).colliding = new PolyRasterElement;
							}
							MyRWGeoGrid::binAt(i,j).colliding->push_back(polyId);
						}
					}
				}
			}
			return true;
		}
		
		RasterElementPolygons checkedAt(const Point & p) const {
			return MyRGeoGrid::at(p.lat, p.lon);
		}

		void getPolygons(const Point & p, std::set<uint32_t> & definiteHits, std::set<uint32_t> & possibleHits) const {
			RasterElementPolygons polys = MyRGeoGrid::at(p.lat, p.lon);
			if (polys.enclosing)
				definiteHits.insert(polys.enclosing->begin(), polys.enclosing->end());

			if (polys.colliding)
				possibleHits.insert(polys.colliding->begin(), polys.colliding->end());

		}
		
		void getPolygons(const std::vector<Point> & p, std::set<uint32_t> & definiteHits, std::set<uint32_t> & possibleHits) const {
			if (!p.size())
				return std::set<uint32_t>();
			if (p.size() == 1)
				return getPolygons(p.front(), definiteHits, possibleHits);
			
			std::vector<Point>::const_iterator end( p.end());
			for(std::vector<Point>::const_iterator it = p.begin(); it != end; ++it)
				getPolygons(*it, definiteHits, possibleHits);
		}
		
		std::ostream & dump(std::ostream & out) const {
			out << "POLYRASTER" << std::endl;
			out << this->rect().lat()[0] << std::endl;
			out << this->rect().lon()[0] << std::endl;
			out << this->rect().lat()[1] << std::endl;
			out << this->rect().lon()[1] << std::endl;
			out << this->latCount() << std::endl;
			out << this->lonCount() << std::endl;
			for(size_t i = 0; i < this->storage().size(); i++) {
				out << "STARTBIN" << std::endl;
				this->storage().at(i).operator<<(out);
				out << "ENDBIN" << std::endl;
			}
			return out;
		}
	};

private:
	std::vector<Polygon> m_polyStore;
	std::vector<TValue> m_values;
	PolyRaster * m_polyRaster;
	
private:
	inline bool pointInPolygon(const Point & p, const Polygon & pg) { return pg.test(p);}

	
public:
	PolygonStore(): m_polyRaster(0) {}
	~PolygonStore() {}
	const std::vector<Polygon> & polygons() const { return m_polyStore; }
	const std::vector<TValue> & values() const { return m_values; }
	inline void push_back(const Polygon & p, const TValue & value) {
		m_polyStore.push_back(p);
		m_values.push_back(value);
	}
	
	void addPolygonsToRaster(unsigned int gridLatCount, unsigned int gridLonCount) {
		if (!m_polyStore.size())
			return;
		if (m_polyRaster) {
			delete m_polyRaster;
			m_polyRaster = 0;
		}
		

		GeoRect bbox( m_polyStore.front().boundaryRect() );
		for(size_t i=0; i < m_polyStore.size(); i++) {
			bbox.enlarge( m_polyStore[i].boundaryRect() );
		}
		
		std::cout << "Creating PolyRaster with " << gridLatCount << "x" << gridLonCount << "bins. BBox: " << bbox << std::endl;
		m_polyRaster = new PolyRaster(bbox, gridLatCount, gridLonCount);
		sserialize::ProgressInfo info;
		info.begin(m_polyStore.size(), "Polyraster::addPolygon");
		for(size_t i=0; i < m_polyStore.size(); i++) {
			m_polyRaster->addPolygon(m_polyStore[i], i);
			info(i);
		}
		info.end("Polyraster::addPolygon");
		std::cout << std::endl;
	}
	
	inline std::set<uint32_t> test(double lat, double lon) {
		return test(Point(lat, lon));
	}
	
	std::set<uint32_t> test(const Point & p) {
		std::set<uint32_t> polys;
		if (!m_polyRaster) {
			for(size_t it = 0; it < m_polyStore.size(); it++) {
				if ( pointInPolygon(p, m_polyStore[it]) ) {
					polys.insert(it);
				}
			}
		}
		else {
			RasterElementPolygons rep( m_polyRaster->checkedAt(p) );
			if (rep.enclosing)
				polys.insert(rep.enclosing->begin(), rep.enclosing->end());
			if (rep.colliding) {
				PolyRasterElement::const_iterator end( rep.colliding->end());
				for(PolyRasterElement::const_iterator it = rep.colliding->begin(); it != end; ++it) {
					if (pointInPolygon(p, m_polyStore[*it]) ) {
						polys.insert(*it);
					}
				}
			}
		}
		return polys;
	}
	
	std::set<uint32_t> test(const std::vector<Point> & p) {
		std::set<unsigned int> polyIds;
		size_t polyStoreSize = m_polyStore.size();
		if (!m_polyRaster) {
			for(size_t it = 0; it < polyStoreSize; it++) {
				for(size_t pit = 0; pit < p.size(); pit++) {
					if ( pointInPolygon(p[pit], m_polyStore[it]) ) {
						polyIds.insert(it);
						break; // at least one point is within the currently tested polygon
					}
				}
			}
		}
		else {
			std::vector<Point>::const_iterator end( p.end() );
			for(std::vector<Point>::const_iterator it = p.begin(); it != end; ++it) {
				RasterElementPolygons rep( m_polyRaster->checkedAt(*it) );
				if (rep.enclosing)
					polyIds.insert(rep.enclosing->begin(), rep.enclosing->end());
				if (rep.colliding) {
					PolyRasterElement::const_iterator pend( rep.colliding->end());
					for(PolyRasterElement::const_iterator pit = rep.colliding->begin(); pit != pend; ++pit) {
						if (!polyIds.count(*pit) && pointInPolygon(*it, m_polyStore[*pit]) ) {
							polyIds.insert(*pit);
						}
					}
				}
			}
		}
		return polyIds;
	}
	
	std::set<uint32_t> test(const std::deque<Point> & p) {
		return test(std::vector<Point>(p.begin(), p.end()));
	}
	
	void dump(std::ostream & out) {
		out.precision(100);
		out << "POLYGONSTORE DUMP" << std::endl;
		out << m_polyStore.size() << std::endl;
		for(size_t i=0; i < m_polyStore.size(); i++) {
			out << m_values[i] << std::endl;
			out << i << std::endl;
			out << m_polyStore[i].points().size() << std::endl;
			size_t ppsize = m_polyStore[i].points().size();
			for(size_t j=0; j < ppsize; j++) {
				out << m_polyStore[i].points()[j].lat << std::endl;
				out << m_polyStore[i].points()[j].lon << std::endl;
			}
		}
		if (m_polyRaster) {
			m_polyRaster->dump(out);
		}
	}
	
	void dumpToFile(const std::string & filename) {
		std::ofstream file;
		file.open(filename.c_str());
		dump(file);
		file.close();
	}
	
};

}}//end namespace

#endif