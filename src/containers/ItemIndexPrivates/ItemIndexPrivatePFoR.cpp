#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/Bitpacking.h>

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {

PFoRBlock::PFoRBlock() :
m_dataSize(0)
{}

PFoRBlock::PFoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn) :
m_values(size)
{
	m_dataSize = decodeBlock(d, prev, size, bpn);
}

uint32_t PFoRBlock::size() const {
	return m_values.size();
}

sserialize::UByteArrayAdapter::SizeType PFoRBlock::getSizeInBytes() const {
	return m_dataSize;
}

void PFoRBlock::update(const UByteArrayAdapter& d, uint32_t prev, uint32_t size, uint32_t bpn) {
	m_dataSize = decodeBlock(d, prev, size, bpn);
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

//TODO: decode fixed bit part with sse or something like that if blocks are large
//It may already be a lot faster if we can increase the ipc of the code
//The problem is as follows: input are n bit numbers arranged in an array of uint8_t in big endian (most significant byte first)
//pad n bit to 32 and do ntoh on padded bytes

sserialize::SizeType PFoRBlock::decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(UByteArrayAdapter::SizeType(0), d.tellGetPtr());
	m_values.resize(size);
	sserialize::SizeType getPtr = d.tellGetPtr();
	sserialize::SizeType arrStorageSize = CompactUintArray::minStorageBytes(bpn, size);
	if (size < 8) {
		MultiBitIterator ait(UByteArrayAdapter(d, 0, arrStorageSize));
		d.incGetPtr(arrStorageSize);
		for(uint32_t i(0); i < size; ++i, ait += bpn) {
			uint32_t v = ait.get32(bpn);
			if (v == 0) {
				v = d.getVlPackedUint32();
			}
			prev += v;
			m_values[i] = prev;
		}
	}
	else {
		sserialize::UByteArrayAdapter::MemoryView mv(d.getMemView(0, arrStorageSize));
		d.incGetPtr(arrStorageSize);
		const uint32_t mask = sserialize::createMask(bpn);
		const uint8_t * dit = mv.data();
		uint32_t * vit = m_values.data();
		uint32_t mySize = size;
		
		auto unpacker = BitunpackerInterface::unpacker(bpn);
		unpacker->unpack_blocks(dit, vit, mySize);
		SSERIALIZE_NORMAL_ASSERT_EQUAL(std::size_t(vit-m_values.data()), size-mySize);
		
		//parse the remainder
		if (mySize) {
			uint32_t i = 0;
			uint64_t bitsBegin(0);
			uint64_t bitsEnd(bpn);
			for(const uint8_t * dend(mv.end()-8); dit+bitsBegin/8 <= dend; ++i) {
				SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(vit-m_values.data()), size);
				uint64_t buffer;
				uint32_t bb = bitsBegin/8;
				uint32_t rs = (bb+sizeof(buffer))*8 - bitsEnd;
				::memmove(&buffer, dit+bb, 8);
				buffer = be64toh(buffer);
				buffer >>= rs;
				buffer &= mask;
				
				*vit = buffer;
				
				bitsBegin = bitsEnd;
				bitsEnd += bpn;
				++vit;
			}
			for(; i < mySize; ++i) {
				uint64_t buffer;
				
				uint32_t bb = bitsBegin/8;
				uint32_t rs = (bb+sizeof(buffer))*8 - bitsEnd;
				::memmove(&buffer, dit+bb, mv.end() - (dit+bb));
				buffer = be64toh(buffer);
				buffer >>= rs;
				buffer &= mask;
				
				*vit = uint32_t(buffer) & mask;
				
				bitsBegin = bitsEnd;
				bitsEnd += bpn;
				++vit;
			}
		}
		//fetching outliers afterwards is faster
		for(uint32_t i(0); i < size; ++i) {
			uint32_t v = m_values[i];
			if (v == 0) {
				v = d.getVlPackedUint32();
			}
			prev += v;
			m_values[i] = prev;
		}
	}
	return d.tellGetPtr() - getPtr;
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
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<const PFoRIterator*>(other));
	const PFoRIterator * myOther = static_cast<const PFoRIterator*>(other);
	return m_indexPos != myOther->m_indexPos;
}

bool PFoRIterator::eq(const MyBaseClass * other) const {
	SSERIALIZE_CHEAP_ASSERT(dynamic_cast<const PFoRIterator*>(other));
	const PFoRIterator * myOther = static_cast<const PFoRIterator*>(other);
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
		m_block.update(d, prev, blockSize, blockBits);
		m_data += m_block.getSizeInBytes();
		return true;
	}
	return false;
}

uint32_t PFoRIterator::blockCount() const {
	return m_bits.maxCount() - 1;
}


//BEGIN CREATOR

PFoRCreator::OptimizerData::Entry::Entry() :
m_bits(0)
{}

PFoRCreator::OptimizerData::Entry::Entry(uint32_t id) :
m_bits(id == 0 ? uint32_t(0) : CompactUintArray::minStorageBits(id))
{}

uint8_t PFoRCreator::OptimizerData::Entry::vsize() const {
	uint8_t r =  m_bits == 0? 1 : (m_bits/7 + uint8_t((m_bits%7)>0));
	SSERIALIZE_CHEAP_ASSERT_EQUAL(r, sserialize::psize_vu32( bits() == 0 ? 0 : sserialize::createMask(uint32_t(bits())) ));
	return r;
}

uint8_t PFoRCreator::OptimizerData::Entry::bits() const {
	return m_bits;
}

PFoRCreator::PFoRCreator() :
PFoRCreator(ItemIndexPrivatePFoR::DefaultBlockSizeOffset)
{}

PFoRCreator::PFoRCreator(uint32_t blockSizeOffset) :
m_fixedSize(false),
m_size(0),
m_blockSizeOffset(blockSizeOffset),
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
	uint32_t blockBits = encodeBlock(m_data, m_values.begin(), m_od.begin(), m_od.end());
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


const std::array<uint32_t, 32> PFoRCreator::BlockSizeTestOrder{{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31
}};

void PFoRCreator::optBlockCfg(const OptimizerData & od, uint32_t & optBlockSizeOffset, uint32_t & optBlockStorageSize) {

	if (od.size() < 1) {
		optBlockSizeOffset = 0;
		optBlockStorageSize = 0;
		return;
	}

	optBlockSizeOffset = ItemIndexPrivatePFoR::BlockSizes.size()-1;
	uint32_t optStorageSize = std::numeric_limits<uint32_t>::max();
	uint32_t optMetaDataSize = std::numeric_limits<uint32_t>::max();
	
	auto f = [&od, &optBlockSizeOffset,&optStorageSize, &optMetaDataSize](uint32_t blockSizeOffset) {
		uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset];
		if (blockSize >= 2*od.size()) {
			return;
		}
		uint32_t numFullBlocks = od.size()/blockSize;
		uint32_t numPartialBlocks = od.size()%blockSize > 0; // int(false)==0, int(true)==1
		sserialize::SizeType metaDataSize = CompactUintArray::minStorageBytes(ItemIndexPrivatePFoR::BlockDescBitWidth, 1+numFullBlocks+numPartialBlocks);
		sserialize::SizeType storageSize = metaDataSize;
		storageSize += numFullBlocks+numPartialBlocks; //every block occupies at least one Byte
		for(auto it(od.entries.cbegin()), end(od.entries.cend()); it < end && storageSize < optStorageSize; it += blockSize) {
			auto blockEnd = it + std::min<std::ptrdiff_t>(blockSize, end-it);
			uint32_t myOptBlockBits, myBlockStorageSize;
			PFoRCreator::optBitsOD(it, blockEnd, myOptBlockBits, myBlockStorageSize);
			storageSize += myBlockStorageSize-1; //the 1 accounts is needed since we already added 1 Byte for this block above
		}
		if (storageSize < optStorageSize) {
			optBlockSizeOffset = blockSizeOffset;
			optStorageSize = storageSize;
			optMetaDataSize = metaDataSize;
		}
	};
	
	///try to start with a good block size first
	
	for(uint32_t i(0), s(BlockSizeTestOrder.size()); i < s; ++i) {
		f(BlockSizeTestOrder[i]);
	}
	
	optBlockStorageSize = optStorageSize - optMetaDataSize;
}

void PFoRCreator::optBitsDist(std::array< uint32_t, int(33) >& storageSizes, std::size_t inputSize, uint32_t& optBits, uint32_t& optStorageSize) {
	//sum them up from largest to smallest
	storageSizes[32] += storageSizes[0]; 
	for(uint32_t bits(31); bits > 0; --bits) {
		storageSizes[bits] += storageSizes[bits+1];
	}
	
	//now storageSizes[i] = size of var storage if i-1 bits are used
	
	//add the fixed array storage size,
	for(uint32_t bits=2; bits < 33; ++bits) {
		storageSizes[bits] += CompactUintArray::minStorageBytes(bits-1, inputSize);
	}
	
	//now find the minimum
	optStorageSize = std::numeric_limits<uint32_t>::max();
	optBits = 0;
	for(uint32_t bits=2; bits < 33; ++bits) {
		if (storageSizes[bits] < optStorageSize) {
			optStorageSize = storageSizes[bits];
			optBits = bits-1;
		}
	}
	
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), optBits);
	SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), optStorageSize);
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

//BEGIN GenericSetOpExecuterAccessors

template<>
struct GenericSetOpExecuterAccessors< std::unique_ptr<detail::ItemIndexImpl::PFoRIterator> > {
	typedef detail::ItemIndexImpl::PFoRIterator PositionIteratorBase;
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

const std::array<const uint32_t, 32> ItemIndexPrivatePFoR::BlockSizes{{
	1, 2, 4, 6, 8, 12, 16, 20, 24, 28, //10 entries
	32, 48, 64, 96, 128, 192, 256, 384, 512, 768, //10 entries
	1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, //10 entries
	32768, 65536 //2 entries
}};

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
	if (m_cache.size()) {
		bitSet.set(m_cache.cbegin(), m_cache.cend());
		
		if (m_cache.size() < m_size) {
			auto it(m_it);
			for(uint32_t i = uint32_t(m_cache.size()); i < m_size; ++i, ++it) {
				bitSet.set(*it);
			}
		}
	}
	else {
		detail::ItemIndexImpl::PFoRBlock block;
		UByteArrayAdapter bd = m_blocks;
		uint32_t defaultBlockSize = ItemIndexPrivatePFoR::BlockSizes.at(m_bits.at(0));
		uint32_t prev = 0;
		for(uint32_t blockNum(0), s(blockCount()); blockNum < s; ++blockNum) {
			uint32_t blockSize = std::min<uint32_t>(defaultBlockSize, m_size - blockNum*defaultBlockSize);
			uint32_t blockBits = m_bits.at(blockNum+1);
			block.update(bd, prev, blockSize, blockBits);
			bd += block.getSizeInBytes();
			prev = block.back();
			
			bitSet.set(block.begin(), block.end());
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
		std::unique_ptr<detail::ItemIndexImpl::PFoRIterator>
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
		std::unique_ptr<detail::ItemIndexImpl::PFoRIterator>
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
		std::unique_ptr<detail::ItemIndexImpl::PFoRIterator>
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
		std::unique_ptr<detail::ItemIndexImpl::PFoRIterator>
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivatePFoR::fromBitSet(const DynamicBitSet & bitSet, sserialize::ItemIndex::CompressionLevel cl) {
	sserialize::UByteArrayAdapter tmp(UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY));
	ItemIndexPrivatePFoR::create(bitSet.cbegin(), bitSet.cend(), tmp, cl);
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
	return m_bits.maxCount() - 1;
}

//END INDEX

}//end namespace sserialize
