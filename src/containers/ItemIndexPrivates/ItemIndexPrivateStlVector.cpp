#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateStlVector.h>

namespace sserialize {

ItemIndexPrivateStlVector::ItemIndexPrivateStlVector() :
ItemIndexPrivate()
{}

ItemIndexPrivateStlVector::ItemIndexPrivateStlVector(std::vector<uint32_t> && data) :
ItemIndexPrivate(),
m_data(std::move(data))
{}

ItemIndexPrivateStlVector::ItemIndexPrivateStlVector(const std::vector<uint32_t> & data) :
ItemIndexPrivate(),
m_data(data)
{}

ItemIndexPrivateStlVector::~ItemIndexPrivateStlVector() {}

ItemIndex::Types ItemIndexPrivateStlVector::type() const {
	return ItemIndex::T_STL_VECTOR;
}

void ItemIndexPrivateStlVector::absorb(std::vector<uint32_t> & data) {
	m_data = std::move(data);
}

uint32_t ItemIndexPrivateStlVector::at(uint32_t pos) const {
	if (pos < size())
		return m_data.at( pos );
	sserialize::OutOfBoundsException("ItemIndexPrivateStlVector");
	return 0;
}

uint32_t ItemIndexPrivateStlVector::first() const {
	if (!size())
		return 0;
	return m_data.front();
}

uint32_t ItemIndexPrivateStlVector::last() const {
	if (!size())
		return 0;
	return m_data.back();
}

uint32_t ItemIndexPrivateStlVector::size() const {
	return m_data.size();
}

ItemIndexPrivate * ItemIndexPrivateStlVector::fromBitSet(const DynamicBitSet & bitSet) {
	ItemIndexPrivateStlVector * ret = new ItemIndexPrivateStlVector();
	std::vector<uint32_t> & vec = ret->m_data;
	const UByteArrayAdapter & data = bitSet.data();
	UByteArrayAdapter::SizeType s = data.size();
	uint32_t id = 0;
	for(UByteArrayAdapter::SizeType i = 0; i < s; ++i) {
		uint8_t tmp = data[i];
		uint32_t curId = id;
		while (tmp) {
			if (tmp & 0x1)
				vec.push_back(curId);
			++curId;
			tmp >>= 1;
		}
		id += 8;
	}
	return ret;
}

}//end namespace