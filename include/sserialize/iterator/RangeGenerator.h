#ifndef SSERIALIZE_UTIL_RANGE_GENERATOR_H
#define SSERIALIZE_UTIL_RANGE_GENERATOR_H
#include <sserialize/utility/assert.h>
#include <iterator>
#include <type_traits>

namespace sserialize {
namespace detail {
namespace RangeGenerator {

template<typename TSizeType>
struct IteratorBase: std::iterator<std::random_access_iterator_tag, TSizeType, TSizeType> {
	typedef TSizeType SizeType;
	typedef SizeType const_reference;
	typedef SizeType reference;
	SizeType m_v;
	SizeType m_stride;
	IteratorBase(SizeType v, SizeType stride=1) : m_v(v), m_stride(stride) {}
	inline SizeType operator*() const { return m_v; }
	inline bool operator!=(const IteratorBase & other) const { return m_v != other.m_v || m_stride != other.m_stride; }
	inline bool operator==(const IteratorBase & other) const { return m_v == other.m_v && m_stride == other.m_stride; }
};

template<typename TSizeType>
struct Iterator: public IteratorBase<TSizeType> {
	typedef IteratorBase<TSizeType> MyBaseClass;
	typedef typename MyBaseClass::difference_type difference_type;
	typedef TSizeType SizeType;
	Iterator(SizeType v, SizeType stride=1) : MyBaseClass(v, stride) {}
	inline Iterator & operator++() {
		MyBaseClass::m_v += MyBaseClass::m_stride;
		return *this;
	}
	inline Iterator & operator--() {
		MyBaseClass::m_v -= MyBaseClass::m_stride;
		return *this;
	}
	inline difference_type operator-(const Iterator & other) const {
		return ((difference_type)MyBaseClass::m_v-(difference_type)other.MyBaseClass::m_v)/MyBaseClass::m_stride;
	}
	inline Iterator & operator+=(SizeType d) {
		MyBaseClass::m_v += d*MyBaseClass::m_stride;
		return *this;
	}
	inline Iterator & operator-=(SizeType d) {
		MyBaseClass::m_v -= d*MyBaseClass::m_stride;
		return *this;
	}
	inline Iterator operator+(SizeType d) {
		Iterator other(*this);
		other += d;
		return other;
	}
	inline Iterator operator-(SizeType d) {
		Iterator other(*this);
		other -= d;
		return other;
	}
	inline bool operator>(const Iterator & other) const {
		return MyBaseClass::m_v > other.MyBaseClass::m_v;
	}
	inline bool operator<(const Iterator & other) const {
		return MyBaseClass::m_v < other.MyBaseClass::m_v;
	}
};

template<typename TSizeType>
using ReverseIterator = std::reverse_iterator< Iterator<TSizeType> >;

}} //end namespace detail::RangeGenerator

///This is a simple range generating class with iterators, iterators are comparable without a common creation instance (strides have to be the same)
///RangeGenerator(begin, end, stride).begin() == RangeGenerator(begin, end, stride).begin() (the same holds for end/rbegin/rend)
template<typename TSizeType = uint64_t>
class RangeGenerator {
public:
	typedef TSizeType SizeType;
	typedef detail::RangeGenerator::IteratorBase<SizeType> IteratorBase;
	typedef detail::RangeGenerator::Iterator<SizeType> Iterator;
	typedef std::reverse_iterator<Iterator> ReverseIterator;
	typedef Iterator iterator;
	typedef Iterator const_iterator;
	typedef ReverseIterator reverse_iterator;
	typedef ReverseIterator const_reverse_iterator;
	typedef typename IteratorBase::value_type value_type;
	typedef typename IteratorBase::reference reference;
	typedef typename IteratorBase::const_reference const_reference;
private:
	SizeType m_begin;
	SizeType m_end;
	SizeType m_stride;
public:
	///@param end one past the end (like in python)
	RangeGenerator(SizeType begin, SizeType end, SizeType stride=1) :
	m_begin(begin), m_end(end), m_stride(stride)
	{
		SSERIALIZE_CHEAP_ASSERT((m_end-m_begin) % m_stride == 0);
	}
	RangeGenerator(SizeType size) : m_begin(0), m_end(size), m_stride(1) {}
	inline SizeType size() const { return (m_end-m_begin)/m_stride; }
	inline Iterator begin() const { return Iterator(m_begin, m_stride); }
	inline Iterator cbegin() const { return Iterator(m_begin, m_stride); }
	inline Iterator end() const { return Iterator(m_end, m_stride); }
	inline Iterator cend() const { return Iterator(m_end, m_stride); }
	inline ReverseIterator rbegin() const { return ReverseIterator(end()); }
	inline ReverseIterator crbegin() const { return ReverseIterator(cend()); }
	inline ReverseIterator rend() const { return ReverseIterator(begin()); }
	inline ReverseIterator rcend() const { return ReverseIterator(cbegin()); }
	static inline RangeGenerator range(SizeType begin, SizeType end, SizeType stride=1) {
		return RangeGenerator(begin, end, stride);
	}
	inline static Iterator begin(SizeType begin, SizeType /*end*/, SizeType stride=1) { return Iterator(begin, stride); }
	inline static Iterator end(SizeType /*begin*/, SizeType end, SizeType stride=1) { return Iterator(end, stride); }

	inline static reverse_iterator rbegin(SizeType begin, SizeType end, SizeType stride=1) { return ReverseIterator(RangeGenerator::end(begin, end, stride)); }
	inline static reverse_iterator rend(SizeType begin, SizeType end, SizeType stride=1) { return ReverseIterator(RangeGenerator::begin(begin, end, stride)); }
};

}//end namespace

namespace std {

template<typename TSizeType>
inline
typename sserialize::detail::RangeGenerator::IteratorBase<TSizeType>::difference_type
distance(const sserialize::detail::RangeGenerator::Iterator<TSizeType> & a, const sserialize::detail::RangeGenerator::Iterator<TSizeType> & b) {
	return b-a;
}

}//end namespace std

#endif
