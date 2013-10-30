#ifndef SSERIALIZE_UTF8_ITERATOR_H
#define SSERIALIZE_UTF8_ITERATOR_H
#include <sserialize/vendor/utf8.h>


template<typename octet_iterator>
class Utf8Iterator {
	octet_iterator m_begin;//one before begin
	octet_iterator m_end;//one after end
	octet_iterator m_it;//current
public:
	Utf8Iterator();
	Utf8Iterator(const octet_iterator & begin, const octet_iterator & end) :
	m_begin(begin-1), m_end(end), m_it(begin)
	{}
	virtual ~Utf8Iterator() {}
	uint32_t operator*() const {
		return utf8::peek_next<octet_iterator>(m_it, m_end);
	}
	Utf8Iterator & operator++() {
		utf8::next<octet_iterator>(m_it, m_end);
		return *this;
	}
	Utf8Iterator operator++(int) {
		Utf8Iterator copy = *this;
		this->operator++();
		return copy;
	}
	Utf8Iterator & operator--() {
		utf8::previous<octet_iterator>(m_it, m_begin);
		return *this;
	}
	Utf8Iterator operator--(int) {
		Utf8Iterator copy = *this;
		this->operator--();
		return copy;
	}
};


#endif