#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
#include <sserialize/utility/Bitpacking.h>
#include <numeric>

namespace sserialize {
	
class ItemIndexPrivateFoR;

namespace detail {
namespace ItemIndexImpl {

class FoRBlock final {
public:
	typedef std::vector<uint32_t>::const_iterator const_iterator;
public:
	FoRBlock();
	FoRBlock(const FoRBlock&) = default;
	FoRBlock(FoRBlock&&) = default;
	explicit FoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
	~FoRBlock() = default;
	FoRBlock & operator=(const FoRBlock &) = default;
	FoRBlock & operator=(FoRBlock &&) = default;
	uint32_t size() const;
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
	void update(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
	uint32_t front() const;
	uint32_t back() const;
	uint32_t at(uint32_t pos) const;
	uint32_t operator[](uint32_t pos) const;
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator end() const;
	const_iterator cend() const;
private:
	SizeType decodeBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
private:
	std::vector<uint32_t> m_values;
	sserialize::UByteArrayAdapter::SizeType m_dataSize;
};

class FoRIterator final: public detail::AbstractArrayIterator<uint32_t>{
public:
	using MyBaseClass = detail::AbstractArrayIterator<uint32_t>;
public:
	FoRIterator (const FoRIterator  &) = default;
	FoRIterator (FoRIterator  &&) = default;
	virtual ~FoRIterator () override;
public:
	virtual value_type get() const override;
	virtual void next() override;
	virtual bool notEq(const MyBaseClass * other) const override;
	virtual bool eq(const MyBaseClass * other) const override;
	virtual MyBaseClass * copy() const override;
private:
	friend class sserialize::ItemIndexPrivateFoR;
private:
	///begin iterator
	explicit FoRIterator (uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data);
	///end iterator
	explicit FoRIterator (uint32_t idxSize);
	
	bool fetchBlock(const sserialize::UByteArrayAdapter& d, uint32_t prev);
	uint32_t blockCount() const;
private:
	sserialize::UByteArrayAdapter m_data;
	sserialize::CompactUintArray m_bits;
	uint32_t m_indexPos;
	uint32_t m_indexSize;
	uint32_t m_blockPos;
	FoRBlock m_block;
};

class FoRCreator {
public:
	using BlockCache = std::vector<uint32_t>;
	typedef enum {
		OO_NONE, //uses 32 Bits for blocks
		OO_BLOCK_BITS, //uses default block size with optimum block bits
		OO_BLOCK_SIZE //uses optimum block size and block bits
	} OptimizationOptions;
public:
	FoRCreator(const FoRCreator& other) = delete;
	FoRCreator & operator=(const FoRCreator & other) = delete;
public:
	FoRCreator();
	FoRCreator(uint32_t blockSizeOffset);
	FoRCreator(UByteArrayAdapter & data, uint32_t blockSizeOffset);
	FoRCreator(FoRCreator && other);
	virtual ~FoRCreator();
public:
	uint32_t size() const;
	void push_back(uint32_t id);
	void flush();
	
	UByteArrayAdapter flushedData() const;
	///flush needs to be called before
	ItemIndex getIndex();
	///flush needs to be called before
	sserialize::ItemIndexPrivate * getPrivateIndex();
public:
	///begin->end are delta values
	template<typename T_ITERATOR>
	static void encodeBlock(UByteArrayAdapter& dest, T_ITERATOR begin, T_ITERATOR end, uint32_t bits);

	///begin->end are delta values
	template<typename T_ITERATOR>
	static uint32_t optBlockSize(T_ITERATOR begin, T_ITERATOR end);	

	///begin->end are absolute values
	template<typename T_ITERATOR, int T_OPTIMIZATION_OPTIONS = OO_BLOCK_SIZE>
	static bool create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter& dest);
private:
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;
	void flushBlock();
private:
	uint32_t m_size;
	uint32_t m_blockSizeOffset;
	//holds delta values!
	BlockCache m_values;
	uint32_t m_vpos;
	uint32_t m_prev;
	uint32_t m_vor; //all values of m_values "ored"
	//stuff for flush
	std::vector<uint8_t> m_blockBits;
	UByteArrayAdapter m_data;
	UByteArrayAdapter * m_dest;
	sserialize::UByteArrayAdapter::OffsetType m_putPtr;
	bool m_delete;
};


}} //end namespace detail::ItemIndexImpl

//A Frame of Reference coding, uses the same Storage layout as ItemIndexPrivateFoR, with the exception that outlieres are not present
class ItemIndexPrivateFoR: public ItemIndexPrivate {
public:
	ItemIndexPrivateFoR(sserialize::UByteArrayAdapter d);
	virtual ~ItemIndexPrivateFoR();
	
	virtual ItemIndex::Types type() const override;
	
	virtual UByteArrayAdapter data() const override;
public:
	///load all data into memory (only usefull if the underlying storage is not contigous)
	virtual void loadIntoMemory() override;

	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual const_iterator cbegin() const override;
	virtual const_iterator cend() const override;

	virtual uint32_t size() const override;
	
	virtual uint8_t bpn() const override;
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;

	virtual bool is_random_access() const override;

	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t* dest) const override;

	///Default uniteK uses unite
	virtual ItemIndexPrivate * uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const override;
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
public:
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet, sserialize::ItemIndex::CompressionLevel cl = sserialize::ItemIndex::CL_DEFAULT);
	///create new index beginning at dest.tellPutPtr()
	template<typename T_ITERATOR>
	static bool create(T_ITERATOR begin, const T_ITERATOR&  end, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndex::CompressionLevel cl = sserialize::ItemIndex::CL_DEFAULT);
	///create new index beginning at dest.tellPutPtr()
	template<typename TSortedContainer>
	static bool create(const TSortedContainer & src, UByteArrayAdapter & dest, sserialize::ItemIndex::CompressionLevel cl = sserialize::ItemIndex::CL_DEFAULT);
public:
	uint32_t blockSizeOffset() const;
	uint32_t blockSize() const;
	uint32_t blockCount() const;
private:
	template<typename TFunc>
	sserialize::ItemIndexPrivate * genericSetOp(const ItemIndexPrivateFoR * cother) const;
private:
	UByteArrayAdapter m_d;
	uint32_t m_size;
	UByteArrayAdapter m_blocks;
	CompactUintArray m_bits;
	mutable AbstractArrayIterator<uint32_t> m_it;
	mutable std::vector<uint32_t> m_cache;
};

}//end namespace

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {
	
	
template<typename T_ITERATOR>
void FoRCreator::encodeBlock(UByteArrayAdapter& dest, T_ITERATOR it, T_ITERATOR end, uint32_t bits) {
	using std::distance;
	uint32_t dist = distance(it, end);
	
	if (dist >= 64) {
		auto blockSizeInBytes = CompactUintArray::minStorageBytes(bits, dist);
		dest.reserveFromPutPtr(blockSizeInBytes);
		auto memv = dest.getMemView(dest.tellPutPtr(), blockSizeInBytes);
		auto dit = memv.data();
		uint32_t count = dist;
		const uint32_t * sit = &(*it);
		sserialize::BitpackingInterface::instance(bits)->pack_blocks(sit, dit, count);
		uint32_t consumed = dist-count;
		memv.flush();
		dest.incPutPtr(dit - memv.data());
		if (count) {
			MultiBitBackInserter dvit(dest);
			dvit.push_back(it+consumed, end, bits);
			dvit.flush();
		}
	}
	else {
		MultiBitBackInserter dvit(dest);
		dvit.push_back(it, end, bits);
		dvit.flush();
	}
}

template<typename T_ITERATOR, int T_OPTIMIZATION_OPTIONS>
bool FoRCreator::create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter & dest) {
	if (begin == end) {
		dest.putVlPackedUint32(0);
		dest.putVlPackedUint32(0);
		uint32_t blockSizeOffset = 0;
		CompactUintArray::create(&blockSizeOffset, (&blockSizeOffset)+1, dest, ItemIndexPrivatePFoR::BlockDescBitWidth);
		return true;
	}

	std::vector<uint32_t> dv(begin, end);
	uint32_t blockSizeOffset = 0;
	uint32_t blockDataStorageSize = std::numeric_limits<uint32_t>::max();
	if (dv.size() > 1) {
		for(std::size_t i(dv.size()-1); i > 0; --i) {
			dv[i] = dv[i] - dv[i-1];
			SSERIALIZE_ASSERT_SMALLER(uint32_t(0), dv[i]);
		}
	}
	if (T_OPTIMIZATION_OPTIONS == int(OO_BLOCK_SIZE)) {
		for(std::size_t i(0); i < PFoRCreator::BlockSizeTestOrder.size(); ++i) {
			uint32_t mbso = PFoRCreator::BlockSizeTestOrder[i];
			uint32_t mbs = ItemIndexPrivatePFoR::BlockSizes[mbso];
			uint32_t mbdss = 0;
			for(auto dvit(dv.begin()), dvend(dv.end()); dvit < dvend && mbdss < blockDataStorageSize; dvit += mbs) {
				uint32_t cbs = std::min<uint32_t>(mbs, dvend-dvit);
				uint32_t dvor = std::accumulate(dvit, dvit+cbs, uint32_t(0), std::bit_or<uint32_t>());
				uint32_t mbb = CompactUintArray::minStorageBits(dvor);
				mbdss += CompactUintArray::minStorageBytes(mbb, cbs);
			}
			if (mbdss < blockDataStorageSize) {
				blockDataStorageSize = mbdss;
				blockSizeOffset = mbso;
			}
		}
	}
	else {
		blockSizeOffset = sserialize::ItemIndexPrivatePFoR::DefaultBlockSizeOffset;
	}

	uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset];
	std::vector<uint8_t> metadata(dv.size()/blockSize + uint32_t(dv.size()%blockSize>0) + 1);
	metadata.front() = blockSizeOffset;
	dest.putVlPackedUint32(dv.size());
	
	if (T_OPTIMIZATION_OPTIONS == int(OO_BLOCK_SIZE)) {
		dest.putVlPackedUint32(blockDataStorageSize);
		{
			SSERIALIZE_CHEAP_ASSERT_EXEC(auto blockDataBegin = dest.tellPutPtr());
			auto mdit = metadata.begin()+1;
			for(auto dvit(dv.begin()), dvend(dv.end()); dvit < dvend; dvit += blockSize, ++mdit) {
				uint32_t cbs = std::min<uint32_t>(blockSize, dvend-dvit);
				auto blockEnd = dvit+cbs;
				uint32_t blockBits = CompactUintArray::minStorageBits(std::accumulate(dvit, blockEnd, uint32_t(0), std::bit_or<uint32_t>()));
				encodeBlock(dest, dvit, blockEnd, blockBits);
				*mdit = blockBits;
			}
			SSERIALIZE_CHEAP_ASSERT_EQUAL(dest.tellPutPtr()-blockDataBegin, blockDataStorageSize);
		}
	}
	else {
		sserialize::UByteArrayAdapter tmp(0, sserialize::MM_PROGRAM_MEMORY);
		{
			auto mdit = metadata.begin()+1;
			for(auto dvit(dv.begin()), dvend(dv.end()); dvit < dvend; dvit += blockSize, ++mdit) {
				uint32_t cbs = std::min<uint32_t>(blockSize, dvend-dvit);
				auto blockEnd = dvit+cbs;
				uint32_t blockBits;
				if (T_OPTIMIZATION_OPTIONS == int(OO_BLOCK_BITS)) {
					blockBits = CompactUintArray::minStorageBits(std::accumulate(dvit, blockEnd, uint32_t(0), std::bit_or<uint32_t>()));
				}
				else {
					blockBits = 32;
				}
				encodeBlock(tmp, dvit, blockEnd, blockBits);
				*mdit = blockBits;
			}
		}
		
		dest.putVlPackedUint32(tmp.size());
		tmp.resetPtrs();
		dest.putData(tmp);
	}
	//and the block bits
	sserialize::CompactUintArray::create(metadata, dest, ItemIndexPrivatePFoR::BlockDescBitWidth);
	return true;
}

}} //end namespace detail::ItemIndexImpl

template<typename T_ITERATOR>
bool ItemIndexPrivateFoR::create(T_ITERATOR begin, const T_ITERATOR & end, UByteArrayAdapter & dest, sserialize::ItemIndex::CompressionLevel cl) {
	using Creator = detail::ItemIndexImpl::FoRCreator;
	switch(cl) {
	case sserialize::ItemIndex::CL_NONE:
		return Creator::create<T_ITERATOR, Creator::OO_NONE>(begin, end, dest);
	case sserialize::ItemIndex::CL_LOW:
		return Creator::create<T_ITERATOR, Creator::OO_BLOCK_BITS>(begin, end, dest);
	case sserialize::ItemIndex::CL_MID:
	case sserialize::ItemIndex::CL_HIGH:
	default:
		return Creator::create<T_ITERATOR, Creator::OO_BLOCK_SIZE>(begin, end, dest);
	}
	return detail::ItemIndexImpl::FoRCreator::create(begin, end, dest);
}

template<typename TSortedContainer>
bool
ItemIndexPrivateFoR::create(const TSortedContainer & src, UByteArrayAdapter & dest, sserialize::ItemIndex::CompressionLevel cl) {
	return create(src.begin(), src.end(), dest, cl);
}

template<typename TFunc>
sserialize::ItemIndexPrivate * ItemIndexPrivateFoR::genericSetOp(const ItemIndexPrivateFoR * cother) const {
	detail::ItemIndexImpl::PFoRCreator creator;
	
	uint32_t myI = 0;
	uint32_t oI = 0;
	uint32_t myS = size();
	uint32_t oS = cother->size();
	std::unique_ptr<detail::ItemIndexImpl::FoRIterator>myIt{static_cast<detail::ItemIndexImpl::FoRIterator*>(cbegin())};
	std::unique_ptr<detail::ItemIndexImpl::FoRIterator> oIt{static_cast<detail::ItemIndexImpl::FoRIterator*>(cother->cbegin())};
	
	for( ;myI < myS && oI < oS; ) {
		uint32_t myId = myIt->get();
		uint32_t oId = oIt->get();
		if (myId < oId) {
			if (TFunc::pushFirstSmaller) {
				creator.push_back(myId);
			}
			++myI;
			if (myI < myS) {
				myIt->next();
			}
		}
		else if (oId < myId) {
			if (TFunc::pushSecondSmaller) {
				creator.push_back(oId);
			}
			++oI;
			if (oI < oS) {
				oIt->next();
			}
		}
		else {
			if (TFunc::pushEqual) {
				creator.push_back(myId);
			}
			++myI;
			if (myI < myS) {
				myIt->next();
			}
			++oI;
			if (oI < oS) {
				oIt->next();
			}
		}
	}
	if (TFunc::pushFirstRemainder) {
		for(; myI < myS; ++myI, myIt->next()) {
			creator.push_back(myIt->get());
		}
	}
	
	if (TFunc::pushSecondRemainder) {
		for(; oI < oS; ++oI, oIt->next()) {
			creator.push_back(oIt->get());
		}
	}
	
	creator.flush();
	return creator.getPrivateIndex();
}

}

#endif
