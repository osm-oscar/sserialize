#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
#include <sserialize/storage/pack_unpack_functions.h>

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {


PFoRIterator::PFoRIterator(uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data) :
m_data(data),
m_bits(bits),
m_indexPos(0),
m_indexSize(idxSize),
m_blockPos(0),
m_block(data)
{}


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
		if (m_blockPos >= m_block.size() && m_indexPos < m_indexSize) {
			//there is exactly one partial block at the end
			//all blocks before have the same blocksize
			uint32_t defaultBlockSize = ItemIndexPrivatePFoR::BlockSizes.at(m_bits.at(0));
			uint32_t blockSize = std::min<uint32_t>(m_indexSize-m_indexPos, defaultBlockSize);
			uint32_t blockNum = m_indexPos/m_indexSize;
			uint32_t blockBits = m_bits.at(blockNum+1);
			m_block = PFoRBlock(m_data, m_block.back(), blockSize, blockBits);
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

//BEGIN CREATOR

PFoRCreator::PFoRCreator() :
m_fixedSize(false),
m_size(0),
m_blockSizeOffset(ItemIndexPrivatePFoR::BlockSizes[ItemIndexPrivatePFoR::DefaultBlockSizeOffset]),
m_prev(0),
m_blockBits(1, m_blockSizeOffset),
m_data(0, MM_PROGRAM_MEMORY),
m_dest( new UByteArrayAdapter(0, MM_PROGRAM_MEMORY) ),
m_putPtr(0),
m_delete(true)
{
	
}

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
	SSERIALIZE_CHEAP_ASSERT_SMALLER(blockSize, ItemIndexPrivatePFoR::BlockSizes.size());
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
	m_prev = id;
	if (m_values.size() == ItemIndexPrivatePFoR::BlockSizes[m_blockSizeOffset]) {
		flushBlock();
	}
}

void PFoRCreator::flushBlock() {
	if (!m_values.size()) {
		return;
	}
	if (!m_fixedSize) {
		m_size += m_values.size();
	}
	uint32_t blockBits = encodeBlock(m_data, m_values.begin(), m_values.end());
	m_values.clear();
}

void PFoRCreator::flush() {
	if (m_values.size()) {
		flushBlock();
	}
	m_dest->putVlPackedUint32(m_size);
	m_dest->putVlPackedUint32(m_data.size());
	m_dest->putData(m_data);
	CompactUintArray::create(m_blockBits, *m_dest);
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
	return *m_data;
}

const UByteArrayAdapter& PFoRCreator::data() const {
	return *m_data;
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

ItemIndexPrivatePFoR::ItemIndexPrivatePFoR(UByteArrayAdapter d) :
m_d(d)
{
	m_size = d.getVlPackedUint32();
	uint32_t blockDataSize = d.getVlPackedUint32();
	d.shrinkToGetPtr();
	
	m_blocks = UByteArrayAdapter(d, 0, blockDataSize);
	m_bits = CompactUintArray(UByteArrayAdapter(d, blockDataSize), 5);
	
	sserialize::SizeType totalSize = psize_vu32(m_size) + psize_vu32(blockDataSize) +
		m_blocks.size() + CompactUintArray::minStorageBytes(5, blockCount());
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(totalSize, m_d.size());
	if (m_d.size() != totalSize) {
		m_d = UByteArrayAdapter(m_d, 0, totalSize);
	}
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(m_bits.at(0), BlockSizes.size());
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
	const ItemIndexPrivateEliasFano * cother = dynamic_cast<const ItemIndexPrivatePFoR*>(other);
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

uint32_t ItemIndexPrivatePFoR::blockSize() const {
	return BlockSizes[m_bits.at(0)];
}

uint32_t ItemIndexPrivatePFoR::blockCount() const {
	return m_size/blockSize();
}

//END INDEX

}//end namespace sserialize