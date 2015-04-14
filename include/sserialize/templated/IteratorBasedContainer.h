#ifndef SSERIALIZE_ITERATOR_BASED_CONTAINER_H
#define SSERIALIZE_ITERATOR_BASED_CONTAINER_H
#include <sserialize/utility/utilfunctional.h>
#include <limits>
#include <iterator>
#include <type_traits>

namespace sserialize {

template<typename T_ITERATOR>
class IteratorBasedContainer {
public:
	typedef T_ITERATOR iterator;
	typedef iterator const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef reverse_iterator const_reverse_iterator;
	typedef typename std::iterator_traits<T_ITERATOR>::reference reference;
	typedef typename std::add_const<reference>::type const_reference;
private:
	T_ITERATOR m_begin;
	T_ITERATOR m_end;
	std::size_t m_size;
public:
	IteratorBasedContainer(const T_ITERATOR & begin, const T_ITERATOR & end, std::size_t size) :
	m_begin(begin), m_end(end), m_size(size)
	{}
	IteratorBasedContainer(const T_ITERATOR & begin, const T_ITERATOR & end) :
	m_begin(begin), m_end(end), m_size(std::distance(begin, end))
	{}
	~IteratorBasedContainer() {}
	std::size_t size() const { return m_size; }
	iterator begin() { return m_begin; }
	const_iterator begin() const { return m_begin; }
	const_iterator cbegin() const { return m_begin; }
	iterator end() { return m_end; }
	const_iterator end() const { return m_end; }
	const_iterator cend() const { return m_end; }

	reverse_iterator rbegin() { return reverse_iterator(end()-1); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()-1); }
	const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()-1); }
	reverse_iterator rend() { return reverse_iterator(begin()-1); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()-1); }
	const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()-1); }
	bool operator<(const IteratorBasedContainer & other) const {
		return sserialize::is_smaller(begin(), end(), other.begin(), other.end());
	}
};

template<typename T_ITERATOR>
inline std::ostream & operator<<(std::ostream & out, const IteratorBasedContainer<T_ITERATOR> & c) {
	typename IteratorBasedContainer<T_ITERATOR>::const_iterator begin(c.cbegin()), end(c.cend());
	if (begin != end) {
		out << *begin;
		++begin;
		for(; begin != end; ++begin) {
			out << ',' << *begin;
		}
	}
	return out;
}

}//end namespace


#endif