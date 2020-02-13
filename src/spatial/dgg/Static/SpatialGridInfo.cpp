#include <sserialize/spatial/dgg/Static/SpatialGridInfo.h>

#include <sserialize/Static/Version.h>

namespace sserialize::spatial::dgg::Static {

namespace ssinfo::SpatialGridInfo {


sserialize::UByteArrayAdapter::SizeType
MetaData::getSizeInBytes() const {
	return 
		sserialize::SerializationInfo<decltype(MetaData::version)>::length+
		sserialize::SerializationInfo<Types<DataMembers::type>::type>::sizeInBytes(m_d->type())+
		sserialize::SerializationInfo<Types<DataMembers::levels>::type>::sizeInBytes(m_d->levels())+
		m_d->trixelId2HtmIndexId().getSizeInBytes()+
		m_d->htmIndexId2TrixelId().getSizeInBytes()+
		m_d->trixelItemIndexIds().getSizeInBytes();
}

sserialize::UByteArrayAdapter::SizeType
MetaData::offset(DataMembers member) const {
	sserialize::UByteArrayAdapter::SizeType offset = sserialize::SerializationInfo<decltype(MetaData::version)>::length;
	if (member == DataMembers::type) {
		return offset;
	}
	offset += sserialize::SerializationInfo<Types<DataMembers::type>::type>::sizeInBytes(m_d->type());
	if (member == DataMembers::levels) {
		return offset;
	}
	offset += sserialize::SerializationInfo<Types<DataMembers::levels>::type>::sizeInBytes(m_d->levels());
	if (member == DataMembers::trixelId2HtmIndexId) {
		return offset;
	}
	offset += m_d->trixelId2HtmIndexId().getSizeInBytes();
	if (member == DataMembers::htmIndexId2TrixelId) {
		return offset;
	}
	offset += m_d->htmIndexId2TrixelId().getSizeInBytes();
	if (member == DataMembers::trixelItemIndexIds) {
		return offset;
	}
	throw sserialize::InvalidEnumValueException("MetaData");
	return 0;
}


Data::Data(sserialize::UByteArrayAdapter d) {
	sserialize::Static::ensureVersion(MetaData::version, d.getUint8(0));
	++d;
	d >> m_type >> m_levels >> m_trixelId2HtmIndexId >> m_htmIndexId2TrixelId >> m_trixelId2HtmIndexId;

}

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
