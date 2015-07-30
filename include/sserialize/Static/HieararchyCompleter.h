#ifndef SSERIALIZE_STATIC_SPATIAL_HIERARCHY_COMPLETER_H
#define SSERIALIZE_STATIC_SPATIAL_HIERARCHY_COMPLETER_H
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/spatial/GeoShape.h>
#include <sserialize/storage/SerializationInfo.h>
#define SSERIALIZE_STATIC_SPATIAL_HIERARCHY_COMPLETER_VERSION 0

namespace sserialize {
namespace Static {
namespace spatial {
/** The PolygonCompleter completes a given string to polygon elements residing in a database
  * It supports intersections with lower-complexity spatial objects like ways and nodes
  * 
  * ------------------------------------------
  * VERSION|TYPE|INDEXIDS|STRINGCOMPLETER
  * ------------------------------------------
  *    u8  | u8 |MVBA    |Stringcompleter
  */

class HierarchyCompleter {
	///offsets for index ids with ptrs to lower-complexity types in m_indexIds
	sserialize::spatial::GeoShapeType m_myType;
	sserialize::MultiVarBitArray m_indexIds;
	RCPtrWrapper<sserialize::Static::StringCompleter> m_cmp;
	sserialize::Static::ItemIndexStore m_idxStore;
public:
	HierarchyCompleter();
	HierarchyCompleter(const sserialize::UByteArrayAdapter & d, sserialize::Static::ItemIndexStore & idxStore);
	virtual ~HierarchyCompleter();
	sserialize::spatial::GeoShapeType type() const;
	UByteArrayAdapter::OffsetType getSizeInBytes() const;
	sserialize::ItemIndex complete(const std::string & qstr, sserialize::StringCompleter::QuerryType qt);
	sserialize::ItemIndex intersect(uint32_t id, sserialize::spatial::GeoShapeType t, const sserialize::ItemIndex & idx) const;
};

}}//end spatial/Static namespaces

template<>
struct SerializationInfo<sserialize::Static::spatial::HierarchyCompleter> {
	static const bool is_fixed_length = false;
	static const OffsetType length = 0;
	static const OffsetType max_length = 0;
	static const OffsetType min_length = 0;
	static OffsetType sizeInBytes(const sserialize::Static::spatial::HierarchyCompleter & value) {
		return value.getSizeInBytes();
	}
};

}//end namespace



#endif