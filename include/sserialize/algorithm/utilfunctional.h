#ifndef SSERIALIZE_UTIL_FINCTIONAL_H
#define SSERIALIZE_UTIL_FINCTIONAL_H
#include <algorithm>
#include <assert.h>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <deque>

namespace sserialize {

//from https://stackoverflow.com/questions/18085331/recursive-lambda-functions-in-c14

template<typename Functor>
struct fix_type {
	Functor functor;

	template<typename... Args>
	decltype(auto) operator()(Args&&... args) const & {
		return functor(*this, std::forward<Args>(args)...);
	}
	
	template<typename... Args>
	decltype(auto) operator()(Args&&... args) & {
		return functor(*this, std::forward<Args>(args)...);
	}
};

template<typename Functor>
fix_type<typename std::decay<Functor>::type> fix(Functor&& functor) {
	return { std::forward<Functor>(functor) };
}

template<typename T_IT1, typename T_IT2>
bool is_smaller(T_IT1 begin1, T_IT1 end1, T_IT2 begin2, T_IT2 end2) {
	for(; begin1 != end1 && begin2 != end2; ++begin1, ++begin2) {
		if (*begin1 == *begin2) {
			continue;
		}
		if (*begin1 < *begin2) {
			return true;
		}
		else {
			return false;
		}
	}
	return begin1 == end1 && begin2 != end2;
}

template<typename T_OUT_TYPE, typename T_INPUT_IT, typename T_MAP_FUNC>
T_OUT_TYPE transform(T_INPUT_IT begin, const T_INPUT_IT & end, T_MAP_FUNC func) {
	T_OUT_TYPE out;
	typename T_OUT_TYPE::iterator outIt(out.end());
	while(begin  != end) {
		outIt = out.insert(outIt, func(*begin));
		++begin;
	};
	return out;
};

template<typename T_CONTAINER>
T_CONTAINER sort(T_CONTAINER a) {
	std::sort(a.begin(), a.end());
	return a;
}

/** @param begin iterator pointing to the first element
  * @param end iterator pointing past the last element
  * @param func function that maps two iterator::value_type to a new one
  */
template<typename T_ITERATOR, typename T_RETURN = typename std::iterator_traits<T_ITERATOR>::value_type, typename T_FUNC>
T_RETURN treeReduce(T_ITERATOR begin, T_ITERATOR end, T_FUNC redFunc) {
	if (end - begin == 0) {
		return T_RETURN();
	}
	else if (end - begin == 1) {
		return *begin;
	}
	else if (end - begin == 2) {
		return redFunc(*begin, *(begin+1));
	}
	else {
		return redFunc( treeReduce<T_ITERATOR, T_RETURN, T_FUNC>(begin, begin+(end-begin)/2, redFunc),
						treeReduce<T_ITERATOR, T_RETURN, T_FUNC>(begin+(end-begin)/2, end, redFunc)
					);
	}
}

/** @param begin iterator pointing to the first element
  * @param end iterator pointing past the last element
  * @param redfunc function that maps two T_RETURN to a new one
  * @param mapfunc function that maps one iterator to T_RETURN
  */
template<typename T_ITERATOR, typename T_RETURN = typename std::iterator_traits<T_ITERATOR>::value_type, typename T_REDFUNC, typename T_MAPFUNC>
T_RETURN treeReduceMap(T_ITERATOR begin, T_ITERATOR end, T_REDFUNC redFunc, T_MAPFUNC mapFunc) {
	if (end - begin == 0) {
		return T_RETURN();
	}
	else if (end - begin == 1) {
		return mapFunc(*begin);
	}
	else if (end - begin == 2) {
		return redFunc(mapFunc(*begin), mapFunc(*(begin+1)));
	}
	else {
		return redFunc( treeReduceMap<T_ITERATOR, T_RETURN, T_REDFUNC, T_MAPFUNC>(begin, begin+(end-begin)/2, redFunc, mapFunc),
						treeReduceMap<T_ITERATOR, T_RETURN, T_REDFUNC, T_MAPFUNC>(begin+(end-begin)/2, end, redFunc, mapFunc)
					);
	}
}

/** @param begin iterator pointing to the first element
  * @param end iterator pointing past the last element
  * @param func function that maps two iterator::value_type to a new one
  */
template<typename T_ITERATOR, typename T_RETURN, typename T_FUNC>
class TreeReducer {
public:
	typedef T_ITERATOR iterator_type;
	typedef T_RETURN return_value;
	typedef typename std::iterator_traits<iterator_type>::difference_type difference_type;
private:
	struct Range {
		Range(difference_type begin, difference_type end) : begin(begin), end(end) {}
		Range(difference_type begin, difference_type end, T_RETURN value) : begin(begin), end(end), value(value) {}
		Range(const Range &) = default;
		Range(Range &&) = default;
		Range & operator=(const Range &) = default;
		Range & operator=(Range &&) = default;
		
		auto size() { return end-begin; }
		
		bool operator<(const Range & other) const { return size() < other.size(); }
		
		Range firstHalf() { return Range(begin, begin+(end-begin)/2); }
		Range secondHalf() { return Range(begin+(end-begin)/2, end); }
		
		difference_type begin;
		difference_type end;
		
		T_RETURN value;
	};
	struct State {
		std::atomic<uint64_t> remainingRanges;
		std::atomic<uint64_t> needWork; 
		std::deque<Range> ranges;
		std::mutex lock;
		std::condition_variable cv;
		iterator_type begin;
		iterator_type end;
	};
	
	struct Worker {
		Worker(State & state) : state(state) {}
		State & state;
		void operator()() {
			std::unique_lock<std::mutex> lock(state.lock);
			lock.unlock();
			while(state.remainingRanges.load()) {
				lock.lock();
				if (!state.ranges.size()) {
					++state.needWork;
					state.cv.wait(lock);
				}
				if (state.ranges.size()) {
					Range r = state.ranges.front();
					state.ranges.pop_front();
					if (state.needWork) {
						state.ranges.push_back(r.firstHalf());
						state.ranges.push_back(r.secondHalf());
						lock.unlock();
						state.cv.notify_one();
						continue;
					}
				}
			}
		}
	};
public:
	TreeReducer(uint32_t threadCount = 0) {
		if (!threadCount) {
			std::thread::hardware_concurrency();
		}
		m_threads.reserve(threadCount);
		for(uint32_t i(0); i < threadCount; ++i) {
// 			m_threads.emplace_back(Worker());
		}
	}
	void operator()(T_ITERATOR begin, T_ITERATOR end, T_FUNC redFunc) {
		std::vector<std::thread> threads;
		State state;
		state.remainingRanges = 1;
		state.needWork = 0;
		state.ranges.emplace_back(begin, end);
	}
	~TreeReducer() {
		for(std::thread & t : m_threads) {
			t.join();
		}
	}
private:
	std::vector<std::thread> m_threads;
	State s;
	std::mutex m_lock;
};

namespace ReorderMappers {
	struct Identity {
		std::size_t at(std::size_t pos) const { return pos; }
	};
};

namespace detail {

///@param reorderMap maps new positions to old positions
template<typename T_RANDOM_ACCESS_CONTAINER, typename T_REORDER_MAP, typename SizeType>
void reorder(T_RANDOM_ACCESS_CONTAINER & srcDest, const T_REORDER_MAP & reorderMap) {
	using std::swap;
	std::vector<SizeType> itemToCurPos;
	itemToCurPos.reserve(srcDest.size());
	for(SizeType i(0), s((SizeType)srcDest.size()); i < s; ++i) {
		itemToCurPos.push_back(i);
	}
	std::vector<SizeType> posToItem(itemToCurPos);
	for(SizeType i(0), s((SizeType)srcDest.size()); i < s; ++i) {
		SizeType initialItemPos = reorderMap.at(i);
		SizeType realSrcItemPos = itemToCurPos[initialItemPos];
		SizeType itemInCurDest = posToItem[i];
		swap(srcDest[i], srcDest[realSrcItemPos]);
		//now update the information to our swapped items
		itemToCurPos[initialItemPos] = i;
		posToItem[i] = initialItemPos;
		itemToCurPos[itemInCurDest] = realSrcItemPos;
		posToItem[realSrcItemPos] = itemInCurDest;
	}
}

}

///@param reorderMap maps new positions to old positions
template<typename T_RANDOM_ACCESS_CONTAINER, typename T_REORDER_MAP>
void reorder(T_RANDOM_ACCESS_CONTAINER & srcDest, const T_REORDER_MAP & reorderMap) {
	//save some runtime space at the expense of codesize
	if (srcDest.size() < std::numeric_limits<uint16_t>::max()) {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint16_t>(srcDest, reorderMap);
	}
	else if (srcDest.size() < std::numeric_limits<uint32_t>::max()) {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint32_t>(srcDest, reorderMap);
	}
	else {
		detail::reorder<T_RANDOM_ACCESS_CONTAINER, T_REORDER_MAP, uint64_t>(srcDest, reorderMap);
	}
}

}//end namespace


#endif