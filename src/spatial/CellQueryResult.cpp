#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/CellQueryResultPrivate.h>
#include <sserialize/mt/ThreadPool.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/iterator/RangeGenerator.h>

namespace sserialize {
namespace detail {

CellQueryResultIterator::CellQueryResultIterator(const RCPtrWrapper<detail::CellQueryResult> & cqr, uint32_t pos) :
m_d(cqr),
m_pos(pos)
{}

CellQueryResultIterator::CellQueryResultIterator(const CellQueryResultIterator & other) : 
m_d(other.m_d),
m_pos(other.m_pos)
{}

CellQueryResultIterator::~CellQueryResultIterator() {}

CellQueryResultIterator & CellQueryResultIterator::operator=(const CellQueryResultIterator & other) {
	m_d = other.m_d;
	m_pos = other.m_pos;
	return *this;
}

const sserialize::ItemIndex & CellQueryResultIterator::operator*() const {
	return m_d->idx(m_pos);
}

uint32_t CellQueryResultIterator::cellId() const {
	return m_d->cellId(m_pos);
}

bool CellQueryResultIterator::fullMatch() const {
	return m_d->fullMatch(m_pos);
}

//TODO:improve this if more information from the text-search is available
uint32_t CellQueryResultIterator::idxSize() const {
	return m_d->idxSize(m_pos);
}

uint32_t CellQueryResultIterator::idxId() const {
	return m_d->idxId(m_pos);
}

sserialize::ItemIndex CellQueryResultIterator::items() const {
	return m_d->items(m_pos);
}

uint32_t CellQueryResultIterator::rawDesc() const {
	return m_d->rawDesc(m_pos);
}

bool CellQueryResultIterator::fetched() const {
	return m_d->fetched(m_pos);
}

CellQueryResultIterator CellQueryResultIterator::operator++(int ) {
	++m_pos;
	return CellQueryResultIterator(m_d, m_pos-1);
}

CellQueryResultIterator & CellQueryResultIterator::operator++() {
	++m_pos;
	return *this;
}

CellQueryResultIterator CellQueryResultIterator::operator+(difference_type v) const {
	SSERIALIZE_CHEAP_ASSERT(v >= 0);
	return CellQueryResultIterator(m_d, m_pos+(uint32_t)v);
}

}//end namespace detail


CellQueryResult::CellQueryResult(detail::CellQueryResult * priv) :
m_priv(priv)
{}

CellQueryResult::CellQueryResult() :
m_priv(new detail::CellQueryResult())
{}

CellQueryResult::CellQueryResult(const CellInfo & ci, const ItemIndexStore & idxStore, int flags) :
m_priv(new detail::CellQueryResult(ci, idxStore, flags))
{}

CellQueryResult::CellQueryResult(const ItemIndex & fullMatches, const CellInfo & ci, const ItemIndexStore & idxStore, int flags) : 
m_priv( new detail::CellQueryResult(fullMatches, ci, idxStore, flags) )
{}


CellQueryResult::CellQueryResult(
	bool fullMatch,
	uint32_t cellId,
	const CellInfo & ci,
	const ItemIndexStore & idxStore,
	uint32_t cellIdxId,
	int flags)
:
m_priv( new detail::CellQueryResult(fullMatch, cellId, ci, idxStore, cellIdxId, flags) )
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	const sserialize::CompactUintArray::const_iterator& partialMatchesItemsPtrBegin,
	const CellInfo & ci,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, ci, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	const sserialize::RLEStream& partialMatchesItemsPtrBegin,
	const CellInfo & ci,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, ci, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	std::vector< uint32_t >::const_iterator partialMatchesItemsPtrBegin,
	const CellInfo & ci,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags)
:
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesItemsPtrBegin, ci, idxStore, flags))
{}

CellQueryResult::CellQueryResult(
	const sserialize::ItemIndex& fullMatches,
	const sserialize::ItemIndex& partialMatches,
	std::vector<sserialize::ItemIndex>::const_iterator partialMatchesIdx,
	const CellQueryResult::CellInfo & ci,
	const sserialize::CellQueryResult::ItemIndexStore& idxStore,
	int flags) :
m_priv(new detail::CellQueryResult(fullMatches, partialMatches, partialMatchesIdx, ci, idxStore, flags))
{}

CellQueryResult::~CellQueryResult() {}

CellQueryResult::CellQueryResult(const CellQueryResult & other) : m_priv(other.m_priv) {}

CellQueryResult & CellQueryResult::operator=(const CellQueryResult & other) {
	m_priv = other.m_priv;
	return *this;
}

const CellQueryResult::CellInfo & CellQueryResult::cellInfo() const {
	return m_priv->cellInfo();
}

const CellQueryResult::ItemIndexStore& CellQueryResult::idxStore() const {
	return m_priv->idxStore();
}

int CellQueryResult::flags() const {
	return m_priv->flags();
}

uint32_t CellQueryResult::cellCount() const {
	return m_priv->cellCount();
}

bool CellQueryResult::hasHits() const {
	return cellCount() > 0;
}

uint32_t CellQueryResult::maxItems() const {
	return m_priv->maxItems();
}

int CellQueryResult::defaultIndexTypes() const {
	return m_priv->defaultIndexTypes();
}

uint32_t CellQueryResult::idxSize(uint32_t pos) const {
	return m_priv->idxSize(pos);
}

uint32_t CellQueryResult::cellId(uint32_t pos) const {
	return m_priv->cellId(pos);
}

sserialize::ItemIndex CellQueryResult::idx(uint32_t pos) const {
	return m_priv->idx(pos);
}

sserialize::ItemIndex CellQueryResult::items(uint32_t pos) const {
	return m_priv->items(pos);
}

uint32_t CellQueryResult::idxId(uint32_t pos) const {
	return m_priv->idxId(pos);
}

bool CellQueryResult::fetched(uint32_t pos) const {
	return m_priv->fetched(pos);
}

bool CellQueryResult::fullMatch(uint32_t pos) const {
	return m_priv->fullMatch(pos);
}

CellQueryResult CellQueryResult::operator/(const sserialize::CellQueryResult& o) const {
	return CellQueryResult(m_priv->intersect(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator+(const sserialize::CellQueryResult & o) const {
	return CellQueryResult(m_priv->unite(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator-(const CellQueryResult & o) const {
	return CellQueryResult(m_priv->diff(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::operator^(const CellQueryResult & o) const {
	return CellQueryResult(m_priv->symDiff(o.m_priv.priv()));
}

CellQueryResult CellQueryResult::allToFull() const {
	return CellQueryResult(m_priv->allToFull());
}

CellQueryResult CellQueryResult::removeEmpty() const {
	return CellQueryResult(m_priv->removeEmpty());
}

bool CellQueryResult::operator!=(const CellQueryResult& other) const {
	return !CellQueryResult::operator==(other);
}

bool CellQueryResult::operator==(const CellQueryResult& other) const {
	if (cellCount() != other.cellCount()) {
		return false;
	}
	if (flags() != other.flags()) {
		return false;
	}
	for(uint32_t i(0), s(cellCount()); i < s; ++i) {
		if (cellId(i) != other.cellId(i)) {
			return false;
		}
		if (fullMatch(i) != other.fullMatch(i)) {
			return false;
		}
		if (!fullMatch(i)) {
			if (!fetched(i) && !other.fetched(i)) {
				if (idxId(i) != other.idxId(i)) {
					return false;
				}
			}
			else if (idx(i) != other.idx(i)) {
				return false;
			}
		}
	}
	return true;
}

CellQueryResult::const_iterator CellQueryResult::begin() const {
	return const_iterator(m_priv, 0);
}

CellQueryResult::const_iterator CellQueryResult::cbegin() const {
	return const_iterator(m_priv, 0);
}

CellQueryResult::const_iterator CellQueryResult::end() const {
	return const_iterator(m_priv, m_priv->cellCount());
}

CellQueryResult::const_iterator CellQueryResult::cend() const {
	return const_iterator(m_priv, m_priv->cellCount());
}

sserialize::ItemIndex CellQueryResult::flaten(uint32_t threadCount) const {
	if (!cellCount()) {
		return sserialize::ItemIndex();
	}
	if (uint64_t(5) * cellCount() > cellInfo()->cellSize()) {
		if (threadCount == 1) {
			DynamicBitSet bitset;
			for(uint32_t i(0), s(cellCount()); i < s; ++i) {
				items(i).putInto(bitset);
			}
			return bitset.toIndex(idxStore().indexTypes(), sserialize::ItemIndex::CL_LOW);
		}
		else {
			struct State {
				std::vector<DynamicBitSet> intermediates;
				std::mutex lock;
				std::atomic<uint32_t> i{0};
				uint32_t cellCount;
				const CellQueryResult * that;
			} state;
			state.intermediates.reserve(threadCount);
			state.cellCount = cellCount();
			state.that = this;
			
			struct Worker {
				DynamicBitSet bitset;
				State * state;
				Worker(State * state) : state(state) {}
				Worker(const Worker & other) : state(other.state) {}
				void operator()() {
					while(true) {
						uint32_t i = state->i.fetch_add(1, std::memory_order_relaxed);
						if (i >= state->cellCount) {
							break;
						}
						state->that->items(i).putInto(bitset);
					}
					std::unique_lock<std::mutex> lck(state->lock, std::defer_lock);
					while(true) {
						lck.lock();
						if (!state->intermediates.size()) {
							state->intermediates.emplace_back(std::move(bitset));
							break;
						}
						DynamicBitSet other( std::move(state->intermediates.back()) );
						state->intermediates.pop_back();
						lck.unlock();
						bitset |= other;
					}
				};
			};
			sserialize::ThreadPool::execute(Worker(&state), threadCount, ThreadPool::CopyTaskTag());
			SSERIALIZE_CHEAP_ASSERT_EQUAL(std::size_t(1), state.intermediates.size());
			return state.intermediates.front().toIndex(idxStore().indexTypes(), sserialize::ItemIndex::CL_LOW);
		}
	}
	else {
		auto redfunc = [](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex { return a + b; };
		sserialize::RangeGenerator<uint32_t> rg(0, cellCount(), 1);
		auto mapfunc = [this](uint32_t pos) { return this->items(pos); };
		return sserialize::treeReduceMap<sserialize::RangeGenerator<uint32_t>::const_iterator, sserialize::ItemIndex>(rg.cbegin(), rg.cend(), redfunc, mapfunc, threadCount);
	}
}

ItemIndex CellQueryResult::topK(uint32_t numItems) const {
	if ((flags() & FF_CELL_LOCAL_ITEM_IDS) != 0) {
		throw sserialize::UnimplementedFunctionException("CellQueryResult::topK: cell local ids are not supported yet");	
	}
	auto func = [numItems](const sserialize::ItemIndex & a, const sserialize::ItemIndex & b) -> sserialize::ItemIndex {
		return sserialize::ItemIndex::uniteK(a, b, numItems);
	};
	return sserialize::treeReduce<const_iterator, sserialize::ItemIndex>(cbegin(), cend(), func);
}

CellQueryResult CellQueryResult::toGlobalItemIds(uint32_t threadCount) const {
	SSERIALIZE_CHEAP_ASSERT(flags() & FF_CELL_LOCAL_ITEM_IDS);
	return CellQueryResult( m_priv->toGlobalItemIds(threadCount) );
}

CellQueryResult CellQueryResult::toCellLocalItemIds(uint32_t threadCount) const {
	SSERIALIZE_CHEAP_ASSERT(flags() & FF_CELL_GLOBAL_ITEM_IDS);
	return CellQueryResult( m_priv->toCellLocalItemIds(threadCount) );
}

CellQueryResult CellQueryResult::convert(int flags, uint32_t threadCount) const {
	if ((flags & FF_CELL_GLOBAL_ITEM_IDS) && (this->flags() & FF_CELL_LOCAL_ITEM_IDS)) {
		return toGlobalItemIds(threadCount);
	}
	else if ((flags & FF_CELL_LOCAL_ITEM_IDS) && (this->flags() & FF_CELL_GLOBAL_ITEM_IDS)) {
		return toCellLocalItemIds(threadCount);
	}
	return *this;
}

void CellQueryResult::dump(std::ostream & out) const {
	out << "CQR<" << cellCount();
	if (flags() & FF_CELL_LOCAL_ITEM_IDS) {
		out << ", local ids";
	}
	else {
		out << ", global ids";
	}
	out << ">";
	if (!cellCount()) {
		out << "{}";
		return;
	}
	char sep = '{';
	for(const_iterator it(cbegin()), end(cend()); it != end; ++it) {
		out << sep << ' ';
		out << it.cellId() << ":";
		if (it.fullMatch()) {
			out << "f";
		}
		else {
			out << *it;
		}
		sep = ',';
	}
	out << " }";
	return;
}

void CellQueryResult::dump() const {
	this->dump(std::cout);
	std::cout << std::endl;
}

ItemIndex CellQueryResult::cells() const {
	return m_priv->cells();
}


sserialize::spatial::GeoRect CellQueryResult::boundary() const {
	return m_priv->boundary();
}

bool CellQueryResult::selfCheck() {
	return m_priv->selfCheck();
}

std::ostream& operator<<(std::ostream& out, const CellQueryResult& src) {
	src.dump(out);
	return out;
}

}
