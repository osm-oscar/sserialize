#include <sserialize/containers/MultiBitIterator.h>
#include <sserialize/utility/utilfuncs.h>

namespace sserialize {


MultiBitIterator::MultiBitIterator() :
m_bitOffset(0)
{}

MultiBitIterator::MultiBitIterator(const UByteArrayAdapter & data) :
m_data(data),
m_bitOffset(0)
{
	m_data.resetGetPtr();
}

MultiBitIterator::~MultiBitIterator() {}

MultiBitIterator & MultiBitIterator::operator+=(uint32_t bitCount) {
	bitCount += m_bitOffset;
	m_data.incGetPtr(bitCount/8);
	m_bitOffset = bitCount % 8;
	return *this;
}

void MultiBitIterator::reset() {
	m_data.resetGetPtr();
	m_bitOffset = 0;
}

bool MultiBitIterator::hasNext() const {
	return m_bitOffset || m_data.getPtrHasNext();
}



}//end namespace