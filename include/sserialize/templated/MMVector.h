#ifndef SSERIALIZE_MM_VECTOR_H
#define SSERIALIZE_MM_VECTOR_H
#include <sserialize/utility/MmappedMemory.h>

namespace sserialize {

template<typename TValue>
class MMVector {
public:
	typedef TValue value_type;
	typedef value_type * iterator;
	typedef const value_type * const_iterator;
	typedef value_type & reference;
	typedef const value_type & const_reference;
private:
	MmappedMemory<TValue> m_d;
	value_type * m_begin;
	SizeType m_size;
	SizeType m_pP; //push ptr
private:
public:
	MMVector(sserialize::MmappedMemoryType mmt) : m_d(1, mmt), m_size(1), m_pP(0) { m_begin = m_d.data(); }
	MMVector(const MMVector & other) : m_d(other.m_pP, other.m_d.type()), m_size(other.m_pP), m_pP(other.m_pP) {
		m_begin = m_d.resize(other.m_pP);
		memmove(m_begin, other.m_begin, sizeof(TValue)*m_pP);
	}
	MMVector(MMVector && other) : m_d(other.m_d), m_begin(other.m_begin), m_size(other.m_size), m_pP(other.m_pP) {}
	~MMVector() {}
	MMVector & operator=(const MMVector & other) {
		m_size = other.m_pP;
		m_pP = other.m_pP;
		m_d = MmappedMemory<TValue>(other.m_pP, other.m_d.type());
		m_begin = m_d.resize(m_pP);
		memmove(m_begin, other.m_begin, sizeof(TValue)*m_pP);

	}
	MMVector & operator=(MMVector && other) {
		m_d = other.m_d;
		m_begin = other.m_begin;
		m_size = other.m_size;
		m_pP = other.m_pP;
	}
	void swap(MMVector & other) {
		using std::swap;
		swap(m_d, other.m_d);
		swap(m_begin, other.m_begin);
		swap(m_size, other.m_size);
		swap(m_pP, other.m_pP);
	}
	SizeType size() const { return m_pP;}
	SizeType capacity() const { return m_size; }
	void reserve(SizeType size) {
		if (size > m_size) {
			m_begin = m_d.resize(size);
			m_size = size;
		}
	}
	void clear() {
		m_pP = 0;
	}
	void shrink_to_fit() {
		m_begin = resize(m_pP);
		m_size = m_pP;
	}
	void resize(SizeType size, const TValue & v = TValue()) {
		m_begin = m_d.resize(size);
		if (size > m_pP) {
			for(iterator it(m_begin+m_pP), end(m_begin+size); it != end; ++it) {
				*it = v;
			}
		}
		m_pP = size;
		m_size = size;
	}
	void push_back(const TValue & v) {
		if (m_pP >= m_size) {
			SizeType tSize = m_size + std::max<SizeType>( std::max<SizeType>(sizeof(TValue)/SSERIALIZE_SYSTEM_PAGE_SIZE, 1), m_size/100);
			m_begin = m_d.resize(tSize);
			m_size = tSize;
		}
		m_begin[m_pP] = v;
		++m_pP;
	}
	void pop_back() {
		if (m_pP > 0)
			--m_pP;
	}
	inline reference operator[](SizeType pos) { return *(begin()+pos);}
	inline const_reference operator[](SizeType pos) const { return *(cbegin()+pos);}
	inline reference at(SizeType pos) {
		if (pos < m_pP) {
			return *(begin()+pos);
		}
		throw std::out_of_range();
	}
	inline const_reference at(SizeType pos) const {
		if (pos < m_pP) {
			return *(cbegin()+pos);
		}
		throw std::out_of_range();
	}
	inline iterator begin() { return m_begin; }
	inline const_iterator begin() const { return m_begin; }
	inline const_iterator cbegin() const { return m_begin; }
	inline iterator end() { return m_begin+m_pP;}
	inline const_iterator end() const { return m_begin+m_pP;}
	inline const_iterator cend() const { m_begin+m_pP;}
	inline reference back() { return *(end()-1);} 
	inline const_reference back() const { return *(cend()-1);}
	inline reference front() { return *begin();}
	inline const_reference front() const { return *cbegin();}
};

template<typename TValue>
void swap(MMVector<TValue> & a, MMVector<TValue> & b) {
	a.swap(b);
}

}//end namespace

#endif