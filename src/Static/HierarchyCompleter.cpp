#include <sserialize/Static/HieararchyCompleter.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/Static/StringCompleter.h>

namespace sserialize {
namespace Static {
namespace spatial {

HierarchyCompleter::HierarchyCompleter() : m_myType(sserialize::spatial::GS_NONE) {}

HierarchyCompleter::HierarchyCompleter(const sserialize::UByteArrayAdapter & d, sserialize::Static::ItemIndexStore & idxStore) :
m_myType(static_cast<sserialize::spatial::GeoShapeType>(d.getUint8(SerializationInfo<uint8_t>::length))),
m_indexIds(d+2*SerializationInfo<uint8_t>::length),
m_cmp(new Static::StringCompleter(d+(SerializationInfo<uint8_t>::length+SerializationInfo<MultiVarBitArray>::sizeInBytes(m_indexIds)), idxStore)),
m_idxStore(idxStore)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_HIERARCHY_COMPLETER_VERSION, d.getUint8(0), "sserialize::Static::spatial::HierarchyCompleter");
}

HierarchyCompleter::~HierarchyCompleter() {}


sserialize::spatial::GeoShapeType HierarchyCompleter::type() const {
	return m_myType;
}

UByteArrayAdapter::OffsetType HierarchyCompleter::getSizeInBytes() const {
	return 2*SerializationInfo<uint8_t>::length + SerializationInfo<MultiVarBitArray>::sizeInBytes(m_indexIds) + m_cmp->getSizeInBytes();
}

sserialize::ItemIndex HierarchyCompleter::complete(const std::string & qstr, sserialize::StringCompleter::QuerryType qt) {
	return m_cmp->complete(qstr, qt);
}

sserialize::ItemIndex HierarchyCompleter::intersect(uint32_t id, sserialize::spatial::GeoShapeType t, const sserialize::ItemIndex & idx) const {
	if (t >= m_myType ||m_myType <= sserialize::spatial::GS_FIRST_SPATIAL_OBJECT || id >= m_indexIds.size())
		return ItemIndex();
	return m_idxStore.at(m_indexIds.at(t-sserialize::spatial::GS_FIRST_SPATIAL_OBJECT , id)) / idx;
}

}}//end spatial/Static namespaces
}//end namespace