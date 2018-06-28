#include <sserialize/Static/CQRDilator.h>
#include <unordered_set>

namespace sserialize {
namespace Static {

CQRDilator::CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg) :
m_priv(new detail::CQRDilator(d, tg))
{}

CQRDilator::CQRDilator(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg) :
m_priv(new detail::CQRDilator(cd, tg))
{}

CQRDilator::~CQRDilator() {}

//dilating TreedCQR doesn't make any sense, since we need the real result with it
sserialize::ItemIndex CQRDilator::dilate(const sserialize::CellQueryResult& src, double diameter, uint32_t threadCount) const {
	struct MyIterator {
		sserialize::CellQueryResult::const_iterator m_it;
		inline bool operator!=(const MyIterator & other) { return m_it != other.m_it; }
		inline bool operator==(const MyIterator & other) { return m_it == other.m_it; }
		inline MyIterator & operator++() { ++m_it; return *this; }
		inline uint32_t operator*() { return m_it.cellId(); }
		MyIterator(const sserialize::CellQueryResult::const_iterator & it) : m_it(it) {}
	};
	return m_priv->dilate<MyIterator>(MyIterator(src.begin()), MyIterator(src.end()), diameter, threadCount);
}

sserialize::ItemIndex CQRDilator::dilate(const sserialize::ItemIndex& src, double diameter, uint32_t threadCount) const {
	return m_priv->dilate<sserialize::ItemIndex::const_iterator>(src.cbegin(), src.cend(), diameter, threadCount);
}

namespace detail {

CQRDilator::CQRDilator(const CellCenters & d, const sserialize::Static::spatial::TracGraph & tg) :
m_cd(new sserialize::Static::spatial::CellDistanceByCellCenter(d)),
m_tg(tg)
{}

CQRDilator::CQRDilator(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg) :
m_cd(cd),
m_tg(tg)
{}

CQRDilator::~CQRDilator() {}

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp1, const sserialize::Static::spatial::GeoPoint& gp2) const {
	return std::abs<double>( sserialize::spatial::distanceTo(gp1.lat(), gp1.lon(), gp2.lat(), gp2.lon()) );
}

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp, uint32_t cellId) const {
	return m_cd->distance(gp, cellId);
}

double CQRDilator::distance(uint32_t cellId1, uint32_t cellId2) const {
	return m_cd->distance(cellId1, cellId2);
}


}}}//end namespace sserialize::Static::detail
