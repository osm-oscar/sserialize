#pragma once
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/spatial/dgg/SpatialGrid.h>

namespace sserialize::spatial::dgg::interface {

class HCQR: public sserialize::RefCountObject {
public:
    using Self = HCQR;
    using SizeType = uint32_t;
    using HCQRPtr = sserialize::RCPtrWrapper<Self>;
    using ItemIndex = sserialize::ItemIndex;
public:
    HCQR();
    virtual ~HCQR();
public:
    virtual SizeType depth() const = 0;
	virtual SizeType numberOfNodes() const = 0;
    virtual SizeType numberOfItems() const = 0;
    virtual ItemIndex items() const = 0;
public:
    virtual HCQRPtr operator/(HCQR const & other) const = 0;
    virtual HCQRPtr operator+(HCQR const & other) const = 0;
    virtual HCQRPtr operator-(HCQR const & other) const = 0;
public:
    ///@param maxPMLevel the highest level up to which merging of partial-match nodes should be considered
    ///note that the level of the root-node is 0.
    virtual HCQRPtr compactified(SizeType maxPMLevel) const = 0;
    ///param level up to which the tree should be expanded
    virtual HCQRPtr expanded(SizeType level) const = 0;
    virtual HCQRPtr allToFull() const = 0;
};

class SpatialGridInfo: public sserialize::RefCountObject {
public:
    using PixelId = SpatialGrid::PixelId;
	using CompressedPixelId = SpatialGrid::CompressedPixelId;
    using SizeType = uint32_t;
    using ItemIndex = sserialize::ItemIndex;
public:
    SpatialGridInfo() {}
    virtual ~SpatialGridInfo() {}
    virtual SizeType itemCount(PixelId pid) const = 0;
    virtual ItemIndex items(PixelId pid) const = 0;
	virtual PixelId pixelId(CompressedPixelId const & cpid) const = 0;
};

class HCQRSpatialGrid: public HCQR {
public:
	using PixelId = sserialize::spatial::dgg::interface::SpatialGridInfo::PixelId;
	using CompressedPixelId = sserialize::spatial::dgg::interface::SpatialGridInfo::CompressedPixelId;
	using ItemIndexId = uint32_t;
	using Parent = interface::HCQR;
	using Self = sserialize::spatial::dgg::interface::HCQRSpatialGrid;
	using PixelLevel = sserialize::spatial::dgg::interface::SpatialGrid::Level;
public:
	HCQRSpatialGrid(
		sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
		sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi);
	HCQRSpatialGrid(HCQRSpatialGrid const & other);
	HCQRSpatialGrid(HCQRSpatialGrid && other);
	~HCQRSpatialGrid() override;
public:
	auto const & sg() const { return *m_sg; }
	auto const & sgi() const { return *m_sgi; } 
	auto const & sgPtr() const { return m_sg; }
	auto const & sgiPtr() const { return m_sgi; }
	using Parent::items;
protected:
	std::vector<PixelId> pixelChildren(PixelId pid) const;
    sserialize::ItemIndex items(PixelId pid) const;
	PixelLevel level(PixelId pid) const;
private:
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> m_sg;
	sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> m_sgi;
};

}//end namespace sserialize::spatial::dgg::interface

