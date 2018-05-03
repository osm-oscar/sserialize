#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_FOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h>
#include <numeric>

namespace sserialize {

namespace detail {
namespace ItemIndexImpl {

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

//A simple "wrapper" which disables the p in PFoR i.e. no outlier encoding
class ItemIndexPrivateFoR: public ItemIndexPrivatePFoR {
public:
	ItemIndexPrivateFoR(sserialize::UByteArrayAdapter d);
	virtual ~ItemIndexPrivateFoR();
	virtual ItemIndex::Types type() const override;
public:
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
};

}//end namespace

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {
	
	
template<typename T_ITERATOR>
void FoRCreator::encodeBlock(UByteArrayAdapter& dest, T_ITERATOR it, T_ITERATOR end, uint32_t bits) {
	CompactUintArray::create(it, end, dest, bits);
}

template<typename T_ITERATOR>
bool FoRCreator::create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter & dest) {
	if (begin == end) {
		dest.putVlPackedUint32(0);
		dest.putVlPackedUint32(0);
		return true;
	}

	std::vector<uint32_t> dv(begin, end);
	uint32_t blockSizeOffset = 0;
	uint32_t blockDataStorageSize = std::numeric_limits<uint32_t>::max();
	if (dv.size() > 1) {
		for(std::size_t i(dv.size()-1); i > 1; --i) {
			dv[i] = dv[i] - dv[i-1];
		}
	}
	for(std::size_t i(0); i < PFoRCreator::BlockSizeTestOrder.size(); ++i) {
		uint32_t mbso = PFoRCreator::BlockSizeTestOrder[i];
		uint32_t mbs = ItemIndexPrivateFoR::BlockSizes[mbso];
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

	uint32_t blockSize = ItemIndexPrivateFoR::BlockSizes[blockSizeOffset];
	std::vector<uint8_t> metadata(dv.size()/blockSize +  uint32_t(dv.size()%blockSize>0) + 1);
	metadata.front() = blockSizeOffset;
	dest.putVlPackedUint32(dv.size());
	dest.putVlPackedUint32(blockDataStorageSize);
	{
		auto mdit = metadata.begin()+1;
		for(auto dvit(dv.begin()), dvend(dv.end()); dvit < dvend; dvit += blockSize, ++mdit) {
			uint32_t myBlockSize = std::min<uint32_t>(blockSize, dvend-dvit);
			auto blockEnd = dvit+myBlockSize;
			uint32_t blockBits = CompactUintArray::minStorageBits(std::accumulate(dvit, blockEnd, uint32_t(0), std::bit_or<uint32_t>()));
			CompactUintArray::create(dvit, dvit+myBlockSize, dest, blockBits);
			*mdit = blockBits;
		}
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
