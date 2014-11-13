#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h>
#include <string.h>
#include <sserialize/utility/SerializationInfo.h>
#include <sserialize/containers/ItemIndex.h>

namespace sserialize {
namespace detail {
namespace ItemIndexPrivate {

ItemIndexPrivateNative::ItemIndexPrivateNative(const UByteArrayAdapter& data) :
m_dataMem((data+sserialize::SerializationInfo<uint32_t>::length).asMemView()),
m_size(data.getUint32(0))
{}

ItemIndexPrivateNative::ItemIndexPrivateNative() : m_size(0) {}

ItemIndexPrivateNative::~ItemIndexPrivateNative() {}

uint32_t ItemIndexPrivateNative::size() const {
	return m_size;
}

uint32_t ItemIndexPrivateNative::first() const {
	return at(0);
}

uint32_t ItemIndexPrivateNative::last() const {
	return at(size()-1);
}

uint32_t ItemIndexPrivateNative::uncheckedAt(uint32_t pos) const {
	return uncheckedAt(pos, m_dataMem.get());
}

uint32_t ItemIndexPrivateNative::at(uint32_t pos) const {
	if (pos < size()) {
		return uncheckedAt(pos);
	}
	return 0;
}

uint8_t ItemIndexPrivateNative::bpn() const {
	return getSizeInBytes()*8/(size()+1);
}

uint32_t ItemIndexPrivateNative::getSizeInBytes() const {
	return m_dataMem.size()+SerializationInfo<uint32_t>::length;
}

ItemIndex::Types ItemIndexPrivateNative::type() const {
	return ItemIndex::T_NATIVE;
}

sserialize::ItemIndexPrivate* ItemIndexPrivateNative::intersect(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateNative * cother = dynamic_cast<const ItemIndexPrivateNative*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	uint32_t maxResultSize = std::min<uint32_t>(m_size, cother->m_size);
	void * tmpResultRaw = malloc(maxResultSize*sizeof(uint32_t));
	uint32_t * tmpResult = (uint32_t*) tmpResultRaw;
	uint32_t tmpResultSize = 0;
	
	const uint8_t * myD = m_dataMem.get();
	const uint8_t * oD = cother->m_dataMem.get();
	
	for(uint32_t myI(0), oI(0); myI < m_size && oI < cother->m_size; ) {
		uint32_t myId = this->ItemIndexPrivateNative::uncheckedAt(myI, myD);
		uint32_t oId = cother->ItemIndexPrivateNative::uncheckedAt(oI, oD);
		if (myId < oId) {
			++myI;
		}
		else if (oId < myId) {
			++oI;
		}
		else {
			tmpResult[tmpResultSize] = myId;
			++oI;
			++myI;
			++tmpResultSize;
		}
	}
	UByteArrayAdapter tmpD(UByteArrayAdapter::createCache(SerializationInfo<uint32_t>::length+sizeof(uint32_t)*tmpResultSize, false));
	tmpD.putUint32(tmpResultSize);
	tmpD.put(static_cast<uint8_t*>(tmpResultRaw), sizeof(uint32_t)*tmpResultSize);//can we do this? aliasing, cahe updates?
	free(tmpResultRaw);
	return new ItemIndexPrivateNative(tmpD);
}

}}}//end namespace