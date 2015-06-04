#ifndef SSERIALIZE_CFL_ARRAY_H
#define SSERIALIZE_CFL_ARRAY_H
#include <algorithm>
#include <type_traits>
#include <sserialize/utility/exceptions.h>
#include <sserialize/Static/Array.h>

namespace sserialize {
namespace detail {
namespace CFLArray {

	template<typename T_CONTAINER, bool T_IS_CONTAINER = std::is_class<T_CONTAINER>::value >
	class DefaultPointerGetter;
	
	template<typename T_CONTAINER>
	class DefaultPointerGetter<T_CONTAINER, true> {
	protected:
		typedef typename T_CONTAINER::value_type value_type;
	protected:
		value_type * get(T_CONTAINER * container, uint64_t offset) {
			return &((*container)[offset]);
		}
		const value_type * get(const T_CONTAINER * container, uint64_t offset) const {
			return &((*container)[offset]);
		}
	};
	
	template<typename T_CONTAINER>
	class DefaultPointerGetter<T_CONTAINER, false> {
	protected:
		typedef T_CONTAINER value_type;
	protected:
		value_type * get(T_CONTAINER * container, uint64_t offset) {
			return container+offset;
		}
		const value_type * get(const T_CONTAINER * container, uint64_t offset) const {
			return container+offset;
		}
	};
}} //end namespace detail::CFLArray

///Delegating container
///Container needs to provide a continuous storage like an c-array at least for the part where this container is in
///@T_POINTER_GETTER: needs to have a function [const] T_CONTAINER::value_type * get([const] T_CONTAINER * container, uint64_t offset) [const]
///AND needs to define the value_type of the container
///It should NOT have any virtual functions in order to save space
///If you want to use a simple array of type T pass T as T_CONTAINER
///Currenty limits are:
///Max size=2**29-1
///Max offset=2**34-1
template<typename T_CONTAINER, typename T_POINTER_GETTER = detail::CFLArray::DefaultPointerGetter<T_CONTAINER> >
class CFLArray final: private T_POINTER_GETTER {
public:
	typedef T_CONTAINER container_type;
	typedef typename T_POINTER_GETTER::value_type value_type;
	typedef const value_type * const_iterator;
	typedef value_type * iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef value_type & reference;
	typedef const value_type const_reference;
	typedef uint64_t size_type;
private:
	typedef T_POINTER_GETTER MyPointerGetter;
private:
	union {
		container_type * backend;
		value_type * copy;
	} m_d;
	uint64_t m_size:29;
	uint64_t m_offset:34;
	uint64_t m_delete:1;
public:
	CFLArray() {
		m_d.backend = 0;
		m_d.copy = 0;
		m_size = 0;
		m_offset = 0;
		m_delete = 0;
	}
	CFLArray(size_type size) {
		m_d.copy = new value_type[size];
		m_size = size;
		m_delete = 1;
	}
	///Create PolygonPointsContainer by copying elements from begin to end
	template<typename T_INPUT_ITERATOR>
	CFLArray(const T_INPUT_ITERATOR & begin, const T_INPUT_ITERATOR & end) {
		int size = std::distance(begin, end);
		m_size = size;
		m_delete = 1;
		m_d.copy = new value_type[m_size];
		std::copy(begin, end, m_d.copy);
	}
	CFLArray(container_type * container, uint64_t offset, uint32_t size) {
		m_d.backend = container;
		m_offset = offset;
		m_size = size;
		m_delete = 0;
	}
	CFLArray(container_type * container, uint32_t size) {
		m_d.backend = container;
		m_offset = 0;
		m_size = size;
		m_delete = 0;
	}
	CFLArray(const CFLArray & other) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		m_offset = other.m_offset;
		if (other.m_delete) {
			m_d.copy = new value_type[m_size];
			std::copy(other.begin(), other.end(), m_d.copy);
		}
		else {
			m_d.backend = other.m_d.backend;
		}
	}
	CFLArray(CFLArray && other) {
		m_size = other.m_size;
		m_delete = other.m_delete;
		m_offset = other.m_offset;
		if (other.m_delete) {
			m_d.copy = other.m_d.copy;
			other.m_delete = 0;
			other.m_d.copy = 0;
		}
		else {
			m_d.backend = other.m_d.backend;
		}
	}
	~CFLArray() {
		if (m_delete) {
			delete[] m_d.copy;
		}
	}
	CFLArray & operator=(const CFLArray & other) {
		m_size = other.m_size;
		m_offset = other.m_offset;
		if (m_delete) {
			delete[] m_d.copy;
		}
		m_delete = other.m_delete;
		if (m_delete) {
			m_d.copy = new value_type[m_size];
			std::copy(other.begin(), other.end(), m_d.copy);
		}
		else {
			m_d.backend = other.m_d.backend;
		}
		return *this;
	}
	CFLArray & operator=(CFLArray && other) {
		m_size = other.m_size;
		m_offset = other.m_offset;
		if (m_delete) {
			delete[] m_d.copy;
		}
		m_delete = other.m_delete;
		if (m_delete) {
			m_d.copy = other.m_d.copy;
			other.m_d.copy = 0;
			other.m_delete = 0;
		}
		else {
			m_d.backend = other.m_d.backend;
		}
		return *this;
	}
	void swap(CFLArray & o) {
		using std::swap;
		swap(m_d, o.m_d);
		
		uint64_t tmp = m_size;
		m_size = o.m_size;
		o.m_size = tmp;
		
		tmp = m_delete;
		m_delete = o.m_delete;
		o.m_delete = tmp;
		
		tmp = m_offset;
		m_offset = o.m_offset;
		o.m_offset = tmp;
	}
	inline uint32_t size() const { return m_size; }
	
	inline iterator begin() { return (m_delete ? m_d.copy : MyPointerGetter::get(m_d.backend, m_offset)); }
	inline const_iterator begin() const { return (m_delete ? m_d.copy : MyPointerGetter::get(m_d.backend, m_offset)); }
	inline const_iterator cbegin() const { return begin(); }
	
	inline iterator end() { return begin()+size(); }
	inline const_iterator end() const { return begin()+size(); }
	inline const_iterator cend() const { return cbegin()+size(); }
	
	inline reverse_iterator rbegin() { return reverse_iterator(end()); }
	inline const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	inline const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
	inline reverse_iterator rend() { return reverse_iterator(begin()); }
	inline const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	inline const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
	
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
	
	bool operator<(const CFLArray & other) const {
		return sserialize::is_smaller(cbegin(), cend(), other.cbegin(), other.cend());
	}
	///compares the values
	bool operator==(const CFLArray & other) const {
		if (size() != other.size()) {
			return false;
		}
		return std::equal(begin(), end(), other.begin());
	}
};

template<typename T_CONTAINER, typename T_POINTER_GETTER>
inline void swap(CFLArray<T_CONTAINER, T_POINTER_GETTER> & a, CFLArray<T_CONTAINER, T_POINTER_GETTER> & b) {
	a.swap(b);
}

template<typename T_CONTAINER, typename T_POINTER_GETTER>
std::ostream & operator<<(std::ostream & out, const CFLArray<T_CONTAINER, T_POINTER_GETTER> & c) {
	typename CFLArray<T_CONTAINER, T_POINTER_GETTER>::const_iterator begin(c.cbegin()), end(c.cend());
	if (begin != end) {
		out << *begin;
		++begin;
		for(; begin != end; ++begin) {
			out << ',' << *begin;
		}
	}
	return out;
}

template<typename T_CONTAINER, typename T_POINTER_GETTER>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const CFLArray<T_CONTAINER, T_POINTER_GETTER> & c) {
	typedef CFLArray<T_CONTAINER, T_POINTER_GETTER> MyIFLArray;
	typedef typename CFLArray<T_CONTAINER, T_POINTER_GETTER>::value_type MyValueType;
	sserialize::Static::ArrayCreator<MyValueType> ac(dest);;
	for(typename MyIFLArray::const_iterator begin(c.cbegin()), end(c.cend()); begin != end; ++begin) {
		ac.put(*begin);
	}
	ac.flush();
	return dest;
}

}//end namespace sserialize

#endif