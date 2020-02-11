#include <sserialize/spatial/dgg/Static/SpatialGridRegistry.h>
#include <mutex>

namespace sserialize::spatial::dgg::Static {

SpatialGridRegistry & SpatialGridRegistry::get() {
	static SpatialGridRegistry s;
	return s;
}

void SpatialGridRegistry::set(TypeId const & typeId, Factory f) {
	std::unique_lock<MutexType> l(m_l);
	m_d[typeId] = f;
}

void SpatialGridRegistry::remove(TypeId const & typeId) {
	std::unique_lock<MutexType> l(m_l);
	m_d.erase(typeId);
}

SpatialGridRegistry::SpatialGridPtr SpatialGridRegistry::get(SpatialGridInfo const & info) const {
	std::shared_lock<MutexType> l(m_l);
	return m_d.at(info.type())(info);
}

SpatialGridRegistry::SpatialGridRegistry() {}

SpatialGridRegistry::~SpatialGridRegistry() {}
	
}//end namespace sserialize::spatial::dgg::Static
