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
m_cellPtr(0),
m_type(sserialize::spatial::GS_NONE),
m_parent(0),
m_id(0)
{}

inline uint32_t ugly_hack_function(UByteArrayAdapter & data) {
	data.resetGetPtr();
	return data.getVlPackedUint32();
}

GeoHierarchy::Region::Region(UByteArrayAdapter data) :
m_cellPtr(ugly_hack_function(data)),
m_type(static_cast<sserialize::spatial::GeoShapeType>(data.getUint8())),
m_parent(data.getVlPackedUint32()),
m_id(data.getVlPackedUint32()),
m_children(data.shrinkToGetPtr())
{}

GeoHierarchy::Region::~Region() {}

GeoHierarchy::GeoHierarchy() {}

GeoHierarchy::GeoHierarchy(const UByteArrayAdapter & data) : 
m_regions(data+1),
m_cells(data+(1+m_regions.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_GEO_HIERARCHY_VERSION, data.at(0), "sserialize::Static::GeoHierarchy");
}

GeoHierarchy::~GeoHierarchy() {}

}}} //end namespace