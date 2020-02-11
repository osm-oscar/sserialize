#include <sserialize/spatial/dgg/Static/HCQRCellInfo.h>


namespace sserialize::spatial::dgg::Static {

HCQRCellInfo::HCQRCellInfo(sserialize::Static::ItemIndexStore const & idxStore, std::shared_ptr<SpatialGridInfo> const & sgi) :
m_idxStore(idxStore),
m_sgi(sgi)
{}

HCQRCellInfo::~HCQRCellInfo() {}

HCQRCellInfo::SpatialGrid::Level
HCQRCellInfo::level() const {
	return m_sgi->levels();
}

bool
HCQRCellInfo::hasPixel(PixelId pid) const {
	return m_sgi->hasSgIndex(pid);
}

HCQRCellInfo::ItemIndex
HCQRCellInfo::items(PixelId pid) const {
	SSERIALIZE_CHEAP_ASSERT(hasPixel(pid));
	try {
		return m_idxStore.at(
			m_sgi->itemIndexId(
				m_sgi->cPixelId(pid)
			)
		);
	}
	catch (sserialize::OutOfBoundsException const &) {
		return ItemIndex();
	}
}

HCQRCellInfo::PixelId
HCQRCellInfo::pixelId(CompressedPixelId const & cpid) const {
	return m_sgi->sgIndex(cpid.value());
}

std::vector<HCQRCellInfo::CompressedPixelId>
HCQRCellInfo::cells() const
{
	sserialize::RangeGenerator<uint32_t> range(0, m_sgi->cPixelCount());
	return std::vector<CompressedPixelId>(range.begin(), range.end());
}

}//end namespace sserialize::spatial::dgg::Static