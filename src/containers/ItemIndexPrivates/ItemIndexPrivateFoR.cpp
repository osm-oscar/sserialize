#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateFoR.h>
#include <sserialize/utility/assert.h>

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {
//BEGIN FoRBlock

FoRBlock::FoRBlock() :
m_dataSize(0)
{}

FoRBlock::FoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn) :
m_values(size)
{
	m_dataSize = decodeBlock(d, prev, size, bpn);
}

uint32_t FoRBlock::size() const {
	return m_values.size();
}

sserialize::UByteArrayAdapter::SizeType FoRBlock::getSizeInBytes() const {
	return m_dataSize;
}

void FoRBlock::update(const UByteArrayAdapter& d, uint32_t prev, uint32_t size, uint32_t bpn) {
	m_dataSize = decodeBlock(d, prev, size, bpn);
}

uint32_t FoRBlock::front() const {
	return m_values.front();
}

uint32_t FoRBlock::back() const {
	return m_values.back();
}

uint32_t FoRBlock::at(uint32_t pos) const {
	return m_values.at(pos);
}


FoRBlock::const_iterator FoRBlock::begin() const {
	return m_values.begin();
}

FoRBlock::const_iterator FoRBlock::cbegin() const {
	return m_values.cbegin();
}

FoRBlock::const_iterator FoRBlock::end() const {
	return m_values.end();
}

FoRBlock::const_iterator FoRBlock::cend() const {
	return m_values.cend();
}

sserialize::SizeType FoRBlock::decodeBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(UByteArrayAdapter::SizeType(0), d.tellGetPtr());
	m_values.resize(size);
	auto blockStorageSize = CompactUintArray::minStorageBytes(bpn, size);
#if 0
	MultiBitIterator ait(d);
	for(uint32_t i(0); i < size; ++i, ait += bpn) {
		prev += ait.get32(bpn);
		m_values[i] = prev;
	}
#else
	sserialize::UByteArrayAdapter::MemoryView mv(d.getMemView(0, blockStorageSize));
	uint32_t mask = sserialize::createMask(bpn);
	const uint8_t * dit = mv.data();
	uint32_t * vit = m_values.data();
	//in theory this loop can be computed in parallel (and hopefully is executed in parallel by the processor)
	for(uint32_t i(0); i < size; ++i, ++vit) {
		uint64_t buffer = 0;
		//calculate source byte begin and end and intra byte offset
		sserialize::SizeType eb = sserialize::SizeType(i)*bpn/8;
		sserialize::SizeType ee = sserialize::SizeType(i+1)*bpn/8;
		sserialize::SizeType ie = 8-(sserialize::SizeType(i+1)*bpn%8);
		//copy these into our buffer
		int len = ee-eb+sserialize::SizeType(ie>0); // 0 < len <= 5
		if (eb+8 <= blockStorageSize) {
			::memmove(&buffer, dit+eb, 8);
			buffer = be64toh(buffer);
			buffer >>= (8-len)*8;
		}
		else {
			char * bit = ((char*)&buffer);
			const uint8_t * mydit = dit+eb+(len-1);
			for(char * bend(bit+len); bit < bend; ++bit, --mydit) {
				*bit = *mydit;
			}
	// 		::memmove(((char*)&buffer)+(8-len), dit+eb, len);
			buffer = le64toh(buffer);
		}
		buffer >>= ie;
		prev += uint32_t(buffer) & mask;
		*vit = prev;
	}
#endif
	return blockStorageSize;
}


//END FoRBlock
//BEGIN FoRIterator

FoRIterator::FoRIterator(uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data) :
m_data(data),
m_bits(bits),
m_prev(0),
m_indexPos(0),
m_indexSize(idxSize),
m_blockEnd(0),
m_blockBits(0),
m_blockIt(data)
{
	fetchBlock(m_data, 0);
}

FoRIterator::FoRIterator(uint32_t idxSize) :
m_prev(0),
m_indexPos(idxSize),
m_indexSize(idxSize),
m_blockEnd(0)
{}

FoRIterator::~FoRIterator() {}

FoRIterator::value_type
FoRIterator::get() const {
	return m_prev + m_blockIt.get32(m_blockBits);
}

void
FoRIterator::next() {
	if (m_indexPos < m_indexSize) {
		++m_indexPos;
		if (m_indexPos >= m_blockEnd) {
			fetchBlock(m_data, get());
		}
		else {
			m_blockIt += m_blockBits;
		}
	}
}

bool
FoRIterator::notEq(const MyBaseClass * other) const {
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<const FoRIterator*>(other));
	const FoRIterator * myOther = static_cast<const FoRIterator*>(other);
	return m_indexPos != myOther->m_indexPos;
}

bool FoRIterator::eq(const MyBaseClass * other) const {
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<const FoRIterator*>(other));
	const FoRIterator * myOther = static_cast<const FoRIterator*>(other);
	return m_indexPos == myOther->m_indexPos;
}

FoRIterator::MyBaseClass * FoRIterator::copy() const {
	return new FoRIterator(*this);
}

bool FoRIterator::fetchBlock(const UByteArrayAdapter& d, uint32_t prev) {
	if (m_indexPos < m_indexSize) {
		//there is exactly one partial block at the end
		//all blocks before have the same blocksize
		uint32_t defaultBlockSize = ItemIndexPrivatePFoR::BlockSizes.at(m_bits.at(0));
		uint32_t blockNum = m_indexPos/defaultBlockSize;
		uint32_t blockSize = std::min<uint32_t>(defaultBlockSize, m_indexSize - blockNum*defaultBlockSize);
		m_blockEnd = blockNum*defaultBlockSize+blockSize;
		m_blockBits = m_bits.at(blockNum+1);
		return true;
	}
	return false;
}

uint32_t FoRIterator::blockCount() const {
	return m_bits.maxCount() - 1;
}
	
//END FoRIterator
//BEGIN FoRCreator
	
FoRCreator::FoRCreator() :
FoRCreator(ItemIndexPrivatePFoR::DefaultBlockSizeOffset)
{}

FoRCreator::FoRCreator(uint32_t blockSizeOffset) :
m_size(0),
m_blockSizeOffset(blockSizeOffset),
m_prev(0),
m_vor(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest( new UByteArrayAdapter(0, MM_PROGRAM_MEMORY) ),
m_putPtr(0),
m_delete(true)
{}

FoRCreator::FoRCreator(UByteArrayAdapter & data, uint32_t blockSizeOffset) :
m_size(0),
m_blockSizeOffset(blockSizeOffset),
m_prev(0),
m_vor(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest(&data),
m_putPtr(m_dest->tellPutPtr()),
m_delete(false)
{
	SSERIALIZE_CHEAP_ASSERT_SMALLER(blockSizeOffset, ItemIndexPrivatePFoR::BlockSizes.size());
}

FoRCreator::FoRCreator(FoRCreator&& other) :
m_size(other.m_size),
m_blockSizeOffset(other.m_blockSizeOffset),
m_values(std::move(other.m_values)),
m_prev(other.m_prev),
m_vor(other.m_vor),
m_blockBits(std::move(other.m_blockBits)),
m_data(std::move(other.m_data)),
m_dest(std::move(other.m_dest)),
m_putPtr(other.m_putPtr),
m_delete(other.m_delete)
{
	other.m_dest = 0;
	other.m_delete = false;
}

FoRCreator::~FoRCreator() {
	if (m_delete) {
		delete m_dest;
	}
}

uint32_t FoRCreator::size() const {
	return m_size;
}

void FoRCreator::push_back(uint32_t id) {
	SSERIALIZE_CHEAP_ASSERT(m_prev == 0 || m_prev < id);
	m_values.push_back(id - m_prev);
	m_prev = id;
	m_vor |= m_values.back();
	if (m_values.size() == ItemIndexPrivatePFoR::BlockSizes[m_blockSizeOffset]) {
		flushBlock();
	}
	SSERIALIZE_CHEAP_ASSERT_SMALLER(m_values.size(), ItemIndexPrivatePFoR::BlockSizes[m_blockSizeOffset]);
}

void FoRCreator::flush() {
	if (m_values.size()) {
		flushBlock();
	}
	m_dest->putVlPackedUint32(m_size);
	m_dest->putVlPackedUint32(m_data.size());
	m_dest->putData(m_data);
	#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
	uint32_t bits =
	#endif
		CompactUintArray::create(m_blockBits, *m_dest, ItemIndexPrivatePFoR::BlockDescBitWidth);
	SSERIALIZE_CHEAP_ASSERT_EQUAL(bits, ItemIndexPrivatePFoR::BlockDescBitWidth);
}

UByteArrayAdapter FoRCreator::flushedData() const {
	UByteArrayAdapter d(data());
	d.setGetPtr(m_putPtr);
	return d;
}

ItemIndex FoRCreator::getIndex() {
	return ItemIndex(flushedData(), ItemIndex::T_FOR);
}

ItemIndexPrivate * FoRCreator::getPrivateIndex() {
	return new ItemIndexPrivateFoR(flushedData());
}

UByteArrayAdapter& FoRCreator::data() {
	return *m_dest;
}

const UByteArrayAdapter& FoRCreator::data() const {
	return *m_dest;
}

void FoRCreator::flushBlock() {
	SSERIALIZE_EXPENSIVE_ASSERT_ASSIGN(auto blockBegin, m_data.tellPutPtr());
	if (!m_values.size()) {
		return;
	}
	m_size += m_values.size();
	uint32_t blockBits = CompactUintArray::minStorageBits(m_vor);
	encodeBlock(m_data, m_values.begin(), m_values.end(), blockBits);
	m_blockBits.push_back(blockBits);
	
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	{
		UByteArrayAdapter blockData(m_data, blockBegin);
		blockData.shrinkToGetPtr();
		FoRBlock block(blockData, 0, m_values.size(), blockBits);
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(m_values.size(), block.size());
		uint32_t prev = 0;
		for(uint32_t i(0), s(m_values.size()); i < s; ++i) {
			prev += m_values.at(i);
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(prev, block.at(i));
		}
	}
	#endif
	
	m_values.clear();
	m_vor = 0;
}

//END FoRCreator

//BEGIN GenericSetOpExecuterInit

template<>
struct GenericSetOpExecuterInit<FoRCreator, sserialize::detail::ItemIndexImpl::IntersectOp> {
	using Creator = FoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::IntersectOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<FoRCreator, sserialize::detail::ItemIndexImpl::UniteOp> {
	using Creator = FoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::UniteOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<FoRCreator, sserialize::detail::ItemIndexImpl::DifferenceOp> {
	using Creator = FoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::DifferenceOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<FoRCreator, sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp> {
	using Creator = FoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

//END GenericSetOpExecuterInit

//BEGIN GenericSetOpExecuterAccessors

template<>
struct GenericSetOpExecuterAccessors< std::unique_ptr<detail::ItemIndexImpl::FoRIterator> > {
	typedef detail::ItemIndexImpl::FoRIterator PositionIteratorBase;
	typedef std::unique_ptr<PositionIteratorBase> PositionIterator;
	static PositionIterator begin(const sserialize::ItemIndexPrivate * idx) {
		return PositionIterator( static_cast<PositionIteratorBase*>(idx->cbegin()) );
	}
	static PositionIterator end(const sserialize::ItemIndexPrivate * idx) {
		return PositionIterator( static_cast<PositionIteratorBase*>(idx->cend()) );
	}
	static void next(PositionIterator & it) {
		it->PositionIteratorBase::next();
	}
	static bool unequal(const PositionIterator & first, const PositionIterator & second) {
		return first->PositionIteratorBase::notEq(second.get());
	}
	static uint32_t get(const sserialize::ItemIndexPrivate * /*idx*/, const PositionIterator & it) {
		return it->PositionIteratorBase::get();
	}
};

//END GenericSetOpExecuterAccessors

}} //end namespace detail::ItemIndexImpl

//BEGIN INDEX

//this is mostly code duplicated from ItemIndexPrivatePFoR
//But the PFoR index is heavily optimized and adding support for decoding FoR and PFoR introduces many ugly hacks
//So its either duplicate by hand or try using templates

ItemIndexPrivateFoR::ItemIndexPrivateFoR(sserialize::UByteArrayAdapter d) :
m_d(d)
{
	sserialize::SizeType totalSize = 0;
	SSERIALIZE_CHEAP_ASSERT_EQUAL(uint32_t(0), d.tellGetPtr());
	m_size = d.getVlPackedUint32();
	uint32_t blockDataSize = d.getVlPackedUint32();
	
	totalSize += d.tellGetPtr();
	
	d.shrinkToGetPtr();
	
	m_blocks = UByteArrayAdapter(d, 0, blockDataSize);
	m_bits = CompactUintArray(UByteArrayAdapter(d, blockDataSize), ItemIndexPrivatePFoR::BlockDescBitWidth);
	
	uint32_t blockSizeOffset = m_bits.at(0);
	uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes.at(blockSizeOffset);
	uint32_t blockCount = m_size/blockSize + uint32_t(m_size%blockSize != 0);
	
	m_bits = CompactUintArray(UByteArrayAdapter(d, blockDataSize), ItemIndexPrivatePFoR::BlockDescBitWidth, blockCount+1);
	
	totalSize += blockDataSize;
	totalSize += m_bits.getSizeInBytes();
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(totalSize, m_d.size());
	if (m_d.size() != totalSize) {
		m_d = UByteArrayAdapter(m_d, 0, totalSize);
	}
	
	m_it = cbegin();
}

ItemIndexPrivateFoR::~ItemIndexPrivateFoR() {}

ItemIndex::Types ItemIndexPrivateFoR::type() const {
	return ItemIndex::T_FOR;
}

void
ItemIndexPrivateFoR::loadIntoMemory() {}

UByteArrayAdapter ItemIndexPrivateFoR::data() const {
    return m_d;
}

uint8_t ItemIndexPrivateFoR::bpn() const {
	if (size()) {
		return sserialize::multiplyDiv64(getSizeInBytes(), 8, size());
	}
	else {
		return std::numeric_limits<uint8_t>::max();
	}
}

uint32_t
ItemIndexPrivateFoR::at(uint32_t pos) const {
	if (pos >= m_size) {
		throw std::out_of_range("ItemIndex::at with pos=" + std::to_string(pos) + "; size=" + std::to_string(size()));
	}
	
	while(m_cache.size() <= pos) {
		m_cache.emplace_back(*m_it);
		++m_it;
	}
	
	return m_cache[pos];
}

uint32_t
ItemIndexPrivateFoR::first() const {
	return at(0);
}

uint32_t
ItemIndexPrivateFoR::last() const {
	return at(m_size-1);
}

ItemIndexPrivateFoR::const_iterator
ItemIndexPrivateFoR::cbegin() const {
	return new detail::ItemIndexImpl::FoRIterator(size(), m_bits, m_blocks);
}

ItemIndexPrivateFoR::const_iterator
ItemIndexPrivateFoR::cend() const {
	return new detail::ItemIndexImpl::FoRIterator(size());
}

uint32_t
ItemIndexPrivateFoR::size() const {
	return m_size;
}

sserialize::UByteArrayAdapter::SizeType
ItemIndexPrivateFoR::getSizeInBytes() const {
	return m_d.size();
}

bool
ItemIndexPrivateFoR::is_random_access() const {
	return false;
}

void
ItemIndexPrivateFoR::putInto(DynamicBitSet & bitSet) const {
	bitSet.set(m_cache.cbegin(), m_cache.cend());
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i = uint32_t(m_cache.size()); i < m_size; ++i, ++it) {
			bitSet.set(*it);
		}
	}
}

void
ItemIndexPrivateFoR::putInto(uint32_t* dest) const {
	dest = std::copy(m_cache.cbegin(), m_cache.cend(), dest);
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i = uint32_t(m_cache.size()); i < m_size; ++i, ++dest, ++it) {
			*dest = *it;
		}
	}
}

ItemIndexPrivate *
ItemIndexPrivateFoR::uniteK(const sserialize::ItemIndexPrivate * other, uint32_t /*numItems*/) const {
	return unite(other);
}

ItemIndexPrivate *
ItemIndexPrivateFoR::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateFoR * cother = dynamic_cast<const ItemIndexPrivateFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::IntersectOp,
		detail::ItemIndexImpl::FoRCreator,
		std::unique_ptr<detail::ItemIndexImpl::FoRIterator>
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateFoR::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateFoR * cother = dynamic_cast<const ItemIndexPrivateFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::UniteOp,
		detail::ItemIndexImpl::FoRCreator,
		std::unique_ptr<detail::ItemIndexImpl::FoRIterator>
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateFoR::difference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateFoR * cother = dynamic_cast<const ItemIndexPrivateFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::DifferenceOp,
		detail::ItemIndexImpl::FoRCreator,
		std::unique_ptr<detail::ItemIndexImpl::FoRIterator>
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateFoR::symmetricDifference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateFoR * cother = dynamic_cast<const ItemIndexPrivateFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doSymmetricDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::SymmetricDifferenceOp,
		detail::ItemIndexImpl::FoRCreator,
		std::unique_ptr<detail::ItemIndexImpl::FoRIterator>
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateFoR::fromBitSet(const DynamicBitSet & bitSet) {
	sserialize::UByteArrayAdapter tmp(UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY));
	create(bitSet.cbegin(), bitSet.cend(), tmp);
	tmp.resetPtrs();
	return new ItemIndexPrivateFoR(tmp);
}

uint32_t ItemIndexPrivateFoR::blockSizeOffset() const {
	return m_bits.at(0);
}

uint32_t ItemIndexPrivateFoR::blockSize() const {
	return ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset()];
}

uint32_t ItemIndexPrivateFoR::blockCount() const {
	return m_size/blockSize();
}
	
}//end namespace
