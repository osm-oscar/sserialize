#ifndef SSERIALIZE_IFL_ARRAY_H
#define SSERIALIZE_IFL_ARRAY_H
#include <algorithm>
#include <sserialize/utility/exceptions.h>
#include <sserialize/Static/Array.h>

namespace sserialize {

///Iterator based container that also supports copying. Iterators should be random-access. Use IteratorBasedContainer for InputIterators
template<typename T_ITERATOR, typename T_CONST_ITERATOR>
class IFLArray {
public:
	typedef T_CONST_ITERATOR const_iterator;
	typedef T_ITERATOR iterator;
	typedef typename std::iterator_traits<iterator>::reference reference;
	typedef typename std::iterator_traits<const_iterator>::reference const_reference;
	typedef typename std::iterator_traits<iterator>::value_type value_type;
	typedef uint32_t size_type;
private:
	iterator m_begin;
	uint32_t m_size:31;
	uint32_t m_delete:1;
public:
	IFLArray() : m_begin(0) {m_size = 0; m_delete = 0;}
	IFLArray(size_type size) : m_begin(0) {
		m_begin = new value_type[size];
		m_size = size;
		m_delete = 0;
	}
	///Create PolygonPointsContainer by copying elements from begin to end
	template<typename T_INPUT_ITERATOR>
	IFLArray(const T_INPUT_ITERATOR & begin, const T_INPUT_ITERATOR & end) {
		int size = std::distance(begin, end);
		m_size = size;
		m_delete = 1;
		m_begin = new value_type[m_size];
		std::copy(begin, end, m_begin);
	}
	///Create PolygonPointsContainer starting at @begin, does not copy elements
	IFLArray(const iterator & begin, uint32_t size) : m_begin(begin) {
		m_size = size;
		m_delete = 0;
	}
	IFLArray(const IFLArray & other) : m_begin(other.m_begin) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		if (other.m_delete) {
			m_begin = new value_type[m_size];
			std::copy(other.begin(), other.end(), m_begin);
		}
	}
	IFLArray(IFLArray && other) : m_begin(other.m_begin) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		if (other.m_delete) {
			other.m_delete = false;
		}
	}
	~IFLArray() {
		if (m_delete) {
			delete[] m_begin;
		}
	}
	IFLArray & operator=(const IFLArray & other) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		if (m_delete) {
			m_begin = new value_type[m_size];
			std::copy(other.begin(), other.end(), m_begin);
		}
		else {
			m_begin = other.m_begin;
		}
		return *this;
	}
	IFLArray & operator=(const IFLArray && other) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		m_begin = other.m_begin;
		if (m_delete) {
			other.m_delete = 0;
		}
	}
	void swap(IFLArray & o) {
		using std::swap;
		swap(m_begin, o.m_begin);
		uint32_t tmp = m_size;
		m_size = o.m_size;
		o.m_size = tmp;
		tmp = m_delete;
		m_delete = o.m_delete;
		o.m_delete = tmp;
	}
	inline uint32_t size() const { return m_size; }
	inline iterator begin() { return m_begin; }
	inline const_iterator begin() const { return const_iterator(m_begin); }
	inline const_iterator cbegin() const { return begin(); }
	
	inline iterator end() { return begin()+size(); }
	inline const_iterator end() const { return begin()+size(); }
	inline const_iterator cend() const { return cbegin()+size(); }
	reference operator[](size_type pos) { return *(begin()+pos); }
	const_reference operator[](size_type pos) const { return *(begin()+pos); }
	reference at(size_type pos) {
		if (pos >= m_size || !m_size) {
			throw sserialize::OutOfBoundsException("IFLArray");
		}
		return operator[](pos);
	}
	const_reference at(size_type pos) const {
		if (pos >= m_size || !m_size) {
			throw sserialize::OutOfBoundsException("IFLArray");
		}
		return operator[](pos);
	}
	inline reference front() { return at(0);}
	inline const_reference front() const { return at(0);}

	inline reference back() { return at(m_size ? m_size-1 : m_size);}
	inline const_reference back() const { return at(m_size ? m_size-1 : m_size);}
};

template<typename T_ITERATOR, typename T_CONST_ITERATOR>
inline void swap(IFLArray<T_ITERATOR, T_CONST_ITERATOR> & a, IFLArray<T_ITERATOR, T_CONST_ITERATOR> & b) {
	a.swap(b);
}

template<typename T_ITERATOR, typename T_CONST_ITERATOR>
std::ostream & operator<<(std::ostream & out, const IFLArray<T_ITERATOR, T_CONST_ITERATOR> & c) {
	typename IFLArray<T_ITERATOR, T_CONST_ITERATOR>::const_iterator begin(c.cbegin()), end(c.cend());
	if (begin != end) {
		out << *begin;
		++begin;
		for(; begin != end; ++begin) {
			out << ',' << *begin;
		}
	}
	return out;
}

template<typename T_ITERATOR, typename T_CONST_ITERATOR>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const IFLArray<T_ITERATOR, T_CONST_ITERATOR> & c) {
	typedef IFLArray<T_ITERATOR, T_CONST_ITERATOR> MyIFLArray;
	typedef typename IFLArray<T_ITERATOR, T_CONST_ITERATOR>::value_type MyValueType;
	sserialize::Static::ArrayCreator<MyValueType> ac(dest);;
	for(typename MyIFLArray::const_iterator begin(c.cbegin()), end(c.cend()); begin != end; ++begin) {
		ac.put(*begin);
	}
	ac.flush();
	return dest;
}
}//end namespace sserialize

#endif