#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/utility/exceptions.h>
#include <vendor/libs/minilzo/lzoconf.h>


namespace sserialize {
namespace Static {
namespace spatial {

GeoHierarchy::Cell::Cell() : 
m_itemPtr(0)
{}

GeoHierarchy::Cell::Cell(const UByteArrayAdapter & data) {
	int len = 0;
	m_itemPtr = data.getVlPackedUint32(0, len);
	if (len < 0) {
		throw sserialize::CorruptDataException("sserialize::Static::spatial::GeoHierarchy::Cell::Cell");
	}
	m_parents = ItemIndex(data+len, ItemIndex::T_REGLINE);
}


GeoHierarchy::Cell::~Cell() {}

GeoHierarchy::Region::Region() :
m_pos(0)
{}

GeoHierarchy::Region::Region(uint32_t pos, const GeoHierarchy * db) :
m_pos(pos),
m_db(db)
{}

GeoHierarchy::Region::~Region() {}

sserialize::spatial::GeoShapeType GeoHierarchy::Region::type() const {
	return  (sserialize::spatial::GeoShapeType) m_db->regions().at(m_pos, RD_TYPE);
}

uint32_t GeoHierarchy::Region::id() const {
	return m_db->regions().at(m_pos, RD_ID);
}

uint32_t GeoHierarchy::Region::cellIndexPtr() const {
	return m_db->regions().at(m_pos, RD_CELL_LIST_PTR);
}

uint32_t GeoHierarchy::Region::parentsBegin() const {
	return (m_db->regions().at(m_pos, RD_CHILDREN_BEGIN) + m_db->regions().at(m_pos, RD_PARENTS_OFFSET));
}

uint32_t GeoHierarchy::Region::parentsEnd() const {
	return  m_db->regions().at(m_pos+1, RD_CHILDREN_BEGIN);
}

uint32_t GeoHierarchy::Region::parentsSize() const {
	return parentsEnd() - parentsBegin();
}

uint32_t GeoHierarchy::Region::parent(uint32_t pos) const {
	uint32_t pB = parentsBegin();
	uint32_t pE = parentsEnd();
	if (pB + pos < pE) {
		return m_db->regionPtrs().at( pB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t GeoHierarchy::Region::childrenBegin() const {
	return m_db->regions().at(m_pos, RD_CHILDREN_BEGIN);
}

uint32_t GeoHierarchy::Region::childrenEnd() const {
	return parentsBegin();
}

uint32_t GeoHierarchy::Region::childrenSize() const {
	return childrenEnd() - childrenBegin();
}

uint32_t GeoHierarchy::Region::child(uint32_t pos) const {
	uint32_t cB = childrenBegin();
	uint32_t cE = childrenEnd();
	if (cB + pos < cE) {
		return m_db->regionPtrs().at( cB + pos );
	}
	return GeoHierarchy::npos;
}

GeoHierarchy::GeoHierarchy() {}

GeoHierarchy::GeoHierarchy(const UByteArrayAdapter & data) :
m_cells(data + 1),
m_regions(data + (1+m_cells.getSizeInBytes())),
m_regionPtrs(data + (1+m_regions.getSizeInBytes() + m_cells.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION, data.at(0), "sserialize::Static::GeoHierarchy");
}

GeoHierarchy::~GeoHierarchy() {}

OffsetType GeoHierarchy::getSizeInBytes() const {
	return 1+ m_cells.getSizeInBytes() + m_regionPtrs.getSizeInBytes() + m_regions.getSizeInBytes();
}

uint32_t GeoHierarchy::regionSize() const {
	uint32_t rs  = m_regions.size();
	return (rs > 0 ? rs-1 : 0);
}

GeoHierarchy::Region GeoHierarchy::region(uint32_t id) const {
	return Region(id, this);
}


uint32_t GeoHierarchy::regionPtrSize() const {
	return m_regionPtrs.size();
}

uint32_t GeoHierarchy::regionPtr(uint32_t pos) const {
	return m_regionPtrs.at(pos);
}

std::ostream & GeoHierarchy::printStats(std::ostream & out) const {
	out << "sserialize::Static::spatial::GeoHierarchy::stats--BEGIN" << std::endl;
	out << "regions.size()=" << regionSize() << std::endl;
	out << "regionPtrs.size()=" << regionPtrSize() << std::endl;
	out << "cells.size()=" << cellSize() << std::endl;
	out << "total data size=" << getSizeInBytes() << std::endl;
	out << "regions data size=" << m_regions.getSizeInBytes() << std::endl;
	out << "region ptr data size=" << m_regionPtrs.getSizeInBytes() << std::endl;
	out << "cell data size=" << m_cells.getSizeInBytes() << std::endl;
	out << "sserialize::Static::spatial::GeoHierarchy::stats--END" << std::endl;
	return out;
}


}}} //end namespace