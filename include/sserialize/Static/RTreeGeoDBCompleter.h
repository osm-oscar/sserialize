#ifndef SSERIALIZE_RTREE_GEO_DB_COMPLETER_H
#define SSERIALIZE_RTREE_GEO_DB_COMPLETER_H
#include <sserialize/Static/RTree.h>
#include <sserialize/completers/GeoCompleterPrivate.h>

namespace sserialize {
namespace Static {
namespace spatial {

template<typename T_GEO_DB>
class RTreeGeoDBIntersecter: public RTree::ElementIntersecter {
private:
	T_GEO_DB m_db;
public:
	RTreeGeoDBIntersecter(const T_GEO_DB & db) : m_db(db) {}
	virtual ~RTreeGeoDBIntersecter() {}
	virtual bool operator()(uint32_t id, const sserialize::spatial::GeoRect & rect) const {
		return m_db.match(id, rect);
	}
};

template<typename T_GEO_DB>
class  RTreeGeoDBCompleter: public GeoCompleterPrivate {
private:
	RTreeGeoDBIntersecter<T_GEO_DB> m_intersecter;
	Static::spatial::RTree m_rtree;
public:
	RTreeGeoDBCompleter(const T_GEO_DB & db, const Static::spatial::RTree & rtree) :
	m_intersecter(db),
	m_rtree(rtree)
	{}
	virtual ~RTreeGeoDBCompleter() {}
	virtual ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool approximate) {
		DynamicBitSet bitSet;
		if (approximate) {
			m_rtree.intersect(rect, bitSet, 0);
		}
		else {
			m_rtree.intersect(rect, bitSet, &m_intersecter);
		}
		return bitSet.toIndex(m_rtree.indexStore().indexType());
	}
	virtual ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool approximate) {
		return ItemIndexIterator( complete(rect, approximate) );
	}
	virtual ItemIndex filter(const sserialize::spatial::GeoRect& rect, bool approximate, const ItemIndex& partner) {
		return complete(rect, approximate) / partner;
	}
	virtual ItemIndexIterator filter(const sserialize::spatial::GeoRect& rect, bool approximate, const ItemIndexIterator& partner) {
		return partialComplete(rect, approximate) / partner;
	}
	
	virtual std::ostream & printStats(std::ostream & out) const {
		return (out << "RTreeGeoDBCompleter has no stats" << std::endl);
	}
	virtual std::string getName() const {
		return "RTreeGeoDBCompleter";
	}
};


}}}//end namespace

#endif