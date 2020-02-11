#pragma once

#include <sserialize/spatial/dgg/HCQRIndexFromCellIndex.h>
#include <sserialize/spatial/dgg/Static/SpatialGridInfo.h>


namespace sserialize::spatial::dgg::Static {

class HCQRCellInfo: public sserialize::spatial::dgg::detail::HCQRIndexFromCellIndex::interface::CellInfo {
public:
	using SpatialGrid = sserialize::spatial::dgg::interface::SpatialGrid;
public:
	HCQRCellInfo(sserialize::Static::ItemIndexStore const & idxStore, std::shared_ptr<SpatialGridInfo> const & sgi);
	~HCQRCellInfo() override;
public:
    SpatialGrid::Level level() const override;
public:
    bool hasPixel(PixelId pid) const override;
    ItemIndex items(PixelId pid) const override;
	PixelId pixelId(CompressedPixelId const & cpid) const override;
public:
	std::vector<CompressedPixelId> cells() const override;
private:
	sserialize::Static::ItemIndexStore m_idxStore;
	std::shared_ptr<SpatialGridInfo> m_sgi;
};

}//end namespace sserialize::spatial::dgg::Static