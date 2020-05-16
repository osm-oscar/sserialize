#include <sserialize/Static/CQRDilator.h>
#include <unordered_set>

namespace sserialize {
namespace Static {

CQRDilator::CQRDilator() {}

CQRDilator::CQRDilator(const sserialize::RCPtrWrapper<detail::CQRDilator> & other) :
m_priv( other )
{}

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
		inline bool operator!=(const MyIterator & other) const { return m_it != other.m_it; }
		inline bool operator==(const MyIterator & other) const { return m_it == other.m_it; }
		inline MyIterator & operator++() { ++m_it; return *this; }
		inline uint32_t operator*() const { return m_it.cellId(); }
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

std::unique_ptr<SingleCellDilatorInterface> CQRDilator::dilator(SingleCellDilatorInterface::State * state) const {
	return std::unique_ptr<SingleCellDilatorInterface>( new SingleCellDilator(state) );
}

CQRDilator::SingleCellDilator::SingleCellDilator(SingleCellDilatorInterface::State * state) :
SingleCellDilatorInterface(state)
{}

CQRDilator::SingleCellDilator::~SingleCellDilator()
{}

void CQRDilator::SingleCellDilator::dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) {
	auto node(state()->that->m_tg.node(cellId));
	//put neighbors into workqueue
	for(uint32_t i(0), s(node.neighborCount()); i < s; ++i) {
		uint32_t nId = node.neighborId(i);
		if (state()->isNotBaseCell(nId) && state()->that->distance(cellId, nId) < state()->diameter) {
			queue.push_back(nId);
			relaxed.insert(nId);
		}
	}
	//now explore the neighborhood
	while(queue.size()) {
		uint32_t oCellId = queue.back();
		queue.pop_back();
		//iterator over all neighbors
		auto tmpNode(state()->that->m_tg.node(oCellId));
		for(uint32_t i(0), s(tmpNode.neighborCount()); i < s; ++i) {
			uint32_t nId = tmpNode.neighborId(i);
			if (state()->isNotBaseCell(nId) && !relaxed.count(nId) && state()->that->distance(cellId, nId) < state()->diameter) {
				queue.push_back(nId);
				relaxed.insert(nId);
			}
		}
	}
	//push them to the output
	for(auto x : relaxed) {
		if (!dilatedMarks.isSet(x)) {
			dilated.push_back(x);
			dilatedMarks.set(x);
		}
	}
	relaxed.clear();
	queue.clear();
}

const CQRDilator * CQRDilator::SingleCellDilator::parent() const {
	return state()->that;
}

//BEGIN CQRDilatorWithCache
CQRDilatorWithCache::CQRDilatorWithCache(const CellCenters & d, const sserialize::Static::spatial::TracGraph & tg) :
CQRDilator(d, tg)
{}


CQRDilatorWithCache::CQRDilatorWithCache(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg) :
CQRDilator(cd, tg)
{}

CQRDilatorWithCache::~CQRDilatorWithCache()
{}

void CQRDilatorWithCache::populateCache(uint32_t threshold, uint32_t threadCount) {
	if (!threadCount) {
		threadCount = sserialize::ThreadPool::hardware_concurrency();
	}
	
	struct State: SingleCellDilatorInterface::State {
		CQRDilatorWithCache * that;
		
		uint32_t threshold;
		
		std::mutex itLock;
		uint32_t i;
		uint32_t s;
		
		std::mutex cacheLock;

		State(CQRDilatorWithCache * that, uint32_t threshold) :
		that(that),
		threshold(threshold),
		i(0),
		s(that->tracGraph().size())
		{}
	};
	
	State state(this, threshold);
	
	struct Worker {
		State * state;
		std::unordered_set<uint32_t> relaxed;
		std::vector<uint32_t> queue;
		std::vector<CQRDilatorWithCache::CacheEntry> ce;
		Worker(State * state) : state(state) {}
		Worker(const Worker & other) : Worker(other.state) {}
		void handle(uint32_t cellId) {
			auto node(state->that->tracGraph().node(cellId));
			//put neighbors into workqueue
			for(uint32_t i(0), s(node.neighborCount()); i < s; ++i) {
				uint32_t nId = node.neighborId(i);
				if (state->that->distance(cellId, nId) < state->threshold) {
					queue.push_back(nId);
					relaxed.insert(nId);
				}
			}
			//now explore the neighborhood
			while(queue.size()) {
				uint32_t oCellId = queue.back();
				queue.pop_back();
				//iterator over all neighbors
				auto tmpNode(state->that->tracGraph().node(oCellId));
				for(uint32_t i(0), s(tmpNode.neighborCount()); i < s; ++i) {
					uint32_t nId = tmpNode.neighborId(i);
					double dist = state->that->distance(cellId, nId);
					if (!relaxed.count(nId) && dist < state->threshold) {
						queue.push_back(nId);
						relaxed.insert(nId);
						ce.emplace_back(nId, dist);
					}
				}
			}
			std::sort(ce.begin(), ce.end(), [](const CacheEntry & a, const CacheEntry & b) {
				return (a.distance == b.distance ? a.cellId < b.cellId : a.distance < b.distance);
			});
			{
				std::lock_guard<std::mutex> lck(state->cacheLock);
				state->that->m_id2d.at(cellId) = state->that->m_cache.size();
				try {
					state->that->m_cache.insert(state->that->m_cache.end(), ce.begin(), ce.end());
				}
				catch (const std::bad_alloc & e) {
					std::cout << "Could not allocate enough space to flush current cell: " << cellId << " size=" << ce.size() << std::endl;
					throw e;
				}
			}
			ce.clear();
			relaxed.clear();
			queue.clear();
		}
		void operator()() {
			std::unique_lock<std::mutex> itlck(state->itLock, std::defer_lock);
			while (true) {
				itlck.lock();
				if (state->i >= state->s) {
					break;
				}
				uint32_t cellId = state->i;
				++(state->i);
				itlck.unlock();
				handle(cellId);
			}
		}
	};
	
	m_id2d.resize(tracGraph().size());
	m_cache.clear();
	
	if (threadCount == 1) {
		Worker w(&state);
		for(uint32_t i(0), s(tracGraph().size()); i < s; ++i) {
			w.handle(i);
		}
	}
	else {
		sserialize::ThreadPool::execute(Worker(&state), threadCount, sserialize::ThreadPool::CopyTaskTag());
	}
}

std::unique_ptr<SingleCellDilatorInterface> CQRDilatorWithCache::dilator(SingleCellDilatorInterface::State * state) const {
	return std::unique_ptr<SingleCellDilatorInterface>( new SingleCellDilator(state) );
}

CQRDilatorWithCache::SingleCellDilator::SingleCellDilator(SingleCellDilatorInterface::State * state) :
CQRDilator::SingleCellDilator(state)
{}

CQRDilatorWithCache::SingleCellDilator::~SingleCellDilator()
{}

void CQRDilatorWithCache::SingleCellDilator::dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) {
	if (cellId >= parent()->m_id2d.size() || parent()->m_threshold < state()->diameter) {
		return CQRDilator::SingleCellDilator::dilate(cellId, dilatedMarks, dilated);
	}
	uint64_t i = parent()->m_id2d.at(cellId);
	uint64_t s = cellId+1 >= parent()->m_id2d.size() ? parent()->m_cache.size() : parent()->m_id2d.at(cellId+1);
	for(; i < s; ++i) {
		const CQRDilatorWithCache::CacheEntry & ce = parent()->m_cache[i];
		if (ce.distance <= state()->diameter) {
			if (!dilatedMarks.isSet(ce.cellId)) {
				dilated.push_back(ce.cellId);
				dilatedMarks.set(ce.cellId);
			}
		}
		else {
			break;
		}
	}
}

const CQRDilatorWithCache * CQRDilatorWithCache::SingleCellDilator::parent() const {
	return static_cast<const CQRDilatorWithCache *>(state()->that);
}

//END CQRDilatorWithCache

}}}//end namespace sserialize::Static::detail
