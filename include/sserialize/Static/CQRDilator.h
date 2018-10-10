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

class SingleCellDilatorInterface {
public:
	///Subclass has to initialized members
	class State {
	public:
		State() {}
		virtual ~State() {}
	public:
		const CQRDilator * that;
		double diameter;
		uint32_t lowestCellId;
		sserialize::SimpleBitVector baseCells;
		inline bool isNotBaseCell(uint32_t cid) const { //move this somewhere else/really needed?
			return (cid < lowestCellId || !baseCells.isSet(cid-lowestCellId));
		}
	};
public:
	SingleCellDilatorInterface(State * state) : m_state(state) {}
	virtual ~SingleCellDilatorInterface() {}
public:
	virtual void dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) = 0;
protected:
	State * state() { return m_state; }
	const State * state() const { return m_state; }
private:
	State * m_state;
};

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
	virtual std::unique_ptr<SingleCellDilatorInterface> dilator(SingleCellDilatorInterface::State * state) const;
protected:
	double distance(const sserialize::Static::spatial::GeoPoint & gp1, const sserialize::Static::spatial::GeoPoint & gp2) const;
	double distance(const sserialize::Static::spatial::GeoPoint & gp, uint32_t cellId) const;
	double distance(uint32_t cellId1, uint32_t cellId2) const;
protected:
	class SingleCellDilator: public SingleCellDilatorInterface {
	public:
		SingleCellDilator(SingleCellDilatorInterface::State * state);
		virtual ~SingleCellDilator();
	public:
		virtual void dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) override;
	protected:
		const CQRDilator * parent() const;
	private:
		std::unordered_set<uint32_t> relaxed;
		std::vector<uint32_t> queue;
	};
protected:
	inline const std::shared_ptr<sserialize::spatial::interface::CellDistance> & cellDistance() const { return m_cd; }
	inline const sserialize::Static::spatial::TracGraph & tracGraph() const { return m_tg; }
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
	void populateCache(uint32_t threshold, uint32_t threadCount);
protected:
	virtual std::unique_ptr<SingleCellDilatorInterface> dilator(SingleCellDilatorInterface::State * state) const;
private:
	struct CacheEntry {
		CacheEntry() {}
		CacheEntry(uint32_t cellId, uint32_t distance) : cellId(cellId), distance(distance) {}
		uint32_t cellId;
		uint32_t distance;
	};
	class SingleCellDilator: public CQRDilator::SingleCellDilator {
	public:
		SingleCellDilator(SingleCellDilatorInterface::State * state);
		virtual ~SingleCellDilator();
	public:
		virtual void dilate(uint32_t cellId, sserialize::SimpleBitVector & dilatedMarks, std::vector<uint32_t> & dilated) override;
	protected:
		const CQRDilatorWithCache * parent() const;
	};
private:
	uint32_t m_threshold;
	std::vector<uint64_t> m_id2d;
	///Each cell has a contigous range of cache entries that are sorted in ascending distance
	std::vector<CacheEntry> m_cache;
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
	///default initializer creates an unusable dilator which will segfault on usage
	CQRDilator();
	CQRDilator(const sserialize::RCPtrWrapper<detail::CQRDilator> & other);
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
	
	struct State: SingleCellDilatorInterface::State {
		
		std::mutex itlock;
		MyIterator it;
		MyIterator end;
		
		std::mutex datalock;
		std::vector<sserialize::ItemIndex> intermediates;

		State(const CQRDilator * that, MyIterator it, MyIterator end, double diameter) : 
		it(it),
		end(end)
		{
			this->that = that;
			this->diameter = diameter;
			this->lowestCellId = *it;
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
		std::unique_ptr<SingleCellDilatorInterface> m_scd;
		Worker(State * state) : state(state), m_scd(state->that->dilator(state)) {}
		Worker(const Worker & other) : Worker(other.state) {}
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
		void handle(uint32_t cellId) {
			m_scd->dilate(cellId, dilatedMarks, dilated);
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
				handle(*it);
			}
			flush();
		}
	};
	
	if (threadCount == 1) {
		Worker w(&state);
		for(; state.it != state.end; ++(state.it)) {
			w.handle(*(state.it));
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
