#ifndef SSERIALIZE_STATIC_CQR_DILATOR_H
#define SSERIALIZE_STATIC_CQR_DILATOR_H
#include <sserialize/Static/TracGraph.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/containers/SimpleBitVector.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/mt/ThreadPool.h>
#include <sserialize/spatial/CellDistance.h>
#include <sserialize/containers/SortedOffsetIndex.h>

namespace sserialize {
namespace Static {
namespace detail {
	
class CQRDilator;

class SingleCellDilator {
public:
	class State {
	public:
		virtual ~State();
	public:
		const CQRDilator * that;
		double diameter;
		uint32_t lowestCellId;
		sserialize::SimpleBitVector baseCells;
	};
public:
	virtual ~SingleCellDilator();
public:
	virtual void dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) const;
};

class SingleCellDilatorExploring;
class SingleCellDilatorWithCache;

//Dilate a CQR
class CQRDilator: public sserialize::RefCountObject {
public:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellCenters;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellCenters & d, const sserialize::Static::spatial::TracGraph & tg);
	CQRDilator(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg);
	virtual ~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	template<typename TCELL_ID_ITERATOR>
	sserialize::ItemIndex dilate(TCELL_ID_ITERATOR begin, TCELL_ID_ITERATOR end, double diameter, uint32_t threadCount) const;
protected:
	virtual std::unique_ptr<SingleCellDilator> dilator() const;
protected:
	double distance(const sserialize::Static::spatial::GeoPoint & gp1, const sserialize::Static::spatial::GeoPoint & gp2) const;
	double distance(const sserialize::Static::spatial::GeoPoint & gp, uint32_t cellId) const;
	double distance(uint32_t cellId1, uint32_t cellId2) const;
private:
	std::shared_ptr<sserialize::spatial::interface::CellDistance> m_cd;
	sserialize::Static::spatial::TracGraph m_tg;
};

class CQRDilatorWithCache: public CQRDilator {
public:
	CQRDilatorWithCache(const CellCenters & d, const sserialize::Static::spatial::TracGraph & tg);
	CQRDilatorWithCache(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg);
	virtual ~CQRDilatorWithCache();
public:
	///@param threshold in meter
	void populateCache(uint32_t threshold);
protected:
	virtual void dilate(uint32_t cellId, uint32_t distance, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) const;
private:
	struct CacheEntry {
		uint32_t cellId:31;
		uint32_t isOnBorder:1;
		uint32_t distance;
	};
private:
	sserialize::Static::SortedOffsetIndex m_id2d;
	///Each cell has a contigous range of cache entries that are sorted in ascending distance
	std::vector<CacheEntry> m_cache;
};

class SingleCellDilatorExploring: public SingleCellDilator {
public:
	SingleCellDilatorExploring(CQRDilator * parent);
	virtual ~SingleCellDilatorExploring();
public:
	virtual void dilate(uint32_t cellId, uint32_t distance, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) const;
private:
	CQRDilator* m_priv;
};

}//end namespace detail
/**
  * ------------------------------------------------------------
  * Array<GeoPoint>
  * ------------------------------------------------------------
  * Static::Array<Static::GeoPoint>
  * ------------------------------------------------------------
  *
  * Each entry at position pos represents the center-of-mass for the cell with the id==pos
  *
  */
  
class CQRDilator {
public:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellInfo;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg);
	CQRDilator(std::shared_ptr<sserialize::spatial::interface::CellDistance> cd, const sserialize::Static::spatial::TracGraph & tg);
	~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	sserialize::ItemIndex dilate(const sserialize::CellQueryResult & src, double diameter, uint32_t threadCount) const;

	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	sserialize::ItemIndex dilate(const sserialize::ItemIndex & src, double diameter, uint32_t threadCount) const;
private:
	sserialize::RCPtrWrapper<detail::CQRDilator> m_priv;
};

namespace detail {

template<typename TCELL_ID_ITERATOR>
sserialize::ItemIndex CQRDilator::dilate(TCELL_ID_ITERATOR begin, TCELL_ID_ITERATOR end, double diameter, uint32_t threadCount) const {
	typedef TCELL_ID_ITERATOR MyIterator;
	SSERIALIZE_EXPENSIVE_ASSERT(std::is_sorted(begin, end));
	if (!(begin != end)) {
		return sserialize::ItemIndex();
	}
	
	if (!threadCount) {
		threadCount = sserialize::ThreadPool::hardware_concurrency();
	}
	
	struct State {
		const CQRDilator * that;
		double diameter;
		uint32_t lowestCellId;
		sserialize::SimpleBitVector baseCells;
		
		std::mutex itlock;
		MyIterator it;
		MyIterator end;
		
		std::mutex datalock;
		std::vector<sserialize::ItemIndex> intermediates;
		
		bool isNotBaseCell(uint32_t cid) const {
			return (cid < lowestCellId || !baseCells.isSet(cid-lowestCellId));
		}
		State(const CQRDilator * that, MyIterator it, MyIterator end, double diameter) :
		that(that),
		diameter(diameter),
		lowestCellId(*it),
		it(it),
		end(end)
		{
			for(MyIterator it(this->it); it != end; ++it) {
				baseCells.set(*it-lowestCellId);
			}
		}
	};
	
	State state(this, begin, end, diameter);
	
	struct Worker {
		State * state;
		sserialize::SimpleBitVector dilatedMarks;
		std::vector<uint32_t> dilated;
		std::unordered_set<uint32_t> relaxed;
		std::vector<uint32_t> queue;
		Worker(State * state) : state(state) {}
		Worker(const Worker & other) : state(other.state) {}
		void handle(MyIterator it) {
			uint32_t cellId = *it;
			auto node(state->that->m_tg.node(cellId));
			//put neighbors into workqueue
			for(uint32_t i(0), s(node.neighborCount()); i < s; ++i) {
				uint32_t nId = node.neighborId(i);
				if (state->isNotBaseCell(nId) && state->that->distance(cellId, nId) < state->diameter) {
					queue.push_back(nId);
					relaxed.insert(nId);
				}
			}
			//now explore the neighborhood
			while(queue.size()) {
				uint32_t oCellId = queue.back();
				queue.pop_back();
				//iterator over all neighbors
				auto tmpNode(state->that->m_tg.node(oCellId));
				for(uint32_t i(0), s(tmpNode.neighborCount()); i < s; ++i) {
					uint32_t nId = tmpNode.neighborId(i);
					if (state->isNotBaseCell(nId) && !relaxed.count(nId) && state->that->distance(cellId, nId) < state->diameter) {
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
		void flush() {
			//convert to ItemIndex
			//depending on the size of dilated it should be faster to just get them from dilatedMarks since these are ordered
			uint32_t dilatedSize = dilated.size()*sizeof(uint32_t);
			if (dilatedSize > 1024 && dilatedSize*(sserialize::fastLog2(dilatedSize)-1) > dilatedMarks.storageSizeInBytes()) {
				dilatedMarks.getSet(dilated.begin());
			}
			else {
				std::sort(dilated.begin(), dilated.end());
			}
			sserialize::ItemIndex result(std::move(dilated));
			std::unique_lock<std::mutex> dlck(state->datalock, std::defer_lock);
			while(true) {
				dlck.lock();
				if (!state->intermediates.size()) {
					state->intermediates.emplace_back(result);
					break;
				}
				auto other = state->intermediates.back();
				state->intermediates.pop_back();
				dlck.unlock();
				result = result + other;
			}
		}
		void operator()() {
			std::unique_lock<std::mutex> itlck(state->itlock, std::defer_lock);
			while (true) {
				itlck.lock();
				if (! (state->it != state->end) ) {
					break;
				}
				MyIterator it = state->it;
				++(state->it);
				itlck.unlock();
				handle(it);
			}
			flush();
		}
	};
	
	if (threadCount == 1) {
		Worker w(&state);
		for(; state.it != state.end; ++(state.it)) {
			w.handle(state.it);
		}
		w.flush();
	}
	else {
		sserialize::ThreadPool::execute(Worker(&state), threadCount, sserialize::ThreadPool::CopyTaskTag());
	}
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER(std::size_t(0), state.intermediates.size());
	return state.intermediates.front();
}

}}}//end namespace sserialize::Static::detail

#endif
