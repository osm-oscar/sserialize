#ifndef SSERIALIZE_AT_STL_INPUT_ITERATOR_H
#define SSERIALIZE_AT_STL_INPUT_ITERATOR_H
#include <sserialize/utility/refcounting.h>
#include <iterator>
#include <limits>
#include <functional>
#include <type_traits>

namespace sserialize {

template<typename T_CONTAINER, typename T_RETURN_TYPE, typename T_SIZE_TYPE>
struct ReadOnlyAtStlIteratorPassThroughDereference {

	template<typename U = T_CONTAINER>
	T_RETURN_TYPE operator()(const typename std::enable_if<std::is_pointer<T_CONTAINER>::value && std::is_same<T_CONTAINER, U>::value, U>::type & c,  T_SIZE_TYPE pos) const {
		return c->at(pos);
	}
	
	template<typename U = T_CONTAINER>
	T_RETURN_TYPE operator()(const typename std::enable_if<!std::is_pointer<T_CONTAINER>::value && std::is_same<T_CONTAINER, U>::value, U>::type & c, T_SIZE_TYPE  pos) const {
		return c.at(pos);
	}
	
	template<typename RCObj>
	T_RETURN_TYPE operator()(const RCPtrWrapper<RCObj> & c, T_SIZE_TYPE pos) const {
		return c->at(pos);
	}
};

/** This is a template class to iterate over a container which element access function is named at(size_t)
  *
  *
  *
  */
template<typename T_CONTAINER, typename T_RETURN_TYPE = typename T_CONTAINER::value_type, typename T_SIZE_TYPE=int64_t, typename T_DEREFERENCE = ReadOnlyAtStlIteratorPassThroughDereference<T_CONTAINER, T_RETURN_TYPE, T_SIZE_TYPE> >
class ReadOnlyAtStlIterator: public std::iterator<std::random_access_iterator_tag, T_RETURN_TYPE, typename std::make_signed<T_SIZE_TYPE>::type > {
public:
	typedef std::iterator<std::random_access_iterator_tag, T_RETURN_TYPE, typename std::make_signed<T_SIZE_TYPE>::type > MyBaseClass;
	typedef typename MyBaseClass::difference_type difference_type;
	typedef T_RETURN_TYPE value_type;
private:
	T_SIZE_TYPE m_pos;
	T_CONTAINER m_data;
	T_DEREFERENCE m_derefer;
public:
	ReadOnlyAtStlIterator() : m_pos(), m_data() {}
	ReadOnlyAtStlIterator(const ReadOnlyAtStlIterator & other) : m_pos(other.m_pos), m_data(other.m_data), m_derefer(other.m_derefer) {}
	ReadOnlyAtStlIterator(T_SIZE_TYPE pos, T_CONTAINER data) : m_pos(pos), m_data(data) {}
	ReadOnlyAtStlIterator(T_SIZE_TYPE pos, T_CONTAINER data, T_DEREFERENCE derefer) : m_pos(pos), m_data(data), m_derefer(derefer) {}
	virtual ~ReadOnlyAtStlIterator() {}
	T_SIZE_TYPE & pos() { return m_pos;}
	const T_SIZE_TYPE & pos() const { return m_pos;}
	T_CONTAINER & data() { return m_data; }
	const T_CONTAINER & data() const { return m_data; }
	ReadOnlyAtStlIterator & operator=(const ReadOnlyAtStlIterator & other) {
		m_pos = other.m_pos;
		m_data = other.m_data;
		m_derefer = other.m_derefer;
		return *this;
	}
	
	bool operator==(const ReadOnlyAtStlIterator & other) const {
		return m_pos == other.m_pos && m_data == other.m_data;
	}
	
	bool operator!=(const ReadOnlyAtStlIterator & other) const {
		return m_pos != other.m_pos || m_data != other.m_data;
	}
	
	bool operator<(const ReadOnlyAtStlIterator & other) const {
		return m_pos < other.m_pos && m_data == other.m_data;
	}

	bool operator<=(const ReadOnlyAtStlIterator & other) const {
		return m_pos <= other.m_pos && m_data == other.m_data;
	}

	bool operator>(const ReadOnlyAtStlIterator & other) const {
		return m_pos > other.m_pos && m_data == other.m_data;
	}

	bool operator>=(const ReadOnlyAtStlIterator & other) const {
		return m_pos >= other.m_pos && m_data == other.m_data;
	}
		
	ReadOnlyAtStlIterator operator++(int) {
		return ReadOnlyAtStlIterator(m_pos++, m_data, m_derefer);
	}
	
	ReadOnlyAtStlIterator & operator++() {
		++m_pos;
		return *this;
	}
	
	ReadOnlyAtStlIterator operator--(int) {
		return ReadOnlyAtStlIterator(m_pos--, m_data, m_derefer);
	}
	
	ReadOnlyAtStlIterator & operator--() {
		--m_pos;
		return *this;
	}
	
	ReadOnlyAtStlIterator & operator+=(difference_type offset) {
		m_pos += offset;
		return *this;
	}

	ReadOnlyAtStlIterator & operator-=(difference_type offset) {
		m_pos -= offset;
		return *this;
	}
	
	ReadOnlyAtStlIterator operator+(difference_type offset) const {
		return ReadOnlyAtStlIterator(m_pos+offset, m_data, m_derefer);
	}

	ReadOnlyAtStlIterator operator-(difference_type offset) const {
		return ReadOnlyAtStlIterator(m_pos-offset, m_data, m_derefer);
	}

	difference_type operator-(const ReadOnlyAtStlIterator & other) const {
		return (m_data == other.m_data ? static_cast<difference_type>(m_pos) - static_cast<difference_type>(other.m_pos) : std::numeric_limits<difference_type>::max());
	}
	
	T_RETURN_TYPE operator*() const {
		return m_derefer(m_data, m_pos);
	}
	
	T_RETURN_TYPE operator->() const {
		return operator*();
	}
};

}//end namespace
#endif