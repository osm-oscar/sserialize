#ifndef SSERIALIZE_GEO_HIERARCHY_SUB_SET_CREATOR_H
#define SSERIALIZE_GEO_HIERARCHY_SUB_SET_CREATOR_H
#include <sserialize/Static/GeoHierarchy.h>

namespace sserialize {
namespace spatial {
namespace detail {

class GeoHierarchySubSetCreator: public RefCountObject {
private:
	struct RegionDesc {
		RegionDesc(uint32_t parentsBegin) : parentsBegin(parentsBegin) {}
		uint32_t parentsBegin;
	};
	struct CellDesc {
		CellDesc(uint32_t parentsBegin, uint32_t itemsCount) : parentsBegin(parentsBegin), itemsCount(itemsCount) {}
		uint32_t parentsBegin;
		uint32_t itemsCount;
	};
public:
	typedef sserialize::Static::spatial::GeoHierarchy::SubSet SubSet;
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	std::vector<uint32_t> m_cellParentsPtrs;
	std::vector<uint32_t> m_regionParentsPtrs;
	std::vector<RegionDesc> m_regionDesc;
	std::vector<CellDesc> m_cellDesc;
private:
	SubSet::Node * createSubSet(const CellQueryResult & cqr, SubSet::Node** nodes, uint32_t size) const;
	SubSet::Node * createSubSet(const CellQueryResult & cqr, std::unordered_map<uint32_t, SubSet::Node*> & nodes) const;
public:
	GeoHierarchySubSetCreator();
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh);
	~GeoHierarchySubSetCreator();
	SubSet::Node * subSet(const sserialize::CellQueryResult & cqr);
};

}//end namespace detail

class GeoHierarchySubSetCreator {
private:
	RCPtrWrapper<detail::GeoHierarchySubSetCreator> m_ghs;
public:
	GeoHierarchySubSetCreator() {}
	GeoHierarchySubSetCreator(const sserialize::Static::spatial::GeoHierarchy & gh) : 
	m_ghs(new detail::GeoHierarchySubSetCreator(gh)) {}
	~GeoHierarchySubSetCreator() {}
	inline sserialize::Static::spatial::GeoHierarchy::SubSet subSet(const sserialize::CellQueryResult & cqr) {
		return sserialize::Static::spatial::GeoHierarchy::SubSet(m_ghs->subSet(cqr), cqr);
	}
};


}}//end namespace

#endif