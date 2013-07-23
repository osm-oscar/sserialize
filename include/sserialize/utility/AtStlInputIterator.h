#ifndef SSERIALIZE_AT_STL_INPUT_ITERATOR_H
#define SSERIALIZE_AT_STL_INPUT_ITERATOR_H
#include <iterator>
#include <limits>
#include <functional>

namespace sserialize {

template<typename T_CONTAINER, typename T_RETURN_TYPE, typename T_SIZE_TYPE>
struct ReadOnlyAtStlIteratorPassThroughDereference {

	T_RETURN_TYPE operator()(const T_CONTAINER & c, std::enable_if<std::is_pointer<T_CONTAINER>::value, T_SIZE_TYPE>  pos) {
		return c->at(pos);
	}
	
	T_RETURN_TYPE operator()(const T_CONTAINER & c, std::enable_if<!std::is_pointer<T_CONTAINER>::value, T_SIZE_TYPE>  pos) {
		return c.at(pos);
	}
};

/** This is a template class to iterate over a container which element access function is named at(size_t)
  *
  *
  *
  */
template<typename T_CONTAINER, typename T_RETURN_TYPE, typename T_SIZE_TYPE=std::size_t, typename T_DEREFERENCE = ReadOnlyAtStlIteratorPassThroughDereference<T_CONTAINER, T_RETURN_TYPE, T_SIZE_TYPE> >
class ReadOnlyAtStlIterator: public std::iterator<std::random_access_iterator_tag, T_RETURN_TYPE, T_SIZE_TYPE> {
private:
	T_SIZE_TYPE m_pos;
	T_CONTAINER m_data;
	T_DEREFERENCE m_derefer;
public:
	ReadOnlyAtStlIterator() : m_pos(), m_data() {}
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
	}
	
	bool operator==(const ReadOnlyAtStlIterator & other) {
		return m_pos == other.m_pos && m_data == other.m_data;
	}
	
	bool operator!=(const ReadOnlyAtStlIterator & other) {
		return m_pos != other.m_pos || m_data != other.m_data;
	}
	
	bool operator<(const ReadOnlyAtStlIterator & other) {
		return m_pos < other.m_pos && m_data == other.m_data;
	}

	bool operator<=(const ReadOnlyAtStlIterator & other) {
		return m_pos <= other.m_pos && m_data == other.m_data;
	}

	bool operator>(const ReadOnlyAtStlIterator & other) {
		return m_pos > other.m_pos && m_data == other.m_data;
	}

	bool operator>=(const ReadOnlyAtStlIterator & other) {
		return m_pos >= other.m_pos && m_data == other.m_data;
	}
		
	ReadOnlyAtStlIterator operator++(int offset) {
		return ReadOnlyAtStlIterator(m_pos++, m_data);
	}
	
	ReadOnlyAtStlIterator & operator++() {
		++m_pos;
		return *this;
	}
	
	ReadOnlyAtStlIterator operator--(int offset) {
		return ReadOnlyAtStlIterator(m_pos--, m_data);
	}
	
	ReadOnlyAtStlIterator & operator--() {
		--m_pos;
		return *this;
	}
	
	ReadOnlyAtStlIterator & operator+=(T_SIZE_TYPE offset) {
		m_pos += offset;
		return *this;
	}

	ReadOnlyAtStlIterator & operator-=(T_SIZE_TYPE offset) {
		m_pos -= offset;
		return *this;
	}

	T_SIZE_TYPE operator-(const ReadOnlyAtStlIterator & other) {
		return (m_data == other.m_data ? m_pos - other.m_pos : std::numeric_limits<T_SIZE_TYPE>::max());
	}
	
	T_RETURN_TYPE operator*() {
		return m_derefer(m_data, m_pos);
	}
};

}//end namespace
#endif