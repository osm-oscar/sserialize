#pragma once

#include <sserialize/search/StringCompleter.h>

#include <sserialize/spatial/dgg/HCQR.h>

namespace sserialize::spatial::dgg::interface {

class HCQRIndex: public sserialize::RefCountObject {
public:
    using Self = HCQRIndex;
    using HCQRPtr = sserialize::spatial::dgg::interface::HCQR::HCQRPtr;
    using SpatialGrid = sserialize::spatial::dgg::interface::SpatialGrid;
    using SpatialGridInfo = sserialize::spatial::dgg::interface::SpatialGridInfo;
public:
    HCQRIndex() {}
    virtual ~HCQRIndex() {}
public:
    virtual sserialize::StringCompleter::SupportedQuerries getSupportedQueries() const = 0;

	virtual HCQRPtr complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
	virtual HCQRPtr items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
	virtual HCQRPtr regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const = 0;
public:
	virtual HCQRPtr cell(uint32_t cellId) const = 0;
	virtual HCQRPtr region(uint32_t regionId) const = 0;
public:
	virtual SpatialGridInfo const & sgi() const = 0;
	virtual SpatialGrid const & sg() const = 0;
};


}//end namespace sserialize::spatial::dgg::interface
