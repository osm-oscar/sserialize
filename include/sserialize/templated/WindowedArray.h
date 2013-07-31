#ifndef SSERIALIZE_WINDOWED_ARRAY
#define SSERIALIZE_WINDOWED_ARRAY
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/utilfuncs.h>
#include <algorithm>

namespace sserialize {

template<typename T_VALUE>
class WindowedArray {
public:
	typedef T_VALUE * iterator;
	typedef const T_VALUE * const_iterator;
private:
	T_VALUE * m_begin;
	T_VALUE * m_end;
	T_VALUE * m_push;
public:
	WindowedArray() : m_begin(0), m_end(0), m_push(0) {}
	WindowedArray(T_VALUE * begin, T_VALUE * end) : m_begin(begin), m_end(end), m_push(begin) {}
	WindowedArray(T_VALUE * begin, T_VALUE * end, T_VALUE * push) : m_begin(begin), m_end(end), m_push(push) {}
	WindowedArray(const WindowedArray & other) : m_begin(other.m_begin), m_end(other.m_end), m_push(other.m_push) {}
	WindowedArray & operator=(const WindowedArray & other) {
		m_begin = other.m_begin;
		m_end = other.m_end;
		m_push = other.m_push;
		return *this;
	}
	
	std::size_t size() const { return m_push - m_begin; }
	std::size_t capacity() const { return m_end - m_begin; }
	
	///This does NOT free the associated memory 
	virtual ~WindowedArray() {}
	
	void push_back(const T_VALUE & value) {
		if (m_push < m_end) {
			*m_push = value;
			++m_push;
		}
		else {
			throw sserialize::OutOfBoundsException("WindowedArray::push_back");
		}
	}
	
	T_VALUE & operator[](std::size_t pos) { return *(m_begin+pos); }
	const T_VALUE & operator[](std::size_t pos) const { return *(m_begin+pos); }
	
	void pop_back() {
		if (m_push > m_begin)
			--m_push;
	}
	
	T_VALUE & back() {
		if (m_push > m_begin)
			return *(m_push-1);
		throw sserialize::OutOfBoundsException("WindowedArray::back"); 
	}
	const T_VALUE & back() const {
		if (m_push > m_begin)
			return *(m_push-1);
		throw sserialize::OutOfBoundsException("WindowedArray::back");
	}
	
	iterator begin() { return m_begin; }
	iterator end() { return m_push; }
	iterator capacityEnd() { return m_end; }
	
	const_iterator begin() const { return m_begin; }
	const_iterator end() const { return m_push; }
	const_iterator capacityEnd() const { return m_end; }
	
	static WindowedArray uniteSortedInPlace(WindowedArray a, WindowedArray b) {
		if (b.m_end == a.m_begin)
			return uniteSortedInPlace(b, a);
		else {
			if (a.m_end != b.m_begin)
				throw sserialize::OutOfBoundsException("WindowedArray::uniteSortedInPlace: bounds don't match");
			T_VALUE * tmp = new T_VALUE[a.size()+b.size()];
			T_VALUE * tmpBegin = tmp;
			T_VALUE * tmpEnd = std::set_union(a.m_begin, a.m_push, b.m_begin, b.m_push, tmpBegin);
			T_VALUE * d = std::copy(tmpBegin, tmpEnd, a.m_begin);
			delete[] tmp;
			return WindowedArray(a.m_begin, b.m_end, d);
		}
	}
	
	template<typename T_ITERATOR_TO_WINDOWED_ARRAYS>
	static WindowedArray uniteSortedInPlace(T_ITERATOR_TO_WINDOWED_ARRAYS begin, T_ITERATOR_TO_WINDOWED_ARRAYS end) {
		auto mergeFunc = [](WindowedArray a, WindowedArray b) { return WindowedArray::uniteSortedInPlace(a, b); };
		return sserialize::treeMap(begin, end, mergeFunc);
	}
	
};

}//end namespace

#endif