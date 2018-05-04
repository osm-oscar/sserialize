#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
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
	uint32_t m_prev;
	uint32_t m_indexPos;
	uint32_t m_indexSize;
	uint32_t m_blockEnd;
	uint32_t m_blockBits;
	MultiBitIterator m_blockIt;
};

class FoRCreator {
public:
	using BlockCache = std::vector<uint32_t>;
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
	template<typename T_ITERATOR>
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
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	///create new index beginning at dest.tellPutPtr()
	template<typename T_ITERATOR>
	static bool create(T_ITERATOR begin, const T_ITERATOR&  end, sserialize::UByteArrayAdapter& dest);
	///create new index beginning at dest.tellPutPtr()
	template<typename TSortedContainer>
	static bool create(const TSortedContainer & src, UByteArrayAdapter & dest);
public:
	uint32_t blockSizeOffset() const;
	uint32_t blockSize() const;
	uint32_t blockCount() const;
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
	MultiBitBackInserter dvit(dest);
	for(; it != end; ++it) {
		dvit.push_back(*it, bits);
	}
	dvit.flush();
}

template<typename T_ITERATOR>
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

	uint32_t blockSize = ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset];
	std::vector<uint8_t> metadata(dv.size()/blockSize + uint32_t(dv.size()%blockSize>0) + 1);
	metadata.front() = blockSizeOffset;
	dest.putVlPackedUint32(dv.size());
	dest.putVlPackedUint32(blockDataStorageSize);
	{
		SSERIALIZE_CHEAP_ASSERT_ASSIGN(auto blockDataBegin, dest.tellPutPtr());
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
	//and the block bits
	sserialize::CompactUintArray::create(metadata, dest, ItemIndexPrivatePFoR::BlockDescBitWidth);
	return true;
}

}} //end namespace detail::ItemIndexImpl

template<typename T_ITERATOR>
bool ItemIndexPrivateFoR::create(T_ITERATOR begin, const T_ITERATOR & end, UByteArrayAdapter & dest) {
	return detail::ItemIndexImpl::FoRCreator::create(begin, end, dest);
}

template<typename TSortedContainer>
bool
ItemIndexPrivateFoR::create(const TSortedContainer & src, UByteArrayAdapter & dest) {
	return create(src.begin(), src.end(), dest);
}

}

#endif
