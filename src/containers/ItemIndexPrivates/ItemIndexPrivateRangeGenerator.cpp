#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRangeGenerator.h>
#include <algorithm>

namespace sserialize {


ItemIndexPrivateRangeGenerator::ItemIndexPrivateRangeGenerator() :
m_data(0,0)
{}

ItemIndexPrivateRangeGenerator::ItemIndexPrivateRangeGenerator(const RangeGenerator<uint32_t> & data) :
m_data(data)
{}

ItemIndexPrivateRangeGenerator::~ItemIndexPrivateRangeGenerator() {}

ItemIndex::Types ItemIndexPrivateRangeGenerator::type() const {
	return ItemIndex::T_RANGE_GENERATOR;
}

uint32_t ItemIndexPrivateRangeGenerator::first() const {
	return *m_data.begin();
}

uint32_t ItemIndexPrivateRangeGenerator::last() const {
	return *m_data.rbegin();
}

uint32_t ItemIndexPrivateRangeGenerator::at(uint32_t pos) const {
	return *(m_data.begin()+pos);
}

uint32_t ItemIndexPrivateRangeGenerator::size() const {
	return m_data.size();
}

uint8_t ItemIndexPrivateRangeGenerator::bpn() const {
	return 0;
}

UByteArrayAdapter::SizeType ItemIndexPrivateRangeGenerator::getSizeInBytes() const {
	return 0;
}


}//end namespace
