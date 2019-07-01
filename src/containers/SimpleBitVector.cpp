#include <sserialize/containers/SimpleBitVector.h>
#include <sserialize/utility/constants.h>
#include <sserialize/algorithm/utilmath.h>

namespace sserialize {

SimpleBitVector::SimpleBitVector() {}

SimpleBitVector::SimpleBitVector(std::size_t size) : m_d(size/digits+1, 0) {}

SimpleBitVector::~SimpleBitVector() {}

void SimpleBitVector::swap(SimpleBitVector & other) {
	using std::swap;
	swap(m_d, other.m_d);
}

std::size_t SimpleBitVector::storageSizeInBytes() const {
	return m_d.size() * sizeof(BaseStorageType);
}

std::size_t SimpleBitVector::size() const {
	std::size_t result = 0;
	for(auto c : m_d) {
		result += sserialize::popCount(c);
	}
	return result;
}

std::size_t SimpleBitVector::capacity() const {
	return m_d.size()*digits;
}

void SimpleBitVector::resize(std::size_t count) {
	m_d.resize(count/digits+1, 0);
}

void SimpleBitVector::set(std::size_t pos) {
	if (UNLIKELY_BRANCH(pos/digits >= m_d.size())) {
		resize(pos);
	}
	m_d.at(pos/digits) |= (static_cast<BaseStorageType>(1) << (pos%digits));
}

void SimpleBitVector::unset(std::size_t pos) {
	if (UNLIKELY_BRANCH(pos/digits >= m_d.size())) {
		return;
	}
	m_d.at(pos/digits) &= ~(static_cast<BaseStorageType>(1) << (pos%digits));
}

bool SimpleBitVector::isSet(std::size_t pos) const {
	if (UNLIKELY_BRANCH(pos/digits >= m_d.size())) {
		return false;
	}
	return m_d.at(pos/digits) & (static_cast<BaseStorageType>(1) << (pos%digits));
}

void SimpleBitVector::reset() {
	m_d.assign(m_d.size(), 0);
}

}//end namespace
