#pragma once

#include <sserialize/containers/LFUCache.h>

#include <sserialize/spatial/dgg/HCQRIndex.h>

namespace sserialize::spatial::dgg::detail::HCQRIndexWithCache {

struct CacheKey {
    enum {ITEMS_AND_REGIONS, ITEMS, REGIONS, CELL, REGION} ItemType;
    CacheKey(uint8_t itemType, uint8_t qt, std::string const & qstr) :
    itemType(itemType), qt(qt), qstr(qstr)
    {}
    CacheKey(CacheKey const &) = default;
    CacheKey(CacheKey &&) = default;
	inline bool operator==(CacheKey const & other) const {
		return (itemType == other.itemType) && (qt == other.qt) && (qstr == other.qstr);
	}
    uint8_t itemType;
    uint8_t qt;
    std::string qstr;
}; 

} //end namespace sserialize::spatial::dgg::detail::HCQRIndexWithCache


namespace std {

template<>
struct hash<sserialize::spatial::dgg::detail::HCQRIndexWithCache::CacheKey> {
    std::hash<std::string> hash;
	inline std::size_t operator()(sserialize::spatial::dgg::detail::HCQRIndexWithCache::CacheKey const & v) const {
		std::size_t seed = std::size_t(v.itemType) << 8 | v.qt;
		::hash_combine(seed, v.qstr, hash);
		return seed;
	}
};

} //end namespace std

namespace sserialize::spatial::dgg {

class HCQRIndexWithCache: public sserialize::spatial::dgg::interface::HCQRIndex {
public:
    using HCQRIndexPtr = sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::HCQRIndex>;
public:
    HCQRIndexWithCache(HCQRIndexPtr const & base, uint32_t cacheSize = 10);
    ~HCQRIndexWithCache() override;
	static HCQRIndexPtr make(HCQRIndexPtr const & base, uint32_t cacheSize = 10);
public:
    void setCacheSize(uint32_t size);
public:
    sserialize::StringCompleter::SupportedQuerries getSupportedQueries() const override;
public:
	HCQRPtr complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
	HCQRPtr regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const override;
public:
	HCQRPtr cell(uint32_t cellId) const override;
	HCQRPtr region(uint32_t regionId) const override;
public:
	SpatialGridInfo const & sgi() const override;
	SpatialGrid const & sg() const override;
private:
    using CacheKey = sserialize::spatial::dgg::detail::HCQRIndexWithCache::CacheKey;
    using Cache = sserialize::LFUCache<CacheKey, HCQRPtr>;
private:
    HCQRIndexPtr m_base;
    mutable std::mutex m_cacheLock;
    mutable Cache m_cache;
};

}//end namespace sserialize::spatial::dgg
