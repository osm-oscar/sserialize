#include <sserialize/spatial/dgg/Static/SpatialGridInfo.h>

#include <sserialize/Static/Version.h>

namespace sserialize::spatial::dgg::Static {

namespace ssinfo::SpatialGridInfo {


sserialize::UByteArrayAdapter::SizeType
MetaData::getSizeInBytes() const {
	return 3+m_d->trixelId2HtmIndexId().getSizeInBytes()+m_d->htmIndexId2TrixelId().getSizeInBytes()+m_d->trixelItemIndexIds().getSizeInBytes();;
}

sserialize::UByteArrayAdapter::SizeType
MetaData::offset(DataMembers member) const {
	switch(member) {
		case DataMembers::type:
			return 1;
		case DataMembers::levels:
			return 2;
		case DataMembers::trixelId2HtmIndexId:
			return 3;
		case DataMembers::htmIndexId2TrixelId:
			return 3+m_d->trixelId2HtmIndexId().getSizeInBytes();
		case DataMembers::trixelItemIndexIds:
			return 3+m_d->trixelId2HtmIndexId().getSizeInBytes()+m_d->htmIndexId2TrixelId().getSizeInBytes();
		default:
			throw sserialize::InvalidEnumValueException("MetaData");
			break;
	};
	return 0;
}


Data::Data(const sserialize::UByteArrayAdapter & d) :
m_type(decltype(m_type)(sserialize::Static::ensureVersion(d, MetaData::version, d.getUint8(0)).getUint8(1))),
m_levels(d.getUint8(2)),
m_trixelId2HtmIndexId(d+3),
m_htmIndexId2TrixelId(d+(3+m_trixelId2HtmIndexId.getSizeInBytes())),
m_trixelItemIndexIds(d+(3+m_trixelId2HtmIndexId.getSizeInBytes()+m_htmIndexId2TrixelId.getSizeInBytes()))
{}

} //end namespace ssinfo::HtmInfo
	
SpatialGridInfo::SpatialGridInfo(const sserialize::UByteArrayAdapter & d) :
m_d(d)
{}

SpatialGridInfo::~SpatialGridInfo() {}

sserialize::UByteArrayAdapter::SizeType
SpatialGridInfo::getSizeInBytes() const {
	return MetaData(&m_d).getSizeInBytes();
}

int SpatialGridInfo::levels() const {
	return m_d.levels();
}

SpatialGridInfo::SizeType
SpatialGridInfo::cPixelCount() const {
	return m_d.trixelId2HtmIndexId().size();
}

SpatialGridInfo::ItemIndexId
SpatialGridInfo::itemIndexId(CPixelId trixelId) const {
	return m_d.trixelItemIndexIds().at(trixelId);
}

SpatialGridInfo::CPixelId
SpatialGridInfo::cPixelId(SGPixelId htmIndex) const {
	return m_d.htmIndexId2TrixelId().at(htmIndex);
}

bool
SpatialGridInfo::hasSgIndex(SGPixelId htmIndex) const {
	return m_d.htmIndexId2TrixelId().contains(htmIndex);
}

SpatialGridInfo::SGPixelId
SpatialGridInfo::sgIndex(CPixelId cPixelId) const {
	return m_d.trixelId2HtmIndexId().at64(cPixelId);
}

}//end namespace sserialize::spatial::dgg::Static
