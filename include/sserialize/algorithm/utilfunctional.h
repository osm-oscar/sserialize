#ifndef SSERIALIZE_UTIL_FINCTIONAL_H
#define SSERIALIZE_UTIL_FINCTIONAL_H
#include <algorithm>
#include <assert.h>
#include <vector>
#include <thread>
#include <mutex>

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
  * @param func function that maps two iterator::value_type to a new one
  */
template<typename T_ITERATOR, typename T_RETURN = typename std::iterator_traits<T_ITERATOR>::value_type, typename T_FUNC>
T_RETURN treeReduce(T_ITERATOR begin, T_ITERATOR end, T_FUNC redFunc, uint32_t threadCount) {
	if (threadCount == 0) {
		threadCount = 1;
	}
	if (threadCount == 1) {
		return treeReduce(begin, end, redFunc);
	}
	using std::distance;
	
	struct State {
		T_ITERATOR & begin;
		T_FUNC & redFunc;
		
		std::size_t inputSize;
		std::size_t blockSize;
		std::vector<T_RETURN> storage;
		std::mutex lock;
		State(T_ITERATOR & begin, T_FUNC & redFunc) : begin(begin), redFunc(redFunc) {}
	};
	State state(begin, redFunc);
	state.inputSize = distance(begin, end);
	
	if (state.inputSize == 0) {
		return T_RETURN();
	}
	
	threadCount = std::min<std::size_t>(state.inputSize, threadCount);
	state.blockSize = state.inputSize/threadCount + std::size_t(state.inputSize%threadCount > 0);
	
	struct Worker {
		State * state;
		void operator()(std::size_t blockNum) {
			std::size_t blockBeginOffset = state->blockSize*blockNum;
			
			if (blockBeginOffset >= state->inputSize) {
				return;
			}
			std::size_t blockSize = std::min(state->blockSize, state->inputSize - blockBeginOffset);
			
			using std::next;
			T_ITERATOR blockBegin = next(state->begin, blockBeginOffset);
			T_ITERATOR blockEnd = next(blockBegin, blockSize);
			T_RETURN result = treeReduce(blockBegin, blockEnd, state->redFunc);
			std::unique_lock<std::mutex> lck(state->lock);
			state->storage.emplace_back( std::move(result) );
			while(true) {
				if (state->storage.size() < 2) {
					return;
				}
				std::vector<T_RETURN> myStorage;
				state->storage.swap(myStorage);
				lck.unlock();
				T_RETURN result = treeReduce(myStorage.begin(), myStorage.end(), state->redFunc);
				lck.lock();
				state->storage.emplace_back(std::move(result));
			}
			
		};
		Worker(State * state) : state(state) {}
	};

	std::vector<std::thread> threads;
	threads.reserve(threadCount);
	for(uint32_t i(0); i < threadCount; ++i) {
		threads.emplace_back(Worker(&state), i);
	}
	for(auto & t : threads) {
		t.join();
	}
	return treeReduce(state.storage.begin(), state.storage.end(), redFunc);
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
