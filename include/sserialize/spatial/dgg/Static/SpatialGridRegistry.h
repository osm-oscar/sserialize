#pragma once

#include <sserialize/spatial/dgg/SpatialGrid.h>
#include <sserialize/spatial/dgg/Static/SpatialGridInfo.h>
#include <map>
#include <shared_mutex>

namespace sserialize::spatial::dgg::Static {
	
class SpatialGridRegistry final {
public:
	using SpatialGridPtr = sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid>;
	using SpatialGridInfo = sserialize::spatial::dgg::Static::SpatialGridInfo;
	using TypeId = typename ssinfo::SpatialGridInfo::Types<SpatialGridInfo::MetaData::DataMembers::type>::type;
public:
	using Factory = std::function<SpatialGridPtr(SpatialGridInfo const&)>;
public:
	static SpatialGridRegistry & get();
public:
	void set(TypeId const & typeId, Factory f);
	void remove(TypeId const & typeId);
	SpatialGridPtr get(SpatialGridInfo const & info) const;
private:
	using MutexType = std::shared_mutex;
private:
	SpatialGridRegistry();
	~SpatialGridRegistry();
private:
	std::map<TypeId, Factory> m_d;
	mutable MutexType m_l;
};
	
}//end namespace sserialize::spatial::dgg::Static
