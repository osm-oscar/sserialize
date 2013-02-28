#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateStlVector.h>

namespace sserialize {

ItemIndexPrivateStlVector::ItemIndexPrivateStlVector() : ItemIndexPrivate() {}
ItemIndexPrivateStlVector::ItemIndexPrivateStlVector(const std::vector<uint32_t> & data) : ItemIndexPrivate(), m_data(data) {}
ItemIndexPrivateStlVector::~ItemIndexPrivateStlVector() {}

ItemIndex::Types ItemIndexPrivateStlVector::type() const {
	return ItemIndex::T_STL_VECTOR;
}

void ItemIndexPrivateStlVector::absorb(std::vector< uint32_t >& data) {
	m_data.swap(data);
}

uint32_t ItemIndexPrivateStlVector::at(uint32_t pos) const {
	if (pos >= size())
		return 0;
    return m_data.at( pos );
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

}//end namespace