#ifndef SSERIALIZE_STATIC_CQR_DILATOR_H
#define SSERIALIZE_STATIC_CQR_DILATOR_H
#include <sserialize/Static/TracGraph.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/containers/SimpleBitVector.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/spatial/CellQueryResult.h>

namespace sserialize {
namespace Static {
namespace detail {

//Dilate a CQR
class CQRDilator: public sserialize::RefCountObject {
public:
private:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellInfo;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg);
	~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	template<typename TCELL_ID_ITERATOR>
	sserialize::ItemIndex dilate(TCELL_ID_ITERATOR begin, TCELL_ID_ITERATOR end, double diameter, uint32_t threadCount) const;
private:
	double distance(const sserialize::Static::spatial::GeoPoint & gp1, const sserialize::Static::spatial::GeoPoint & gp2) const;
	double distance(const sserialize::Static::spatial::GeoPoint & gp, uint32_t cellId) const;
	double distance(uint32_t cellId1, uint32_t cellId2) const;
private:
	CellInfo m_ci;
	sserialize::Static::spatial::TracGraph m_tg;
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
	
	struct State {
		const CQRDilator * that;
		double diameter;
		uint32_t threadCount;
		uint32_t lowestCellId;
		sserialize::SimpleBitVector baseCells;
		
		MyIterator it;
		MyIterator end;
		std::mutex itlock;
		
		std::mutex datalock;
		sserialize::SimpleBitVector dilatedMarks;
		std::vector<uint32_t> dilated;
		
		bool isNotBaseCell(uint32_t cid) const {
			return (cid < lowestCellId || !baseCells.isSet(cid-lowestCellId));
		}
		State(const CQRDilator * that, MyIterator it, MyIterator end, double diameter, uint32_t threadCount) :
		that(that),
		diameter(diameter),
		threadCount(threadCount),
		lowestCellId(*it),
		it(it),
		end(end)
		{
			for(MyIterator it(this->it); it != end; ++it) {
				baseCells.set(*it-lowestCellId);
			}
		}
	};
	
	State state(this, begin, end, diameter, threadCount);
	
	struct Worker {
		State * state;
		sserialize::SimpleBitVector dilatedMarks;
		std::vector<uint32_t> dilated;	
		std::unordered_set<uint32_t> relaxed;
		std::vector<uint32_t> queue;
		void operator()() {
			std::unique_lock<std::mutex> lck(state->itlock, std::defer_lock_t());
			while (true) {
				lck.lock();
				if (! (state->it != state->end) ) {
					lck.unlock();
					this->flush_or_swap();
					break;
				}
				MyIterator it = state->it;
				++(state->it);
				lck.unlock();
			
				uint32_t cellId = *it;
				sserialize::Static::spatial::GeoPoint cellGp( state->that->m_ci.at(cellId) );
				auto node(state->that->m_tg.node(cellId));
				//put neighbors into workqueue
				for(uint32_t i(0), s(node.neighborCount()); i < s; ++i) {
					uint32_t nId = node.neighborId(i);
					if (state->isNotBaseCell(nId) && state->that->distance(cellGp, nId) < state->diameter) {
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
						if (state->isNotBaseCell(nId) && !relaxed.count(nId) && state->that->distance(cellGp, nId) < state->diameter) {
							queue.push_back(nId);
							relaxed.insert(nId);
						}
					}
				}
				//push them to the ouput
				for(auto x : relaxed) {
					if (!dilatedMarks.isSet(x)) {
						dilated.push_back(x);
						dilatedMarks.set(x);
					}
				}
				relaxed.clear();
				queue.clear();
			}
		}
		void flush_or_swap() {
			if (state->threadCount > 1) {
				std::lock_guard<std::mutex> lck(state->datalock);
				for(uint32_t x : dilated) {
					if (! state->dilatedMarks.isSet(x)) {
						state->dilatedMarks.set(x);
						state->dilated.push_back(x);
					}
				}
			}
			else {
				using std::swap;
				swap(state->dilated, dilated);
				swap(state->dilatedMarks, dilatedMarks);
			}
		}
	};
	//depending on the size of dilated it should be faster to just get them from dilatedMarks since these are ordered
	uint32_t dilatedSize = (uint32_t) state.dilated.size()*sizeof(uint32_t);
	if (dilatedSize > 1024 && dilatedSize*(sserialize::fastLog2(dilatedSize)-1) > state.dilatedMarks.storageSizeInBytes()) {
		state.dilatedMarks.getSet(state.dilated.begin());
	}
	else {
		std::sort(state.dilated.begin(), state.dilated.end());
	}
	return sserialize::ItemIndex(std::move(state.dilated));
}

}}}//end namespace sserialize::Static::detail

#endif
