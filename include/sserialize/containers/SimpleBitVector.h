#ifndef SSERIALIZE_SIMPLE_BIT_VECTOR_H
#define SSERIALIZE_SIMPLE_BIT_VECTOR_H
#include <vector>
#include <limits>
#include <stdint.h>

namespace sserialize {

class SimpleBitVector {
public:
	typedef uint64_t BaseStorageType;
private:
	static constexpr int digits = std::numeric_limits<BaseStorageType>::digits;
public:
	SimpleBitVector();
	SimpleBitVector(const SimpleBitVector & other) = default;
	SimpleBitVector(SimpleBitVector && other) = default;
	SimpleBitVector(std::size_t size);
	~SimpleBitVector();
	SimpleBitVector & operator=(const SimpleBitVector & other) = default;
	SimpleBitVector & operator=(SimpleBitVector && other) = default;
	void swap(SimpleBitVector & other);
	std::size_t storageSizeInBytes() const;
	///O(n)
	std::size_t size() const;
	std::size_t capacity() const;
	void resize(std::size_t count);
	void set(std::size_t pos);
	void unset(std::size_t pos);
	bool isSet(std::size_t pos) const;
	void reset();
	template<typename TInputIterator>
	void set(TInputIterator begin, const TInputIterator & end);
	///get all set positions in ascending order 
	template<typename TOutputIterator, typename T_CAST_TYPE = typename std::iterator_traits<TOutputIterator>::value_type >
	void getSet(TOutputIterator out) const;
private:
	std::vector<BaseStorageType> m_d;
};

template<typename TInputIterator>
void SimpleBitVector::set(TInputIterator begin, const TInputIterator & end) {
	for(; begin != end; ++begin) {
		set(*begin);
	}
}

template<typename TOutputIterator, typename T_CAST_TYPE>
void SimpleBitVector::getSet(TOutputIterator out) const {
	std::size_t v = 0;
	for(BaseStorageType x : m_d) {
		for(std::size_t myV(v); x; ++myV, x >>= 1) {
			if (x & 0x1) {
				*out = (T_CAST_TYPE) myV;
				++out;
			}
		}
		v += digits;
	}
}

}//end namespace sserialize


#endif
