#ifndef SSERIALIZE_UTIL_RANGE_GENERATOR_H
#define SSERIALIZE_UTIL_RANGE_GENERATOR_H
#include <assert.h>

namespace sserialize {

///This is a simple range generating class with iterators, iterators are comparable without a common creation instance (strides have to be the same)
///RangeGenerator(begin, end, stride).begin() == RangeGenerator(begin, end, stride).begin() (the same holds for end/rbegin/rend)
class RangeGenerator {
public:
	struct IteratorBase {
		uint64_t m_v;
		uint64_t m_stride;
		IteratorBase(uint64_t v, uint64_t stride=1) : m_v(v), m_stride(stride) {}
		inline uint64_t operator*() const { return m_v; }
		inline bool operator!=(const IteratorBase & other) const { return m_v != other.m_v || m_stride != other.m_stride; }
		inline bool operator==(const IteratorBase & other) const { return m_v == other.m_v && m_stride == other.m_stride; }
	};
	struct Iterator: public IteratorBase {
		Iterator(uint64_t v, uint64_t stride=1) : IteratorBase(v, stride) {}
		inline Iterator & operator++() {
			m_v += m_stride;
			return *this;
		}
	};
	struct ReverseIterator: public IteratorBase {
		ReverseIterator(uint64_t v, uint64_t stride=1) : IteratorBase(v, stride) {}
		inline ReverseIterator & operator++() {
			m_v -= m_stride;
			return *this;
		}
	};
	typedef Iterator iterator;
	typedef Iterator const_iterator;
private:
	uint64_t m_begin;
	uint64_t m_end;
	uint64_t m_stride;
public:
	///@param end one past the end (like in python)
	RangeGenerator(uint64_t begin, uint64_t end, uint64_t stride=1) : m_begin(begin), m_end(end), m_stride(stride) { assert((m_end-m_begin) % m_stride == 0);}
	RangeGenerator(uint64_t size) : m_begin(0), m_end(size), m_stride(1) {}
	inline uint64_t size() const { return (m_end-m_begin)/m_stride; }
	inline Iterator begin() const { return Iterator(m_begin, m_stride); }
	inline Iterator cbegin() const { return Iterator(m_begin, m_stride); }
	inline Iterator end() const { return Iterator(m_end, m_stride); }
	inline Iterator cend() const { return Iterator(m_end, m_stride); }
	inline ReverseIterator rbegin() const { return ReverseIterator(m_end-m_stride, m_stride); }
	inline ReverseIterator crbegin() const { return ReverseIterator(m_end-m_stride, m_stride); }
	inline ReverseIterator rend() const { return ReverseIterator(m_begin-m_stride, m_stride); }
	inline ReverseIterator rcend() const { return ReverseIterator(m_begin-m_stride, m_stride); }
	static inline RangeGenerator range(uint64_t begin, uint64_t end, uint64_t stride=1) {
		return RangeGenerator(begin, end, stride);
	}
};

}//end namespace

#endif