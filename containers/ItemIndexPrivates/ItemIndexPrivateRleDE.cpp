#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {

ItemIndexPrivate * ItemIndexPrivateRleDECreator::getPrivateIndex() {
	return new ItemIndexPrivateRleDE(UByteArrayAdapter(m_data, m_beginning));
}


ItemIndexPrivateRleDE::ItemIndexPrivateRleDE(const UByteArrayAdapter & data) :
m_data(UByteArrayAdapter(data, 8, data.getUint32(0))),
m_size(data.getUint32(4)),
m_dataOffset(0),
m_curId(0),
m_cache(UByteArrayAdapter::createCache(m_size*4, false) ),
m_cacheOffset(0)
{}

ItemIndexPrivateRleDE::ItemIndexPrivateRleDE() : m_size(0),  m_dataOffset(0), m_curId(0), m_cacheOffset(0) {}

ItemIndexPrivateRleDE::~ItemIndexPrivateRleDE() {}

ItemIndex::Types ItemIndexPrivateRleDE::type() const {
	return ItemIndex::T_RLE_DE;
}

int ItemIndexPrivateRleDE::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

uint32_t ItemIndexPrivateRleDE::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	int len;
	for(;m_cacheOffset <= pos;) {
		uint32_t value = m_data.getVlPackedUint32(m_dataOffset, &len);
		if (value & 0x1) { //rle
			uint32_t rle = value >> 1;
			m_dataOffset += len;
			value = m_data.getVlPackedUint32(m_dataOffset, &len);
			value >>= 1;
			if (m_cache.size() - m_cache.tellPutPtr() < 4*rle)
				m_cache.growStorage( 4*rle - (m_cache.size() - m_cache.tellPutPtr()) );
			while(rle) {
				m_curId += value;
				m_cache.putUint32(m_cacheOffset*4, m_curId);
				++m_cacheOffset;
				--rle;
			}
			m_dataOffset += len;
		}
		else {
			value >>= 1;
			m_curId += value;
			m_cache.putUint32(m_cacheOffset*4, m_curId);
			m_dataOffset += len;
			++m_cacheOffset;
		}
	}
	return m_cache.getUint32(pos*4);
}

uint32_t ItemIndexPrivateRleDE::first() const {
	if (size())
		return at(0);
	return 0;
}

uint32_t ItemIndexPrivateRleDE::last() const {
	if (size())
		return at(size()-1);
	return 0;
}

uint32_t ItemIndexPrivateRleDE::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateRleDE::bpn() const { return m_data.size()*8/m_size; } //This shouldn't cause an overflow here


uint32_t ItemIndexPrivateRleDE::getSizeInBytes() const { return m_data.size() + 8; }

void ItemIndexPrivateRleDE::putInto(DynamicBitSet & bitSet) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t mySize = size();
	uint32_t count = 0;
	uint32_t prev = 0;
	while(count < mySize) {
		uint32_t val = tmpData.getVlPackedUint32();
		if (val & 0x1) {
			uint32_t rle = (val >> 1);
			count += rle;
			val = tmpData.getVlPackedUint32();
			val >>= 1;
			
			bitSet.set(prev + rle*val); //set the last bit of this rle to improve buffer allocations
			--rle;//acount for last bit set
			while(rle) {
				prev += val;
				bitSet.set(prev);
				--rle;
			}
			prev += val;//acount for last bit set
		}
		else {
			prev += (val >> 1);
			bitSet.set(prev);
			++count;
		}
	}
}

ItemIndexPrivate *  ItemIndexPrivateRleDE::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData(bitSet.data());
	UByteArrayAdapter cacheData( UByteArrayAdapter::createCache(bitSetData.size(), false));
	ItemIndexPrivateRleDECreator creator(cacheData);
	uint32_t curId = 0;
	uint32_t myDataSize = bitSetData.size();
	for(uint32_t dataOffset = 0; dataOffset < myDataSize; ++dataOffset, curId += 8) {
		uint8_t data = bitSetData.at(dataOffset);
		for(uint32_t i = 0; data; ++i) {
			if (data & 0x1)
				creator.push_back(curId+i);
			data >>= 1;
		}
	}
	creator.flush();
	return creator.getPrivateIndex();
}

ItemIndexPrivate * ItemIndexPrivateRleDE::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doIntersect(other);

	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	ItemIndexPrivateRleDECreator creator(dest);

	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aRle = 0;
	uint32_t bRle = 0;
	uint32_t aId = 0;
	uint32_t bId = 0;
	uint32_t aVal = aData.getVlPackedUint32();
	uint32_t bVal = bData.getVlPackedUint32();
	if (aVal & 0x1) {
		aRle = aVal >> 1;
		aVal = aData.getVlPackedUint32();
	}
	if (bVal & 0x1) {
		bRle = bVal >> 1;
		bVal = bData.getVlPackedUint32();
	}
	aVal >>= 1;
	bVal >>= 1;

	aId = aVal;
	bId = bVal;
	
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aId == bId) {
			creator.push_back(aId);
			
			if (aRle)
				--aRle;
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;

			if (bRle)
				--bRle;
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
		else if (aId < bId) {
			if (aRle)
				--aRle;
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;
		}
		else {
			if (bRle)
				--bRle;
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
	}
	
	creator.flush();
	dest.resetPtrs();
	
	return new ItemIndexPrivateRleDE(dest);
}

ItemIndexPrivate * ItemIndexPrivateRleDE::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother)
		return ItemIndexPrivate::doIntersect(other);

	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	ItemIndexPrivateRleDECreator creator(dest);

	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = m_size;
	uint32_t bSize = cother->m_size;
	uint32_t aRle = 0;
	uint32_t bRle = 0;
	uint32_t aId = 0;
	uint32_t bId = 0;
	uint32_t aVal = aData.getVlPackedUint32();
	uint32_t bVal = bData.getVlPackedUint32();
	if (aVal & 0x1) {
		aRle = aVal >> 1;
		aVal = aData.getVlPackedUint32();
	}
	if (bVal & 0x1) {
		bRle = bVal >> 1;
		bVal = bData.getVlPackedUint32();
	}
	aVal >>= 1;
	bVal >>= 1;

	aId = aVal;
	bId = bVal;
	
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aId == bId) {
			creator.push_back(aId);
			
			if (aRle)
				--aRle;
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;

			if (bRle)
				--bRle;
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
		else if (aId < bId) {
			creator.push_back(aId);
		
			if (aRle)
				--aRle;
				
			if (!aRle) {
				aVal = aData.getVlPackedUint32();
				if (aVal & 0x1) {
					aRle = aVal >> 1;
					aVal = aData.getVlPackedUint32();
				}
				aVal >>= 1;
			}
			aId += aVal;
			++aIndexIt;
		}
		else {
			creator.push_back(bId);
		
			if (bRle)
				--bRle;
				
			if (!bRle) {
				bVal = bData.getVlPackedUint32();
				if (bVal & 0x1) {
					bRle = bVal >> 1;
					bVal = bData.getVlPackedUint32();
				}
				bVal >>= 1;
			}
			bId += bVal;
			++bIndexIt;
		}
	}

	if (aIndexIt < aSize) {
		if (aRle) {
			while (aRle) {
				creator.push_back(aId);
				aId += aVal;
				--aRle;
			}
		}
		else {
			creator.push_back(aId);
			++aIndexIt;
		}
		
		//from here on,  the differences are equal to the ones in aData
		aData.shrinkToGetPtr();
		creator.flushWithData(aData, aSize - aIndexIt);
	}
	else if (bIndexIt < bSize) {
		if (bRle) {
			while (bRle) {
				creator.push_back(bId);
				bId += bVal;
				--bRle;
			}
		}
		else {
			creator.push_back(bId);
			++bIndexIt;
		}
		//from here on,  the differences are equal to the ones in aData
		bData.shrinkToGetPtr();
		creator.flushWithData(bData, bSize - bIndexIt);
	}
	else {
		creator.flush();
	}
	
	dest.resetPtrs();
	
	return new ItemIndexPrivateRleDE(dest);

}

struct IndexStates {
	struct SingleState {
		SingleState(const UByteArrayAdapter & data) : data(data), rle(0), diff(0), id(0), valid(true) {
// 			this->data.resetPtrs();
			next();
		}
		UByteArrayAdapter data;
		uint32_t rle;
		uint32_t diff;
		uint32_t id;
		bool valid;
		void next() {
			if (rle) {
				id += diff;
				--rle;
			}
			else {
				valid = data.getPtrHasNext();
				if (valid) {
					rle = data.getVlPackedUint32();
					if (rle & 0x1) {
						rle >>= 1;
						diff = data.getVlPackedUint32();
						--rle;
					}
					else {
						diff = rle >> 1;
						rle = 0;
					}
					id += diff;
				}
			}
		}
		
		void moveTillLargerOrEqual(uint32_t id) {
			while (this->valid && this->id < id) {
				next();
			}
		}
	};
	
	IndexStates(uint32_t stateCount) : stateCount(stateCount), validCounter(stateCount) {
		states.reserve(stateCount);
	}
	
	void push_back(const UByteArrayAdapter & indexData) {
		states.push_back( SingleState(indexData) );
	}
	
	void next() {
		for(uint32_t i = 0; i < stateCount; ++i) {
			next(i);
		}
	}
	
	void next(uint32_t pos) {
		SingleState & s = states[pos];
		s.next();
		if (!s.valid)
			--validCounter;
	}
	
	int findMin(uint32_t & minId) {
		int minPos = std::numeric_limits<int>::min();
		minId = std::numeric_limits<uint32_t>::max();
		for(uint32_t i = 0; i < stateCount; ++i) {
			const SingleState & s = states[i];
			if (s.valid && s.id <= minId) {
				minPos = i;
				minId = s.id;
			}
		}
		return minPos;
	}
	
	bool findMax(uint32_t & maxId) {
		int maxPos = std::numeric_limits<int>::min();
		maxId = std::numeric_limits<uint32_t>::min();
		for(uint32_t i = 0; i < stateCount; ++i) {
			const SingleState & s = states[i];
			if (s.valid && s.id >= maxId) {
				maxPos = i;
				maxId = s.id;
			}
		}
		return maxPos;
	}
	

	void moveTillLargerOrEqual(const uint32_t id, bool & allEqual) {
		allEqual = true;
		for(uint32_t i = 0; i < stateCount; ++i) {
			SingleState & s =  states[i];
			s.moveTillLargerOrEqual(id);
			if (!s.valid)
				--validCounter;
			allEqual = allEqual && (s.id == id);
		}
	}
	
	///@return true iff id is valid
	bool moveToNextEqual(uint32_t & id) {
		bool allEqual = false;
		while (allValid() && !allEqual) {
			findMax(id);
			moveTillLargerOrEqual(id, allEqual);
		}
		return allValid();
	}
	
	bool allValid() {
		return validCounter == states.size();
	}
	
	bool oneIsValid() {
		return validCounter;
	}
	std::vector<SingleState> states;
	uint32_t stateCount;
	uint32_t validCounter;
};

ItemIndex ItemIndexPrivateRleDE::constrainedIntersect(const std::vector< ItemIndexPrivateRleDE* > & intersect, uint32_t count) {
	IndexStates states(intersect.size());
	for(std::size_t i = 0; i < intersect.size(); ++i) {
		states.push_back(intersect[i]->m_data);
	}
	std::vector<uint32_t> resultSet;
	resultSet.reserve(count);
	
	uint32_t id;
	while (states.allValid() && resultSet.size() < count) {
		if (states.moveToNextEqual(id)) {
			resultSet.push_back(id);
			states.next();
		}
	}
	return ItemIndex::absorb(resultSet);
}

}//end namespace