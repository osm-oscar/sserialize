#ifndef SSERIALIZE_TRANSFORM_ITERATOR_H
#define SSERIALIZE_TRANSFORM_ITERATOR_H
#include <iterator>


namespace sserialize {

//very simple version of boost's transform_iterator

template<typename T_UNARY_FUNC, typename T_RETURN_TYPE, typename T_IT>
class TransformIterator: public std::iterator<std::input_iterator_tag, typename std::remove_reference<T_RETURN_TYPE>::type, typename std::iterator_traits<T_IT>::difference_type> {
private:
	T_UNARY_FUNC m_func;
	T_IT m_it;
public:
	///mapper function gets default constructed
	TransformIterator(T_IT it) : m_it(it) {}
	///args are passed t othe iterator
	template<typename ...Args>
	TransformIterator(T_UNARY_FUNC func, Args... args) : m_func(func), m_it(std::forward<Args>(args)...) {}
	///@param func: functoid
	TransformIterator(T_UNARY_FUNC func, T_IT it) : m_func(func), m_it(it) {}
	virtual ~TransformIterator() {}
	inline TransformIterator & operator++() { ++m_it; return *this;}
	inline T_RETURN_TYPE operator*() { return m_func(*m_it);}
	inline T_RETURN_TYPE operator*() const { return m_func(*m_it);}
	inline bool operator!=(const TransformIterator & other) const { return m_it != other.m_it;}
	inline bool operator==(const TransformIterator & other) const { return !(m_it != other.m_it); }
	const T_IT & base() const { return m_it; }
};

}//end namespace

namespace std {

template<typename T_UNARY_FUNC, typename T_RETURN_TYPE, typename T_IT>
inline typename iterator_traits<T_IT>::difference_type
distance(const sserialize::TransformIterator<T_UNARY_FUNC, T_RETURN_TYPE, T_IT> & a, const sserialize::TransformIterator<T_UNARY_FUNC, T_RETURN_TYPE, T_IT> & b) {
	using std::distance;
	return distance(a.base(), b.base());
}

}//end namespace std

#endif
