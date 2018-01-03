#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
#include <sserialize/storage/pack_unpack_functions.h>

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {

PFoRBlock::PFoRBlock() :
m_dataSize(0)
{}

PFoRBlock::PFoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn) {
	m_dataSize = decodeBlock(d, prev, size, bpn, std::back_inserter(m_values));
}

uint32_t PFoRBlock::size() const {
	return m_values.size();
}

sserialize::UByteArrayAdapter::SizeType PFoRBlock::getSizeInBytes() const {
	return m_dataSize;
}

uint32_t PFoRBlock::front() const {
	return m_values.front();
}

uint32_t PFoRBlock::back() const {
	return m_values.back();
}

uint32_t PFoRBlock::at(uint32_t pos) const {
	return m_values.at(pos);
}


PFoRBlock::const_iterator PFoRBlock::begin() const {
	return m_values.begin();
}

PFoRBlock::const_iterator PFoRBlock::cbegin() const {
	return m_values.cbegin();
}

PFoRBlock::const_iterator PFoRBlock::end() const {
	return m_values.end();
}

PFoRBlock::const_iterator PFoRBlock::cend() const {
	return m_values.cend();
}

PFoRIterator::PFoRIterator(uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data) :
m_data(data),
m_bits(bits),
m_indexPos(0),
m_indexSize(idxSize),
m_blockPos(0)
{
	if (m_indexPos < m_indexSize) {
		fetchBlock(m_data, 0);
	}
}

PFoRIterator::PFoRIterator(uint32_t idxSize) :
m_indexPos(idxSize),
m_indexSize(idxSize),
m_blockPos(0)
{}

PFoRIterator::~PFoRIterator() {}

PFoRIterator::value_type
PFoRIterator::get() const {
	return m_block.at(m_blockPos);
}

void
PFoRIterator::next() {
	if (m_indexPos < m_indexSize) {
		++m_indexPos;
		++m_blockPos;
		if (m_blockPos >= m_block.size()) {
			fetchBlock(m_data, m_block.back());
			m_blockPos = 0;
		}
	}
	
}

bool
PFoRIterator::notEq(const MyBaseClass * other) const {
	const PFoRIterator * myOther = dynamic_cast<const PFoRIterator*>(other);
	
	if (!myOther) {
		return true;
	}

	return m_indexPos != myOther->m_indexPos;
}

bool PFoRIterator::eq(const MyBaseClass * other) const {
	const PFoRIterator * myOther = dynamic_cast<const PFoRIterator*>(other);
	
	if (!myOther) {
		return false;
	}
	
	return m_indexPos == myOther->m_indexPos;
}

PFoRIterator::MyBaseClass * PFoRIterator::copy() const {
	return new PFoRIterator(*this);
}

bool PFoRIterator::fetchBlock(const UByteArrayAdapter& d, uint32_t prev) {
	if (m_indexPos < m_indexSize) {
		//there is exactly one partial block at the end
		//all blocks before have the same blocksize
		uint32_t defaultBlockSize = ItemIndexPrivatePFoR::BlockSizes.at(m_bits.at(0));
		uint32_t blockNum = m_indexPos/defaultBlockSize;
		uint32_t blockSize = std::min<uint32_t>(defaultBlockSize, m_indexSize - blockNum*defaultBlockSize);
		uint32_t blockBits = m_bits.at(blockNum+1);
		m_block = PFoRBlock(d, prev, blockSize, blockBits);
		m_data += m_block.getSizeInBytes();
		return true;
	}
	return false;
}


//BEGIN CREATOR

PFoRCreator::OptimizerData::Entry::Entry() :
// m_vsize(0),
m_bits(0)
{}

PFoRCreator::OptimizerData::Entry::Entry(uint32_t id) :
// m_vsize(sserialize::psize_vu32(id)),
m_bits(CompactUintArray::minStorageBits(id))
{}

uint8_t PFoRCreator::OptimizerData::Entry::vsize() const {
	return sserialize::psize_vu32(uint32_t(1) << (bits()-1));
}

uint8_t PFoRCreator::OptimizerData::Entry::bits() const {
	return m_bits;
}

PFoRCreator::PFoRCreator() :
m_fixedSize(false),
m_size(0),
m_blockSizeOffset(ItemIndexPrivatePFoR::DefaultBlockSizeOffset),
m_prev(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest( new UByteArrayAdapter(0, MM_PROGRAM_MEMORY) ),
m_putPtr(0),
m_delete(true)
{}

PFoRCreator::PFoRCreator(UByteArrayAdapter & data, uint32_t blockSizeOffset) :
m_fixedSize(false),
m_size(0),
m_blockSizeOffset(blockSizeOffset),
m_prev(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest(&data),
m_putPtr(m_dest->tellPutPtr()),
m_delete(false)
{
	SSERIALIZE_CHEAP_ASSERT_SMALLER(blockSizeOffset, ItemIndexPrivatePFoR::BlockSizes.size());
}

PFoRCreator::PFoRCreator(UByteArrayAdapter & data, uint32_t finalSize, uint32_t blockSizeOffset) :
m_fixedSize(true),
m_size(finalSize),
m_blockSizeOffset(blockSizeOffset),
m_prev(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest(&data),
m_putPtr(m_dest->tellPutPtr()),
m_delete(false)
{
	SSERIALIZE_CHEAP_ASSERT_SMALLER(m_blockSizeOffset, ItemIndexPrivatePFoR::BlockSizes.size());
}

PFoRCreator::PFoRCreator(PFoRCreator&& other) :
m_fixedSize(other.m_fixedSize),
m_size(other.m_size),
m_blockSizeOffset(other.m_blockSizeOffset),
m_values(std::move(other.m_values)),
m_prev(other.m_prev),
m_blockBits(std::move(other.m_blockBits)),
m_data(std::move(other.m_data)),
m_dest(std::move(other.m_dest)),
m_putPtr(other.m_putPtr),
m_delete(other.m_delete)
{
	other.m_dest = 0;
	other.m_delete = false;
}

PFoRCreator::~PFoRCreator() {
	if (m_delete) {
		delete m_dest;
	}
}

uint32_t PFoRCreator::size() const {
	return m_size;
}

void PFoRCreator::push_back(uint32_t id) {
	SSERIALIZE_CHEAP_ASSERT(m_prev == 0 || m_prev < id);
	m_values.push_back(id - m_prev);
	m_od.emplace_back( m_values.back() );
	m_prev = id;
	if (m_values.size() == ItemIndexPrivatePFoR::BlockSizes[m_blockSizeOffset]) {
		flushBlock();
	}
	SSERIALIZE_CHEAP_ASSERT_SMALLER(m_values.size(), ItemIndexPrivatePFoR::BlockSizes[m_blockSizeOffset]);
}

void PFoRCreator::flushBlock() {
	if (!m_values.size()) {
		return;
	}
	if (!m_fixedSize) {
		m_size += m_values.size();
	}
	uint32_t blockBits = encodeBlock(m_data, m_values.begin(), m_values.end(), m_od.begin(), m_od.end());
	m_blockBits.push_back(blockBits);
	m_values.clear();
	m_od.clear();
}

void PFoRCreator::flush() {
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

UByteArrayAdapter PFoRCreator::flushedData() const {
	UByteArrayAdapter d(data());
	d.setGetPtr(m_putPtr);
	return d;
}

ItemIndex PFoRCreator::getIndex() {
	return ItemIndex(flushedData(), ItemIndex::T_PFOR);
}

ItemIndexPrivate * PFoRCreator::getPrivateIndex() {
	return new ItemIndexPrivatePFoR(flushedData());
}

UByteArrayAdapter& PFoRCreator::data() {
	return *m_dest;
}

const UByteArrayAdapter& PFoRCreator::data() const {
	return *m_dest;
}


const std::array<uint32_t, 32> PFoRCreator::BlockSizeTestOrder = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31
};

void PFoRCreator::optBlockCfg(const OptimizerData & od, uint32_t & optBlockSizeOffset, uint32_t & optBlockStorageSize) {

	if (od.size() < 1) {
		optBlockSizeOffset = 0;
		optBlockStorageSize = 0;
		return;
	}

	optBlockSizeOffset = ItemIndexPrivatePFoR::BlockSizes.size()-1;
	optBlockStorageSize = std::numeric_limits<uint32_t>::max();
	
	auto f = [&od, &optBlockSizeOffset,&optBlockStorageSize](uint32_t blockSizeOffset) {
		uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset];
		if (blockSize >= 2*od.size()) {
			return;
		}
		uint32_t numFullBlocks = od.size()/blockSize;
		uint32_t numPartialBlocks = od.size()%blockSize > 0; // int(false)==0, int(true)==1
		sserialize::SizeType storageSize = CompactUintArray::minStorageBytes(ItemIndexPrivatePFoR::BlockDescBitWidth, 1+numFullBlocks+numPartialBlocks);
		storageSize += numFullBlocks+numPartialBlocks; //every block occupies at least one Byte
		for(auto it(od.entries.cbegin()), end(od.entries.cend()); it < end && storageSize < optBlockStorageSize; it += blockSize) {
			auto blockEnd = it + std::min<std::ptrdiff_t>(blockSize, end-it);
			uint32_t myOptBlockBits, myBlockStorageSize;
			PFoRCreator::optBitsOD(it, blockEnd, myOptBlockBits, myBlockStorageSize);
			storageSize += myBlockStorageSize-1; //the 1 accounts is needed since we already added 1 Byte for this block above
		}
		if (storageSize < optBlockStorageSize) {
			optBlockSizeOffset = blockSizeOffset;
			         optBlockStorageSize = storageSize;
		}
	};
	
	///try to start with a good block size first
	
	for(uint32_t i(0), s(BlockSizeTestOrder.size()); i < s; ++i) {
		f(BlockSizeTestOrder[i]);
	}
}


//END CREATOR

//BEGIN GenericSetOpExecuterInit

template<>
struct GenericSetOpExecuterInit<PFoRCreator, sserialize::detail::ItemIndexImpl::IntersectOp> {
	using Creator = PFoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::IntersectOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<PFoRCreator, sserialize::detail::ItemIndexImpl::UniteOp> {
	using Creator = PFoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::UniteOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<PFoRCreator, sserialize::detail::ItemIndexImpl::DifferenceOp> {
	using Creator = PFoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::DifferenceOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

template<>
struct GenericSetOpExecuterInit<PFoRCreator, sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp> {
	using Creator = PFoRCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp;
	
	static Creator init(const sserialize::ItemIndexPrivate*, const sserialize::ItemIndexPrivate*) {
		return Creator();
	}
};

//END GenericSetOpExecuterInit

}} //end namespace detail::ItemIndexImpl

//BEGIN INDEX

const std::array<const uint32_t, 32> ItemIndexPrivatePFoR::BlockSizes = {
	1, 2, 4, 6, 8, 12, 16, 20, 24, 28, //10 entries
	32, 48, 64, 96, 128, 192, 256, 384, 512, 768, //10 entries
	1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, //10 entries
	32768, 65536 //2 entries
};

const uint32_t ItemIndexPrivatePFoR::DefaultBlockSizeOffset = 14;

constexpr uint32_t ItemIndexPrivatePFoR::BlockDescBitWidth;

ItemIndexPrivatePFoR::ItemIndexPrivatePFoR(UByteArrayAdapter d) :
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

ItemIndexPrivatePFoR::~ItemIndexPrivatePFoR() {}

ItemIndex::Types ItemIndexPrivatePFoR::type() const {
	return ItemIndex::T_PFOR;
}

void
ItemIndexPrivatePFoR::loadIntoMemory() {}

UByteArrayAdapter ItemIndexPrivatePFoR::data() const {
    return m_d;
}

uint8_t ItemIndexPrivatePFoR::bpn() const {
	if (size()) {
		return sserialize::multiplyDiv64(getSizeInBytes(), 8, size());
	}
	else {
		return std::numeric_limits<uint8_t>::max();
	}
}

uint32_t
ItemIndexPrivatePFoR::at(uint32_t pos) const {
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
ItemIndexPrivatePFoR::first() const {
	return at(0);
}

uint32_t
ItemIndexPrivatePFoR::last() const {
	return at(m_size-1);
}

ItemIndexPrivatePFoR::const_iterator
ItemIndexPrivatePFoR::cbegin() const {
	return new detail::ItemIndexImpl::PFoRIterator(size(), m_bits, m_blocks);
}

ItemIndexPrivatePFoR::const_iterator
ItemIndexPrivatePFoR::cend() const {
	return new detail::ItemIndexImpl::PFoRIterator(size());
}

uint32_t
ItemIndexPrivatePFoR::size() const {
	return m_size;
}

sserialize::UByteArrayAdapter::SizeType
ItemIndexPrivatePFoR::getSizeInBytes() const {
	return m_d.size();
}

bool
ItemIndexPrivatePFoR::is_random_access() const {
	return false;
}

void
ItemIndexPrivatePFoR::putInto(DynamicBitSet & bitSet) const {
	bitSet.set(m_cache.cbegin(), m_cache.cend());
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i = uint32_t(m_cache.size()); i < m_size; ++i, ++it) {
			bitSet.set(*it);
		}
	}
}

void
ItemIndexPrivatePFoR::putInto(uint32_t* dest) const {
	dest = std::copy(m_cache.cbegin(), m_cache.cend(), dest);
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i = uint32_t(m_cache.size()); i < m_size; ++i, ++dest, ++it) {
			*dest = *it;
		}
	}
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::uniteK(const sserialize::ItemIndexPrivate * other, uint32_t /*numItems*/) const {
	return unite(other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivatePFoR * cother = dynamic_cast<const ItemIndexPrivatePFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::IntersectOp,
		detail::ItemIndexImpl::PFoRCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivatePFoR * cother = dynamic_cast<const ItemIndexPrivatePFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::UniteOp,
		detail::ItemIndexImpl::PFoRCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::difference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivatePFoR * cother = dynamic_cast<const ItemIndexPrivatePFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::DifferenceOp,
		detail::ItemIndexImpl::PFoRCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::symmetricDifference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivatePFoR * cother = dynamic_cast<const ItemIndexPrivatePFoR*>(other);
	if (!cother) {
		return ItemIndexPrivate::doSymmetricDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::SymmetricDifferenceOp,
		detail::ItemIndexImpl::PFoRCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::fromBitSet(const DynamicBitSet & bitSet) {
	sserialize::UByteArrayAdapter tmp(UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY));
	create(bitSet.cbegin(), bitSet.cend(), tmp);
	tmp.resetPtrs();
	return new ItemIndexPrivatePFoR(tmp);
}

uint32_t ItemIndexPrivatePFoR::blockSizeOffset() const {
	return m_bits.at(0);
}

uint32_t ItemIndexPrivatePFoR::blockSize() const {
	return BlockSizes[blockSizeOffset()];
}

uint32_t ItemIndexPrivatePFoR::blockCount() const {
	return m_size/blockSize();
}

//END INDEX

}//end namespace sserialize