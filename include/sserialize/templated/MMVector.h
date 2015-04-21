#ifndef SSERIALIZE_MM_VECTOR_H
#define SSERIALIZE_MM_VECTOR_H
#include <sserialize/utility/MmappedMemory.h>
#include <sserialize/Static/Array.h>
#include <iterator>
#define SSERIALIZE_MM_VECTOR_DEFAULT_GROW_FACTOR 0.1

namespace sserialize {
///This is a partially stl-compatible vector basend on MmappedMemory.
///This is especially usefull in combination with shared or file-based memory to create large vectors without the overhead of reallocation (the paging will do this for us)
///BUG: This together with MmappedMemory is totaly broken. Fix memory allocation and deallocation
template<typename TValue>
class MMVector {
public:
	typedef TValue value_type;
	typedef value_type * iterator;
	typedef const value_type * const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef value_type & reference;
	typedef const value_type & const_reference;
	typedef OffsetType size_type;
private:
	MmappedMemory<TValue> m_d;
	value_type * m_begin;
	SizeType m_capacity;
	SizeType m_pP; //push ptr
	double m_growFactor;
private:
	SizeType calcNewGrowSize() {
		SizeType tSize = m_capacity + std::max<SizeType>( std::max<SizeType>(sizeof(TValue)/SSERIALIZE_SYSTEM_PAGE_SIZE, 1), m_capacity*m_growFactor);
		return tSize;
	}
public:
	MMVector(sserialize::MmappedMemoryType mmt = sserialize::MM_PROGRAM_MEMORY) :
	m_d(1, mmt), m_capacity(1), m_pP(0),
	m_growFactor(SSERIALIZE_MM_VECTOR_DEFAULT_GROW_FACTOR)
	{
		m_begin = m_d.data();
	}
	explicit MMVector(SizeType size, const TValue & value = TValue(), sserialize::MmappedMemoryType mmt = sserialize::MM_PROGRAM_MEMORY) :
	m_d(1, mmt), m_capacity(1), m_pP(0),
	m_growFactor(SSERIALIZE_MM_VECTOR_DEFAULT_GROW_FACTOR)
	{
		m_begin = m_d.data();
		if (size)
			resize(size, value);
	}
	explicit MMVector(sserialize::MmappedMemoryType mmt, SizeType size, const TValue & value = TValue()) :
	m_d(1, mmt), m_capacity(1), m_pP(0),
	m_growFactor(SSERIALIZE_MM_VECTOR_DEFAULT_GROW_FACTOR)
	{
		m_begin = m_d.data();
		if (size)
			resize(size, value);
	}
	MMVector(const MMVector & other) :
	m_d(std::max<SizeType>(1, other.m_pP), other.m_d.type()),
	m_capacity(std::max<SizeType>(1, other.m_pP)),
	m_pP(other.m_pP), m_growFactor(other.m_growFactor)
	{
		m_begin = m_d.data();
		sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::initMemory(other.cbegin(), other.cend(), m_begin);
	}
	MMVector(MMVector && other) :
	m_begin(other.m_begin), m_capacity(other.m_capacity), m_pP(other.m_pP), m_growFactor(other.m_growFactor)
	{
		using std::swap;
		swap(m_d, other.m_d);
		other.m_begin = 0;
		other.m_capacity = 0;
		other.m_pP = 0;
	}
	~MMVector() {}
	MMVector & operator=(const MMVector & other) {
		m_capacity = other.m_pP;
		m_pP = other.m_pP;
		m_growFactor = other.m_growFactor;
		m_d = MmappedMemory<TValue>(other.m_pP, other.m_d.type());
		m_begin = m_d.resize(m_pP);
		sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::initMemory(other.cbegin(), other.cend(), m_begin);
		return *this;
	}
	MMVector & operator=(MMVector && other) {
		using std::swap;
		swap(m_d, other.m_d);
		m_begin = other.m_begin;
		m_capacity = other.m_capacity;
		m_pP = other.m_pP;
		m_growFactor = other.m_growFactor;
		return *this;
	}
	void swap(MMVector & other) {
		using std::swap;
		swap(m_d, other.m_d);
		swap(m_begin, other.m_begin);
		swap(m_capacity, other.m_capacity);
		swap(m_pP, other.m_pP);
		swap(m_growFactor, other.m_growFactor);
	}
	sserialize::MmappedMemoryType mmt() const { return m_d.type(); }
	SizeType size() const { return m_pP;}
	SizeType capacity() const { return m_capacity; }
	double growFactor() const { return m_growFactor; }
	void growFactor(double v) { m_growFactor = v; }
	///reserve at least size entries
	void reserve(SizeType size) {
		if (size > m_capacity) {
			m_begin = m_d.resize(size);
			m_capacity = size;
		}
	}
	
	///deinits the memory and resets the size to 0, capacity is unchanged
	void clear() {
		sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::deinitMemory(begin(), end());
		m_pP = 0;
	}
	
	void shrink_to_fit() {
		m_begin = resize(m_pP);
		m_capacity = m_pP;
	}
	
	void resize(SizeType size, const TValue & v = TValue()) {
		if (size < m_pP) {
			sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::deinitMemory(m_begin+size, m_begin+m_pP);
		}
		m_begin = m_d.resize(size);
		if (size > m_pP) {
			sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::initMemory(m_begin+m_pP, m_begin+size, v);
		}
		m_pP = size;
		m_capacity = size;
	}
	
	template<typename... Args>
	void emplace_back(Args && ...args) {
		if (m_pP >= m_capacity) {
			reserve(calcNewGrowSize());
		}
		new(m_begin+m_pP) TValue(std::forward<Args>(args)...);
		++m_pP;
	}
	
	void push_back(const TValue & v) {
		emplace_back(v);
	}
	
	template<typename T_VALUE_IT>
	void push_back(T_VALUE_IT begin, const T_VALUE_IT & end) {
		if ( end > begin) {
			std::size_t pushSize = (end-begin);
			reserve(pushSize+size());
			TValue * dP = m_begin+m_pP;
			for(; begin != end; ++begin, ++dP) {
				new(dP) TValue(*begin);
			}
			m_pP += pushSize;
		}
	}

	void pop_back() {
		if (m_pP > 0) {
			sserialize::detail::MmappedMemory::MmappedMemoryHelper<TValue>::deinitMemory(m_begin+m_pP);
			--m_pP;
		}
	}
	inline reference operator[](SizeType pos) { return *(begin()+pos);}
	inline const_reference operator[](SizeType pos) const { return *(cbegin()+pos);}
	inline reference at(SizeType pos) {
		if (pos < m_pP) {
			return *(begin()+pos);
		}
		throw std::out_of_range("MMVector::at");
	}
	inline const_reference at(SizeType pos) const {
		if (pos < m_pP) {
			return *(cbegin()+pos);
		}
		throw std::out_of_range("MMVector::at");
	}
	inline iterator begin() { return m_begin; }
	inline const_iterator begin() const { return m_begin; }
	inline const_iterator cbegin() const { return m_begin; }
	inline iterator end() { return m_begin+m_pP;}
	inline const_iterator end() const { return m_begin+m_pP;}
	inline const_iterator cend() const { return m_begin+m_pP;}
	
	inline reverse_iterator rbegin() { return reverse_iterator(end()); }
	inline const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	inline const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
	inline reverse_iterator rend() { return reverse_iterator(begin());}
	inline const_reverse_iterator rend() const { return const_reverse_iterator(begin());}
	inline const_reverse_iterator crend() const { return const_reverse_iterator(cbegin());}
	
	inline reference back() { return *(end()-1);} 
	inline const_reference back() const { return *(cend()-1);}
	inline reference front() { return *begin();}
	inline const_reference front() const { return *cbegin();}
};

template<typename TValue>
void swap(MMVector<TValue> & a, MMVector<TValue> & b) {
	a.swap(b);
}

template<typename TValue>
UByteArrayAdapter & operator<<(UByteArrayAdapter & dest, const MMVector<TValue> & src) {
	sserialize::Static::ArrayCreator<TValue> ac(dest);
	ac.reserve(src.size());
	for(typename MMVector<TValue>::const_iterator it(src.cbegin()), end(src.cend()); it != end; ++it) {
		ac.put(*it);
	}
	ac.flush();
	return dest;
}

}//end namespace

#endif