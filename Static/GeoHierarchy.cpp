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

GeoHierarchy::Region::Region(uint32_t pos, const MultiVarBitArray & rdesc, const CompactUintArray & ptrs) :
m_pos(pos),
m_rdesc(rdesc),
m_ptrs(ptrs)
{}

GeoHierarchy::Region::~Region() {}

uint32_t GeoHierarchy::Region::parentsBegin() const {
	return (m_rdesc.at(m_pos, RD_CHILDREN_BEGIN) + m_rdesc.at(m_pos, RD_PARENTS_OFFSET));
}

uint32_t GeoHierarchy::Region::parentsEnd() const {
	return  m_rdesc.at(m_pos+1, RD_CHILDREN_BEGIN);
}

uint32_t GeoHierarchy::Region::parentsSize() const {
	return parentsEnd() - parentsBegin();
}

uint32_t GeoHierarchy::Region::parent(uint32_t pos) const {
	uint32_t pB = parentsBegin();
	uint32_t pE = parentsEnd();
	if (pB + pos < pE) {
		return m_ptrs.at( pB + pos );
	}
	return GeoHierarchy::npos;
}

uint32_t GeoHierarchy::Region::childrenBegin() const {
	return m_rdesc.at(m_pos, RD_CHILDREN_BEGIN);
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
		return m_ptrs.at( cB + pos );
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

uint32_t GeoHierarchy::regionSize() const {
	uint32_t rs  = m_regions.size();
	return (rs > 0 ? rs-1 : 0);
}

GeoHierarchy::Region GeoHierarchy::region(uint32_t id) const {
	return Region(id, m_regions, m_regionPtrs);
}


uint32_t GeoHierarchy::regionPtrSize() const {
	return m_regionPtrs.size();
}

uint32_t GeoHierarchy::regionPtr(uint32_t pos) {
	return m_regionPtrs.at(pos);
}


}}} //end namespace