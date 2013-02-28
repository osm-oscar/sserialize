#ifndef STATIC_PAIR_H
#define STATIC_PAIR_H
#include <utility>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace Static {

template<typename TValue1, typename TValue2>
class Pair {
	TValue1 m_first;
	UByteArrayAdapter m_secondData;
public:
	Pair();
	Pair(const UByteArrayAdapter & data);
	~Pair() {}
	TValue1 first() const;
	TValue2 second() const;
	/** @return -1 if smaller, 0 if equal, 1 if larger */
	int8_t compareWithFirst(const TValue1& value) const;
};

template<typename TValue1, typename TValue2>
Pair<TValue1, TValue2>::Pair() {}

template<typename TValue1, typename TValue2>
Pair<TValue1, TValue2>::Pair(const UByteArrayAdapter & data) : 
m_secondData(data)
{
	m_secondData >> m_first;
	m_secondData.shrinkToGetPtr();
}

template<typename TValue1, typename TValue2>
TValue1
Pair<TValue1, TValue2>::first() const {
	return m_first;
}

template<typename TValue1, typename TValue2>
TValue2
Pair<TValue1, TValue2>::second() const {
	UByteArrayAdapter tmp = m_secondData;
	TValue2 dummy;
	tmp >> dummy;
	return dummy;
}

template<typename TValue1, typename TValue2>
int8_t
Pair<TValue1, TValue2>::compareWithFirst(const TValue1 & value) const {
	if (m_first < value) {
		return -1;
	}
	else if (m_first == value) {
		return 0;
	}
	else {
		return 1;
	}
}

}}//end namespace

template<typename TValue1, typename TValue2>
bool operator==(const sserialize::Static::Pair<TValue1, TValue2> & a, const sserialize::Static::Pair<TValue1, TValue2> & b) {
	return (a.first() == b.first() && a.second() == b.second());
}

template<typename TValue1, typename TValue2>
bool operator<(const sserialize::Static::Pair<TValue1, TValue2> & a, const sserialize::Static::Pair<TValue1, TValue2> & b) {
	TValue1 v1A(a.first());
	TValue1 v1B(b.first());
	if (v1A < v1B)
		return true;
	else if ( v1B > v1A)
		return false;
	else
		return a.second() < b.second();
}

template<typename TValue1, typename TValue2>
bool operator==(const sserialize::Static::Pair<TValue1, TValue2> & a, const std::pair<TValue1, TValue2> & b) {
	return (a.first() == b.first && a.second() == b.second);
}

template<typename TValue1, typename TValue2>
bool operator==(const std::pair<TValue1, TValue2> & a, const sserialize::Static::Pair<TValue1, TValue2> & b) {
	return b == a;
}

template<typename TValue1, typename TValue2>
bool operator!=(const sserialize::Static::Pair<TValue1, TValue2> & a, const sserialize::Static::Pair<TValue1, TValue2> & b) {
	return ! (a == b);
}

template<typename TValue1, typename TValue2>
bool operator!=(const sserialize::Static::Pair<TValue1, TValue2> & a, const std::pair<TValue1, TValue2> & b) {
	return ! (a == b);
}

template<typename TValue1, typename TValue2>
bool operator!=(const std::pair<TValue1, TValue2> & a, const sserialize::Static::Pair<TValue1, TValue2> & b) {
	return ! (a == b);
}

template<typename TValue1, typename TValue2>
bool operator<(const sserialize::Static::Pair<TValue1, TValue2> & a, const std::pair<TValue1, TValue2> & b) {
	TValue1 v1A(a.first());
	if (v1A < b.first)
		return true;
	else if ( b.first > v1A)
		return false;
	else
		return a.second() < b.second;
}

template<typename TValue1, typename TValue2>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::pair<TValue1, TValue2> & value) {
	destination << value.first << value.second;
	return destination;
}

template<typename TValue1, typename TValue2>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & destination, std::pair<TValue1, TValue2> & value) {
	destination >> value.first >> value.second;
	return destination;
}

#endif

