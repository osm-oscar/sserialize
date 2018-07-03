#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/utility/checks.h>

namespace sserialize {

void ItemIndexPrivateRleDECreator::flushRle() {
	if (m_rle) {
		if (m_rle == 1) { //no rle
			m_data.putVlPackedUint32(m_lastDiff << 1);
		}
		else {
			m_data.putVlPackedUint32((m_rle << 1) | 0x1);
			m_data.putVlPackedUint32(m_lastDiff << 1);
		}
	}
	m_rle = 0;
}
//flushes the data and header to m_dest
void ItemIndexPrivateRleDECreator::flushData() {
	m_dest.setPutPtr(m_beginning);
	m_dest.putVlPackedUint32(m_count);
	m_dest.putVlPackedUint32((uint32_t)m_data.size());
	m_dest.putData(m_data);
}
ItemIndexPrivateRleDECreator::ItemIndexPrivateRleDECreator(UByteArrayAdapter & data) :
m_dest(data),
m_data(new std::vector<uint8_t>(), true),
m_beginning(data.tellPutPtr()),
m_rle(0),
m_lastDiff(0),
m_count(0),
m_prev(0)
{}

ItemIndexPrivateRleDECreator::~ItemIndexPrivateRleDECreator() {}

uint32_t ItemIndexPrivateRleDECreator::size() const {
	return m_count;
}

uint32_t ItemIndexPrivateRleDECreator::cId() const {
	return m_prev;
}

uint32_t ItemIndexPrivateRleDECreator::cDelta() const {
	return m_lastDiff;
}


///push only in ascending order (id need to be unique and larger than the one before! otherwise this will eat your kitten!
void ItemIndexPrivateRleDECreator::push_back(uint32_t id) {
	SSERIALIZE_CHEAP_ASSERT(m_count == 0 || m_prev < id);
	uint32_t diff = id - m_prev;
	if (diff == m_lastDiff) {
		++m_rle;
	}
	else {
		if (m_rle) {
			if (m_rle == 1) { //no rle
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
			else {
				m_data.putVlPackedUint32((m_rle << 1) | 0x1);
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
		}
		m_rle = 1;
		m_lastDiff = diff;
	}
	m_prev = id;
	++m_count;
}

void ItemIndexPrivateRleDECreator::push_rle(uint32_t nextId, uint32_t delta, uint32_t length) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), delta);
	push_back(nextId);
	if (length) {
		push_back(nextId+delta);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(m_lastDiff, delta);
		length -= 1;
		//if length == 0, then this stuff does not change
		m_rle += length;
		m_prev += delta*length;
		m_count += length;
	}
}

///Does a flush and appends the data which has countInData elements (mainly used by set functions)
void ItemIndexPrivateRleDECreator::flushWithData(const UByteArrayAdapter & appendData, uint32_t countInData) {
	flushRle();
	m_data.putData(appendData);
	m_count += countInData;
	flushData();
}

///you need to call this prior to using toIndex() or using the data
///you should not push after calling this function
void ItemIndexPrivateRleDECreator::flush() {
	flushRle();
	flushData();
}

ItemIndex ItemIndexPrivateRleDECreator::getIndex() {
	return ItemIndex(UByteArrayAdapter(m_dest, m_beginning), ItemIndex::T_RLE_DE);
}

ItemIndexPrivate * ItemIndexPrivateRleDECreator::getPrivateIndex() {
	return new ItemIndexPrivateRleDE(UByteArrayAdapter(m_dest, m_beginning));
}

///ITERATOR

ItemIndexPrivateRleDE::MyIterator::MyIterator(const sserialize::ItemIndexPrivateRleDE::MyIterator & other) :
m_parent(other.m_parent),
m_dataOffset(other.m_dataOffset),
m_curRleCount(other.m_curRleCount),
m_curRleDiff(other.m_curRleDiff),
m_curId(other.m_curId)
{}

ItemIndexPrivateRleDE::MyIterator::MyIterator(const ItemIndexPrivateRleDE * parent, UByteArrayAdapter::OffsetType dataOffset) :
m_parent(parent),
m_dataOffset(dataOffset),
m_curRleCount(0),
m_curRleDiff(0),
m_curId(0)
{
	next();
}

ItemIndexPrivateRleDE::MyIterator::~MyIterator() {}

uint32_t ItemIndexPrivateRleDE::MyIterator::get() const {
	return m_curId;
}

void ItemIndexPrivateRleDE::MyIterator::next() {
	if (m_curRleCount) {
		--m_curRleCount;
		m_curId += m_curRleDiff;
	}
	else if (m_parent->m_data.size() > m_dataOffset) {
		int len;
		uint32_t tmp = m_parent->m_data.getVlPackedUint32(m_dataOffset, &len);
		m_dataOffset += len;//TODO:Check len for error code? very unlikeley, but "huge" performance costs
		if (tmp & 0x1) { //this is an rle
			m_curRleCount = (tmp >> 1)-1;
			m_curRleDiff = (m_parent->m_data.getVlPackedUint32(m_dataOffset, &len)) >> 1;
			m_curId += m_curRleDiff;
			m_dataOffset += len;
		}
		else {
			m_curId += tmp >> 1;
		}
	}
	else {
		m_curId = std::numeric_limits<uint32_t>::max();
		m_dataOffset = m_curId;
	}
}

bool ItemIndexPrivateRleDE::MyIterator::notEq(const ItemIndexPrivate::const_iterator_base_type * other) const {
	const ItemIndexPrivateRleDE::MyIterator * oIt = static_cast<const ItemIndexPrivateRleDE::MyIterator*>(other);
	return (oIt->m_dataOffset != m_dataOffset) || (oIt->m_parent != m_parent);
}

bool ItemIndexPrivateRleDE::MyIterator::eq(const ItemIndexPrivate::const_iterator_base_type * other) const {
	const ItemIndexPrivateRleDE::MyIterator * oIt = static_cast<const ItemIndexPrivateRleDE::MyIterator*>(other);
	return (oIt->m_dataOffset == m_dataOffset) && (oIt->m_parent == m_parent);
}

ItemIndexPrivate::const_iterator_base_type * ItemIndexPrivateRleDE::MyIterator::copy() const {
	return new MyIterator(*this);
}


ItemIndexPrivateRleDE::ItemIndexPrivateRleDE(const UByteArrayAdapter & data) :
m_data(data),
m_size(m_data.resetGetPtr().getVlPackedUint32()),
m_dataOffset(0),
m_curId(0)
{
	uint32_t myDataSize = m_data.getVlPackedUint32();
	m_data.shrinkToGetPtr();
	if (m_data.size() != myDataSize) {
		m_data.resize(myDataSize);
	}
}

ItemIndexPrivateRleDE::ItemIndexPrivateRleDE(const UDWIterator & /*data*/) {
	throw sserialize::UnimplementedFunctionException("ItemIndexPrivateRleDE with UDWIterator is unsupported as of now!");
}

ItemIndexPrivateRleDE::ItemIndexPrivateRleDE() : m_size(0),  m_dataOffset(0), m_curId(0) {}

ItemIndexPrivateRleDE::~ItemIndexPrivateRleDE() {}

ItemIndex::Types ItemIndexPrivateRleDE::type() const {
	return ItemIndex::T_RLE_DE;
}

uint32_t ItemIndexPrivateRleDE::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

void ItemIndexPrivateRleDE::loadIntoMemory() {
	UByteArrayAdapter::makeContigous(m_data);
}

UByteArrayAdapter ItemIndexPrivateRleDE::data() const {
	UByteArrayAdapter ret(m_data);
	ret -= sserialize::psize_v<uint32_t>(m_size) + sserialize::psize_v<uint32_t>(m_data.size());
	ret.resetPtrs();
	return ret;
}

uint32_t ItemIndexPrivateRleDE::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	int len = 0;
	for(; m_cache.size() <= pos;) {
		uint32_t value = m_data.getVlPackedUint32(m_dataOffset, &len);
		if (value & 0x1) { //rle
			uint32_t rle = value >> 1;
			m_dataOffset += len;
			value = m_data.getVlPackedUint32(m_dataOffset, &len);
			value >>= 1;
			while(rle) {
				m_curId += value;
				m_cache.push_back(m_curId);
				--rle;
			}
			m_dataOffset += len;
		}
		else {
			value >>= 1;
			m_curId += value;
			m_cache.push_back(m_curId);
			m_dataOffset += len;
		}
	}
	return m_cache[pos];
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


ItemIndexPrivateRleDE::MyBaseClass::const_iterator ItemIndexPrivateRleDE::cbegin() const {
	return new MyIterator(this, 0);
}

ItemIndexPrivateRleDE::MyBaseClass::const_iterator ItemIndexPrivateRleDE::cend() const {
	return new MyIterator(this, std::numeric_limits<uint32_t>::max());
}

uint32_t ItemIndexPrivateRleDE::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateRleDE::bpn() const {
	if (m_size)
		return m_data.size()*8/m_size;
	else
		return m_data.size()*8;
}


sserialize::UByteArrayAdapter::SizeType ItemIndexPrivateRleDE::getSizeInBytes() const {
	return m_data.size() + sserialize::psize_v<uint32_t>(m_size) + sserialize::psize_v<uint32_t>(m_data.size());
}

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

void ItemIndexPrivateRleDE::putInto(uint32_t * dest) const {
	UByteArrayAdapter tmpData(m_data);
	uint32_t * destEnd = dest + m_size;
	uint32_t prev = 0;
	while(dest != destEnd) {
		uint32_t val = tmpData.getVlPackedUint32();
		if (val & 0x1) {
			uint32_t rle = (val >> 1);
			val = tmpData.getVlPackedUint32();
			val >>= 1;

			while(rle) {
				prev += val;
				*dest = prev;
				--rle;
				++dest;
			}
		}
		else {
			prev += (val >> 1);
			*dest = prev;
			++dest;
		}
	}
}

ItemIndexPrivate * ItemIndexPrivateRleDE::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData(bitSet.data());
	UByteArrayAdapter cacheData( UByteArrayAdapter::createCache(bitSetData.size(), sserialize::MM_PROGRAM_MEMORY));
	ItemIndexPrivateRleDECreator creator(cacheData);
	uint32_t curId = 0;
	uint32_t myDataSize = narrow_check<uint32_t>( bitSetData.size() );
	for(uint32_t dataOffset = 0; dataOffset < myDataSize; ++dataOffset, curId += 8) {
		uint8_t data = bitSetData.at(dataOffset);
		for(uint32_t i = 0; data; ++i) {
			if (data & 0x1) {
				creator.push_back(curId+i);
			}
			data >>= 1;
		}
	}
	creator.flush();
	return creator.getPrivateIndex();
}

ItemIndexPrivate * ItemIndexPrivateRleDE::uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}

	UByteArrayAdapter aData(m_data);
	UByteArrayAdapter bData(cother->m_data);
	aData.resetGetPtr();
	bData.resetGetPtr();
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY));
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
	
	while (aIndexIt < aSize && bIndexIt < bSize && creator.size() < numItems) {
		if (aId == bId) {
			creator.push_back(aId);
			
			if (aRle) {
				--aRle;
			}
				
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

			if (bRle) {
				--bRle;
			}
				
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
		
			if (aRle) {
				--aRle;
			}
				
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
		
			if (bRle) {
				--bRle;
			}
				
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
			while (aRle && creator.size() < numItems) {
				creator.push_back(aId);
				aId += aVal;
				++aIndexIt;
				--aRle;
			}
		}
		else {
			creator.push_back(aId);
			++aIndexIt;
		}
		if (creator.size() < numItems) {
			//from here on,  the differences are equal to the ones in aData
			aData.shrinkToGetPtr();
			creator.flushWithData(aData, aSize - aIndexIt);
		}
		else {
			creator.flush();
		}
	}
	else if (bIndexIt < bSize) {
		if (bRle) {
			while (bRle && creator.size() < numItems) {
				creator.push_back(bId);
				bId += bVal;
				++bIndexIt;
				--bRle;
			}
		}
		else {
			creator.push_back(bId);
			++bIndexIt;
		}
		if (creator.size() < numItems) {
			//from here on,  the differences are equal to the ones in aData
			bData.shrinkToGetPtr();
			creator.flushWithData(bData, bSize - bIndexIt);
		}
		else {
			creator.flush();
		}
	}
	else {
		creator.flush();
	}
	
	dest.resetPtrs();
	
	return new ItemIndexPrivateRleDE(dest);

}

ItemIndexPrivate * ItemIndexPrivateRleDE::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	return genericOp<sserialize::detail::ItemIndexImpl::IntersectOp>(cother);
}

ItemIndexPrivate * ItemIndexPrivateRleDE::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	return genericOp<sserialize::detail::ItemIndexImpl::UniteOp>(cother);
}

ItemIndexPrivate * ItemIndexPrivateRleDE::difference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother) {
		return ItemIndexPrivate::doDifference(other);
	}
	return genericOp<sserialize::detail::ItemIndexImpl::DifferenceOp>(cother);
}

ItemIndexPrivate* ItemIndexPrivateRleDE::symmetricDifference(const ItemIndexPrivate* other) const {
	const ItemIndexPrivateRleDE * cother = dynamic_cast<const ItemIndexPrivateRleDE*>(other);
	if (!cother) {
		return ItemIndexPrivate::doSymmetricDifference(other);
	}
	return genericOp<sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp>(cother);
}

namespace { //protecting namespace

struct IndexStates {
	struct SingleState {
		SingleState(const UByteArrayAdapter & data) : data(data), rle(0), diff(0), id(0), valid(true) {
			this->data.resetPtrs();
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
						diff = data.getVlPackedUint32() >> 1;
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
		
		inline void moveTillLargerOrEqual(uint32_t inid) {
			while (this->valid && this->id < inid) {
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
		if (!s.valid) {
			--validCounter;
		}
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
	
	inline bool findMax(uint32_t & maxId) {
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
	

	inline void moveTillLargerOrEqual(const uint32_t id, bool & allEqual, bool & oneEqual) {
		allEqual = true;
		oneEqual = false;
		for(uint32_t i = 0; i < stateCount; ++i) {
			SingleState & s =  states[i];
			s.moveTillLargerOrEqual(id);
			if (!s.valid)
				--validCounter;
			allEqual = allEqual && (s.id == id);
			oneEqual = oneEqual || (s.id == id);
		}
	}
	
	///@return true iff id is valid
	bool moveToNextEqual(uint32_t & id) {
		bool allEqual = false;
		bool oneEqual = false;
		while (allValid() && !allEqual) {
			findMax(id);
			moveTillLargerOrEqual(id, allEqual, oneEqual);
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

} //end protecting namespace

ItemIndex ItemIndexPrivateRleDE::fusedIntersectDifference(const std::vector< ItemIndexPrivateRleDE* > & intersect, const std::vector< ItemIndexPrivateRleDE* >& subtract, uint32_t count) {
	IndexStates intersectStates((uint32_t) intersect.size());
	for(uint32_t i = 0; i < intersect.size(); ++i) {
		intersectStates.push_back(intersect[i]->m_data);
	}
	IndexStates subtractStates((uint32_t) subtract.size());
	for(uint32_t i = 0; i < intersect.size(); ++i) {
		subtractStates.push_back(subtract[i]->m_data);
	}

	std::vector<uint32_t> resultSet;
	resultSet.reserve(count);
	
	uint32_t id = 0;
	bool allEqual;
	bool oneEqual;
	while (intersectStates.allValid() && resultSet.size() < count) {
		if (intersectStates.moveToNextEqual(id)) {
			intersectStates.next();
			subtractStates.moveTillLargerOrEqual(id, allEqual, oneEqual);
			if (!oneEqual)
				resultSet.push_back(id);
		}
	}
	return ItemIndex(std::move(resultSet));
}


ItemIndex ItemIndexPrivateRleDE::constrainedIntersect(const std::vector< ItemIndexPrivateRleDE* > & intersect, uint32_t count) {
	IndexStates states((uint32_t) intersect.size());
	for(uint32_t i = 0; i < intersect.size(); ++i) {
		states.push_back(intersect[i]->m_data);
	}
	std::vector<uint32_t> resultSet;
	resultSet.reserve(count);
	
	uint32_t id = 0;
	while (states.allValid() && resultSet.size() < count) {
		if (states.moveToNextEqual(id)) {
			resultSet.push_back(id);
			states.next();
		}
	}
	return ItemIndex(std::move(resultSet));
}

}//end namespace
