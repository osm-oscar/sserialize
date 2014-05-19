#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

ItemIndexPrivateDE::ItemIndexPrivateDE(const UByteArrayAdapter & data) :
m_data(UByteArrayAdapter(data, 8, data.getUint32(0))),
m_size(data.getUint32(4)),
m_dataOffset(0),
m_curId(0),
m_cache(UByteArrayAdapter::createCache(m_size*4, false) ),
m_cacheOffset(0)
{
	if (m_size > m_data.size())
		throw sserialize::CorruptDataException("ItemIndexPrivateDE");
	
}

ItemIndexPrivateDE::ItemIndexPrivateDE() : m_size(0),  m_dataOffset(0), m_curId(0), m_cacheOffset(0) {}

ItemIndexPrivateDE::~ItemIndexPrivateDE() {}

ItemIndex::Types ItemIndexPrivateDE::type() const {
	return ItemIndex::T_DE;
}


int ItemIndexPrivateDE::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

uint32_t ItemIndexPrivateDE::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	int len;
	for(;m_cacheOffset <= pos;) {
		m_curId += m_data.getVlPackedUint32(m_dataOffset, &len);
		m_cache.putUint32(m_cacheOffset*4, m_curId);
		m_dataOffset += len;
		++m_cacheOffset;
	}
	return m_cache.getUint32(pos*4);
}

uint32_t ItemIndexPrivateDE::first() const {
	if (size())
		return at(0);
	return 0;
}

uint32_t ItemIndexPrivateDE::last() const {
	if (size())
		return at(size()-1);
	return 0;
}

uint32_t ItemIndexPrivateDE::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateDE::bpn() const { return m_data.size()*8/m_size; } //This shouldn't cause an overflow here

uint32_t ItemIndexPrivateDE::getSizeInBytes() const { return m_data.size() + 8; }

void ItemIndexPrivateDE::putInto(DynamicBitSet & bitSet) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t mySize = size();
	uint32_t count = 0;
	uint32_t prev = 0;
	while(count < mySize) {
		prev += tmpData.getVlPackedUint32();
		bitSet.set(prev);
		++count;
	}
}

void ItemIndexPrivateDE::putInto(uint32_t * dest) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t prev = 0;
	for(uint32_t * destEnd(dest + size()); dest != destEnd; ++dest) {
		prev += tmpData.getVlPackedUint32();
		*dest = prev;
	}
}

ItemIndexPrivate * ItemIndexPrivateDE::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData(bitSet.data());
	UByteArrayAdapter cacheData( UByteArrayAdapter::createCache(bitSetData.size(), false));
	ItemIndexPrivateDECreator creator(cacheData);
	uint32_t curId = 0;
	uint32_t myDataSize = bitSetData.size();
	for(uint32_t dataOffset = 0; dataOffset < myDataSize; ++dataOffset, curId += 8) {
		uint8_t data = bitSetData.at(dataOffset);
		for(uint32_t i = 0; i < 8 && data; ++i) {
			if (data & 0x1)
				creator.push_back(curId+i);
			data >>= 1;
		}
	}
	creator.flush();
	return creator.getPrivateIndex();
}



ItemIndexPrivate * ItemIndexPrivateDE::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doIntersect(other);

	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	dest.putUint32(0); //dummy data size
	dest.putUint32(0); //dummy count
	uint32_t resSize = 0;
	uint32_t prevDest = 0;
	
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aItemId = aData.getVlPackedUint32();
	uint32_t bItemId = bData.getVlPackedUint32();
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aItemId == bItemId) {
			dest.putVlPackedUint32(aItemId - prevDest);
			prevDest = aItemId;
			aIndexIt++;
			bIndexIt++;
			++resSize;
			aItemId += aData.getVlPackedUint32();
			bItemId = bItemId + bData.getVlPackedUint32();
		}
		else if (aItemId < bItemId) {
			aIndexIt++;
			aItemId += aData.getVlPackedUint32();
		}
		else { //bItemId is smaller
			bIndexIt++;
			bItemId += bData.getVlPackedUint32();
		}
	}

	dest.putUint32(4, resSize);
	dest.putUint32(0, dest.tellPutPtr()-8);
	dest.resetGetPtr();
	return new ItemIndexPrivateDE(dest);
}

ItemIndexPrivate * ItemIndexPrivateDE::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateDE * cother = dynamic_cast<const ItemIndexPrivateDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doUnite(other);

	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	dest.putUint32(0); //dummy data size
	dest.putUint32(0); //dummy count
	uint32_t resSize = 0;
	uint32_t prevDest = 0;
	
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aItemId = aData.getVlPackedUint32();
	uint32_t bItemId = bData.getVlPackedUint32();
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aItemId == bItemId) {
			dest.putVlPackedUint32(aItemId - prevDest);
			prevDest = aItemId;
			aIndexIt++;
			bIndexIt++;
			aItemId += aData.getVlPackedUint32();
			bItemId += bData.getVlPackedUint32();
		}
		else if (aItemId < bItemId) {
			dest.putVlPackedUint32(aItemId - prevDest);
			prevDest = aItemId;
			aIndexIt++;
			aItemId += aData.getVlPackedUint32();
		}
		else { //bItemId is smaller
			dest.putVlPackedUint32(bItemId - prevDest);
			prevDest = bItemId;
			bIndexIt++;
			bItemId += bData.getVlPackedUint32();
		}
		++resSize;
	}

	if (aIndexIt < aSize) {
		dest.putVlPackedUint32(aItemId - prevDest);
		++resSize;
		++aIndexIt;
		//from here on,  the differences are equal to the ones in aData
		aData.shrinkToGetPtr();
		dest.put(aData);
		resSize += aSize - aIndexIt;
	}

	if (bIndexIt < bSize) {
		dest.putVlPackedUint32(bItemId - prevDest);
		++resSize;
		++bIndexIt;
		//from here on,  the differences are equal to the ones in aData
		bData.shrinkToGetPtr();
		dest.put(bData);
		resSize += bSize - bIndexIt;
	}

	dest.putUint32(4, resSize);
	dest.putUint32(0, dest.tellPutPtr()-8);
	dest.resetGetPtr();
	return new ItemIndexPrivateDE(dest);
}

}//end namespace