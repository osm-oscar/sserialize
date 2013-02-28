#ifndef SSERIALIZE_DYNAMIC_ARRAY_H
#define SSERIALIZE_DYNAMIC_ARRAY_H
#include <stddef.h>
#include <algorithm>

namespace sserialize {

template<typename TValue>
class DynamicArray {
	size_t m_len;
	TValue * m_values;
	DynamicArray() : m_len(0), m_values(0) {}
	DynamicArray(const DynamicArray & other) : m_len(other.m_len), m_values(0) {
		if (m_len) {
			m_values = new TValue[m_len];
			for(size_t i = 0; i < m_len; ++i) {
				m_values[i] = other.m_values[i];
			}
		}
	}
	virtual ~DynamicArray() {
		delete[] m_values;
	}

	size_t size() const { return m_len; }
	
	DynamicArray& operator=(const DynamicArray & other) {
		if (this != &other) {
			if (m_len)
				delete m_values;
			m_len = other.m_len;
			m_values = new TValue[m_len];
			for(size_t i = 0; i < m_len; ++i) {
				m_values[i] = other.m_values[i];
			}
		}
		return *this;
	}

	//move operator
	DynamicArray& operator=(DynamicArray&& other) {
		if (this != &other) {
			if (m_len)
				delete m_values;
			m_len = other.m_len;
			m_values = other.m_values;
			other.m_len = 0;
			other.m_values = 0;
		}
		return *this;
	}

	void resize(size_t newSize) {
		TValue * newValues = new TValue[newSize];
		size_t copyCount = std::min(m_len, newSize);
		while(copyCount) {
			newValues[copyCount] = m_values[copyCount];
			--copyCount;
		}
		delete[] m_values;
		m_values = newValues;
		m_len = newSize;
	}

	void push_back(const TValue & value) {
		resize(m_len+1);
		m_values[m_len-1] = value;
	}

	void pop_back() {
		if (m_len >  0)
			resize(m_len-1);
	}
	
	TValue & operator[](size_t pos) {
		return m_values[pos];
	}

	const TValue & operator[](size_t pos) const {
		return m_values[pos];
	}
};




}//end namespace

#endif