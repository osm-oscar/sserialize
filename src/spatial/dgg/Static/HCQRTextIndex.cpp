#include <sserialize/spatial/dgg/Static/HCQRTextIndex.h>
#include <sserialize/spatial/dgg/StaticHCQRSpatialGrid.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/Static/Version.h>
#include <sserialize/Static/CellTextCompleter.h>

namespace sserialize::spatial::dgg::Static {
namespace detail::HCQRTextIndex {
	

Payload::Payload() :
m_types(QuerryType::QT_NONE)
{}

Payload::Payload(sserialize::UByteArrayAdapter const & d) :
m_types(d.at(0)),
m_d(d+1)
{}

Payload::~Payload() {}

Payload::QuerryType
Payload::types() const {
	return QuerryType(m_types & 0xF);
}

Payload::Type
Payload::type(QuerryType qt) const {
	return typeData(qt);
}

sserialize::UByteArrayAdapter
Payload::typeData(QuerryType qt) const {
	qt = sserialize::StringCompleter::toAvailable(qt, types());
	if (qt == sserialize::StringCompleter::QT_NONE) {
		throw sserialize::OutOfBoundsException("OscarSearchSgIndex::typeFromCompletion");
	}
	uint32_t pos = sserialize::Static::CellTextCompleter::Payload::qt2Pos(qt, types());
	sserialize::UByteArrayAdapter d(m_d);
	for(uint32_t i(0); i < pos; ++i) {
		sserialize::spatial::dgg::Static::detail::HCQRSpatialGrid::Tree::MetaData md(d);
		d += md.StorageSize + md.dataSize();
	}
	return d;
}


void
CompactNode::create(sserialize::spatial::dgg::impl::HCQRSpatialGrid::TreeNode const & src, sserialize::MultiBitBackInserter & dest) {
	if (src.isFullMatch()) {
		dest.push_back(1, 1);
	}
	else {
		dest.push_back(0, 1);
	}
	auto pixelIdBits = sserialize::fastLog2(src.pixelId());
	dest.push_back(pixelIdBits, 6);
	dest.push_back(src.pixelId(), pixelIdBits);
	
	if (!src.isFullMatch()) {
		auto idxIdBits = sserialize::fastLog2(src.itemIndexId());
		dest.push_back(idxIdBits, 5);
		dest.push_back(src.itemIndexId(), idxIdBits);
	}
}

CompactNode::CompactNode(sserialize::UByteArrayAdapter const & d) {
	sserialize::MultiBitIterator it(d);
	m_fm = it.get16(1);
	it += 1;
	uint32_t pixelIdBits = it.get16(6);
	it += 6;
	m_pid = it.get<PixelId>(pixelIdBits);
	it += pixelIdBits;
	if (isPartialMatch()) {
		uint32_t itemIndexIdBits = it.get16(5);
		m_itemIndexId = it.get<ItemIndexId>(itemIndexIdBits);
	}
	
}

CompactNode::~CompactNode() {}

sserialize::UByteArrayAdapter::SizeType CompactNode::getSizeInBytes() const {
	sserialize::UByteArrayAdapter::SizeType numBits = 1+6;
	numBits += sserialize::fastLog2(pixelId());
	if (isPartialMatch()) {
		numBits += 5+sserialize::fastLog2(itemIndexId());
	}
	return numBits/8+sserialize::UByteArrayAdapter::SizeType(numBits%8 != 0);
}

bool CompactNode::isFullMatch() const {
	return m_fm;
}

bool CompactNode::isPartialMatch() const {
	return !isFullMatch();
}

CompactNode::SourceNode::PixelId CompactNode::pixelId() const {
	return m_pid;
}

sserialize::Static::ItemIndexStore::IdType CompactNode::itemIndexId() const {
	return m_itemIndexId;
}

CompactTree::CompactTree(sserialize::UByteArrayAdapter const & d) : m_d(d) {}

CompactTree::~CompactTree() {}


uint32_t CompactTree::nodeCount() const {
	return m_d.getVlPackedUint32(0);
}

CompactTree::HCQRSpatialGrid::TreeNodePtr CompactTree::tree(SpatialGrid const & sg) const {
	sserialize::UByteArrayAdapter d(m_d);
	uint32_t nc = d.getVlPackedUint32();
	d.shrinkToGetPtr();
	std::unordered_map<HCQRSpatialGrid::PixelId, HCQRSpatialGrid::TreeNodePtr> nodes;
	for(uint32_t i(0), s(nc); i < s; ++i) {
		CompactNode n(d);
		d += n.getSizeInBytes();
		HCQRSpatialGrid::TreeNode::make_unique(
			n.pixelId(),
			(n.isFullMatch() ? HCQRSpatialGrid::TreeNode::IS_FULL_MATCH : HCQRSpatialGrid::TreeNode::IS_PARTIAL_MATCH),
			n.itemIndexId()
		);
	}
	//leaf nodes are in queue, construct the tree
	while(nodes.size() > 1) {
		decltype(nodes) tmp;
		for(auto & x : nodes) {
			auto pid = sg.parent(x.first);
			if (!tmp.count(pid)) {
				tmp[pid] = HCQRSpatialGrid::TreeNode::make_unique(pid, HCQRSpatialGrid::TreeNode::IS_INTERNAL);
			}
			tmp.at(pid)->children().push_back(std::move(x.second));
		}
		
		std::swap(tmp, nodes);
		for(auto & x: nodes) {
			x.second->sortChildren();
		}
	}
	return std::move(nodes.at(sg.rootPixelId()));
}
	
}//end namespace detail::HCQRTextIndex
	
sserialize::RCPtrWrapper<HCQRTextIndex>
HCQRTextIndex::make(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore) {
	return sserialize::RCPtrWrapper<HCQRTextIndex>( new HCQRTextIndex(d, idxStore) );
}

HCQRTextIndex::~HCQRTextIndex()
{}

sserialize::UByteArrayAdapter::SizeType
HCQRTextIndex::getSizeInBytes() const {
	return 2+m_sgInfo->getSizeInBytes()+m_trie.getSizeInBytes()+m_mixed.getSizeInBytes()+m_items.getSizeInBytes()+m_regions.getSizeInBytes();
}

sserialize::Static::ItemIndexStore const &
HCQRTextIndex::idxStore() const {
	return m_idxStore;
}

int
HCQRTextIndex::flags() const {
	return m_flags;
}

std::ostream &
HCQRTextIndex::printStats(std::ostream & out) const {
	out << "HCQRTextIndex::BEGIN_STATS" << std::endl;
	m_trie.printStats(out);
	out << "HCQRTextIndex::END_STATS" << std::endl;
	return out;
	return out;
}

sserialize::StringCompleter::SupportedQuerries
HCQRTextIndex::getSupportedQueries() const {
	return sserialize::StringCompleter::SupportedQuerries(m_sq);
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
    try {
		Payload::Type t(typeFromCompletion(qstr, qt, m_mixed));
		return hcqrFromPayload(t);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return HCQRPtr();
	}
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
    try {
		Payload::Type t(typeFromCompletion(qstr, qt, m_items));
		return hcqrFromPayload(t);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return HCQRPtr();
	}
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
    try {
		Payload::Type t(typeFromCompletion(qstr, qt, m_regions));
		return hcqrFromPayload(t);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return HCQRPtr();
	}
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::cell(uint32_t cellId) const {
	throw sserialize::UnimplementedFunctionException("OscarSearchSgIndex::cell");
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::region(uint32_t regionId) const {
	throw sserialize::UnimplementedFunctionException("OscarSearchSgIndex::region");
}
	
HCQRTextIndex::SpatialGridInfo const &
HCQRTextIndex::sgInfo() const {
	return *m_sgInfo;
}

std::shared_ptr<HCQRTextIndex::SpatialGridInfo> const &
HCQRTextIndex::sgInfoPtr() const {
	return m_sgInfo;
}

sserialize::spatial::dgg::interface::SpatialGrid const &
HCQRTextIndex::sg() const {
	return *m_sg;
}

sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> const &
HCQRTextIndex::sgPtr() const {
	return m_sg;
}


sserialize::spatial::dgg::interface::SpatialGridInfo const &
HCQRTextIndex::sgi() const {
	return *m_sgi;
}

sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> const &
HCQRTextIndex::sgiPtr() const {
	return m_sgi;
}

HCQRTextIndex::HCQRTextIndex(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore) :
m_sq(sserialize::Static::ensureVersion(d, MetaData::version, d.at(0)).at(1)),
m_sgInfo( std::make_shared<SpatialGridInfo>(d+2) ),
m_trie(d+(2+sgInfo().getSizeInBytes())),
m_mixed(d+(2+sgInfo().getSizeInBytes()+m_trie.getSizeInBytes())),
m_regions(d+(2+sgInfo().getSizeInBytes()+m_trie.getSizeInBytes()+m_mixed.getSizeInBytes())),
m_items(d+(2+sgInfo().getSizeInBytes()+m_trie.getSizeInBytes()+m_mixed.getSizeInBytes()+m_regions.getSizeInBytes())),
m_idxStore(idxStore)
{
	using SpatialGridInfoImp = sserialize::spatial::dgg::detail::HCQRIndexFromCellIndex::impl::SpatialGridInfoFromCellIndexWithIndex;

	auto cellInfoPtr = sserialize::RCPtrWrapper<HCQRCellInfo>( new HCQRCellInfo(this->idxStore(), sgInfoPtr()) );
	m_sgi.reset( new SpatialGridInfoImp(sgPtr(), cellInfoPtr) );
}

HCQRTextIndex::Payload::Type
HCQRTextIndex::typeFromCompletion(const std::string& qs, sserialize::StringCompleter::QuerryType qt, Payloads const & pd) const {
	std::string qstr;
	if (m_sq & sserialize::StringCompleter::SQ_CASE_INSENSITIVE) {
		qstr = sserialize::unicode_to_lower(qs);
	}
	else {
		qstr = qs;
	}
	auto pos = m_trie.find(qstr, (qt & sserialize::StringCompleter::QT_SUBSTRING || qt & sserialize::StringCompleter::QT_PREFIX));
	
	if (pos == m_trie.npos) {
		throw sserialize::OutOfBoundsException("HCQRTextIndex::typeFromCompletion");
	}
	return pd.at(pos).type(qt);
}

HCQRTextIndex::HCQRPtr
HCQRTextIndex::hcqrFromPayload(Payload::Type const & d) const {
	if (m_payloadFlags & PayloadFlags::FULL_TREE) {
		using MyHCQR = sserialize::spatial::dgg::Static::impl::HCQRSpatialGrid;
		return HCQRPtr( new MyHCQR(d, idxStore(), sgPtr(), sgiPtr()) );
	}
	else {
		using MyHCQR = sserialize::spatial::dgg::impl::HCQRSpatialGrid;
		detail::HCQRTextIndex::CompactTree ctree(d);
		auto rn = ctree.tree();
		return HCQRPtr( new MyHCQR(std::move(rn), idxStore(), sgPtr(), sgiPtr()) );
	}
}

}//end namespace sserialize::spatial::dgg::Static
