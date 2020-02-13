#pragma once

#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/Static/Map.h>
#include <sserialize/spatial/dgg/SpatialGrid.h>

namespace sserialize::spatial::dgg::Static {


/**
 *  struct SpatialGridInfo: Version(3) {
 *      std::string type;
 *      uint<8> levels;
 *      sserialize::BoundedCompactUintArray trixelId2HtmIndexId;
 *      sserialize::Static::Map<uint64_t, uint32_t> htmIndexId2TrixelId; 
 *      sserialize::BoundedCompactUintArray trixelItemIndexIds;
 *  };
 **/

namespace ssinfo {
namespace SpatialGridInfo {
	class Data;
	
    class MetaData final {
	public: 
		enum class DataMembers : int {
			type,
			levels,
			trixelId2HtmIndexId,
			htmIndexId2TrixelId,
			trixelItemIndexIds
		};
        static constexpr uint8_t version{3};
	public:
		MetaData(Data const * d) : m_d(d) {}
	public:
		sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
		sserialize::UByteArrayAdapter::SizeType offset(DataMembers member) const;
	private:
		Data const * m_d;
    };
	
	template<MetaData::DataMembers TMember>
	struct Types;
	
	template<> struct Types<MetaData::DataMembers::type>
	{ using type = std::string; };
	
	template<> struct Types<MetaData::DataMembers::levels>
	{ using type = uint8_t; };
	
	template<> struct Types<MetaData::DataMembers::trixelId2HtmIndexId>
	{ using type = sserialize::BoundedCompactUintArray; };
	
	template<> struct Types<MetaData::DataMembers::htmIndexId2TrixelId>
	{ using type = sserialize::Static::Map<uint64_t, uint32_t>; };
	
	template<> struct Types<MetaData::DataMembers::trixelItemIndexIds>
	{ using type = sserialize::BoundedCompactUintArray; };
	
	class Data: sserialize::RefCountObject {
	public:
		Data(sserialize::UByteArrayAdapter d);
		~Data() {}
	public:
		inline Types<MetaData::DataMembers::type>::type const & type() const { return m_type; }
		inline Types<MetaData::DataMembers::levels>::type const & levels() const { return m_levels; }
		inline Types<MetaData::DataMembers::trixelId2HtmIndexId>::type const & trixelId2HtmIndexId() const { return m_trixelId2HtmIndexId; }
		inline Types<MetaData::DataMembers::htmIndexId2TrixelId>::type const & htmIndexId2TrixelId() const { return m_htmIndexId2TrixelId; }
		inline Types<MetaData::DataMembers::trixelItemIndexIds>::type const & trixelItemIndexIds() const { return m_trixelItemIndexIds; }
	private:
		Types<MetaData::DataMembers::type>::type m_type;
		Types<MetaData::DataMembers::levels>::type m_levels;
		Types<MetaData::DataMembers::trixelId2HtmIndexId>::type m_trixelId2HtmIndexId;
		Types<MetaData::DataMembers::htmIndexId2TrixelId>::type m_htmIndexId2TrixelId;
		Types<MetaData::DataMembers::trixelItemIndexIds>::type m_trixelItemIndexIds;
	};
} //end namespace SpatialGridInfo
}//end namespace ssinfo

class SpatialGridInfo final {
public:
	using SizeType = uint32_t;
	using ItemIndexId = uint32_t;
	using CPixelId = uint32_t; //compressed pixel id
	using SGPixelId = sserialize::spatial::dgg::interface::SpatialGrid::PixelId;
public:
	using MetaData = ssinfo::SpatialGridInfo::MetaData;
	
public:
	SpatialGridInfo(const sserialize::UByteArrayAdapter & d);
	~SpatialGridInfo();
	MetaData metaData() const;
public:
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
public:
	auto type() const { return m_d.type(); }
	int levels() const;
	SizeType cPixelCount() const;
public:
	ItemIndexId itemIndexId(CPixelId cPixelId) const;
public:
	CPixelId cPixelId(SGPixelId sgIndex) const;
	bool hasSgIndex(SGPixelId sgIndex) const;
	SGPixelId sgIndex(CPixelId cPixeld) const;
private:
	ssinfo::SpatialGridInfo::Data m_d;
};

}//end namespace
