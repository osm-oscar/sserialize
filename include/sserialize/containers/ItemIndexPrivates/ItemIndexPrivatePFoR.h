#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/iterator/TransformIterator.h>
#include <limits>

namespace sserialize {

class ItemIndexPrivatePFoR;

namespace detail {
namespace ItemIndexImpl {

class PForCreator;

/** A single frame of reference block
  * Data format is as follows:
  * --------------------------------
  * DATA            |OUTLIERS
  * --------------------------------
  * CompactUintArray|vu32*
  * 
  * Since the delta between two successive entries is at least 1,
  * a zero encodes an exception located in the outliers section
  * 
  */

class PFoRBlock final {
public:
	PFoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
	~PFoRBlock();
	uint32_t size() const;
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
	uint32_t min() const;
	uint32_t max() const;
	uint32_t at(uint32_t pos) const;
	uint32_t isOutlier(uint32_t pos) const;
public:
	template<typename T_OUTPUT_ITERATOR>
	   SizeType decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out);
private:
	std::vector<uint32_t> m_values;
	sserialize::UByteArrayAdapter::SizeType m_dataSize;
};

class PFoRIterator: public detail::AbstractArrayIterator<uint32_t> {
public:
	using MyBaseClass = detail::AbstractArrayIterator<uint32_t>;
public:
	virtual ~PFoRIterator() override;
public:
	virtual value_type get() const override;
	virtual void next() override;
	virtual bool notEq(const MyBaseClass * other) const override;
	virtual bool eq(const MyBaseClass * other) const override;
	virtual MyBaseClass * copy() const override;
private:
	friend class sserialize::ItemIndexPrivatePFoR;
private:
	PFoRIterator(uint32_t firstId, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data);
private:
	uint32_t m_firstId;
	uint32_t m_blockPos;
	PFoRBlock m_block;
};

class PFoRCreator final {
public:
	PFoRCreator(const PFoRCreator& other) = delete;
	PFoRCreator & operator=(const PFoRCreator & other) = delete;
public:
	PFoRCreator();
	PFoRCreator(UByteArrayAdapter & data);
	PFoRCreator(UByteArrayAdapter & data, uint32_t size);
	PFoRCreator(PForCreator && other);
	~PFoRCreator();
	uint32_t size() const;
	void push_back(uint32_t id);
	void flush();
	
	UByteArrayAdapter flushedData() const;
	///flush needs to be called before
	ItemIndex getIndex();
	///flush needs to be called before
	sserialize::ItemIndexPrivate * getPrivateIndex();
public:
	template<typename T_ITERATOR>
	static void encodeBlock(sserialize::UByteArrayAdapter& dest, uint32_t prev, T_ITERATOR begin, T_ITERATOR end);

	template<typename T_ITERATOR>
	static sserialize::SizeType storageSize(uint32_t prev, T_ITERATOR begin, T_ITERATOR end, uint32_t bits);
	
	template<typename T_ITERATOR>
	static uint32_t optBits(uint32_t prev, T_ITERATOR begin, T_ITERATOR end);
private:
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;
private:
	bool m_fixedSize;
	std::vector<uint32_t> m_values;
	UByteArrayAdapter * m_data;
	sserialize::UByteArrayAdapter::OffsetType m_putPtr;
	bool m_delete;
};


}} //end namespace detail::ItemIndexImpl

/** Default format is:
  *
  * ----------------------------------------------------------------------
  * SIZE|DATASIZE|BLOCK_BITSIZES  |DATA
  * ----------------------------------------------------------------------
  * vu32|vu32    |CompactUintArray|PFoRBlock*
  * ----------------------------------------------------------------------
  * 
  * where
  * SIZE is the number of entries
  * DATA contains the index encoded in PForBlocks
  * 
  * 
  **/

class ItemIndexPrivatePFoR: public ItemIndexPrivate {
public:
	static constexpr uint32_t DefaultBlockSize = 128;
public:
	ItemIndexPrivatePFoR(const UByteArrayAdapter & d, uint32_t blockSize = DefaultBlockSize);
	virtual ~ItemIndexPrivatePFoR();
	
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
private:
	uint32_t firstId() const;
private:
	UByteArrayAdapter m_d;
	uint32_t m_size;
	uint32_t m_firstId;
	uint32_t m_blockSize;
	mutable AbstractArrayIterator<uint32_t> m_it;
	mutable std::vector<uint32_t> m_cache;
};

}//end namespace

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {


template<typename T_OUTPUT_ITERATOR>
sserialize::SizeType PFoRBlock::decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out) {
	sserialize::SizeType dataSize = 0;
	CompactUintArray arr(d, bpn, size);
	dataSize += arr.getSizeInBytes();
	d.incGetPtr(s);
	for(uint32_t i(0), s(arr.getSizeInBytes(); i < s; ++i) {
		uint32_t v = arr.at(i);
		if (v == 0) {
			int len = -1;
			v = d.getVlPackedUint32(&len);
			SSERIALIZE_CHEAP_ASSERT_SMALLER(0, len);
			dataSize += len;
		}
		prev += v;
		*out = prev;
		++out;
	}
	return dataSize;
}

template<typename T_ITERATOR>
void PFoRCreator::encodeBlock(sserialize::UByteArrayAdapter & dest, uint32_t prev, T_ITERATOR begin, T_ITERATOR end) {
	using std::distance;
	std::size_t ds = distance(begin, end);
	uint32_t optbits = PFoRCreator::optBits(prev, begin, end);
	std::vector<uint32_t> dv;
	std::vector<uint32_t> outliers;
	UByteArrayAdapter tmp(UByteArrayAdapter::createCache(CompactUintArray::minStorageBytes(optbits, ds)), MM_PROGRAM_MEMORY);
	uint32_t myPrev = prev;
	for(T_ITERATOR it(begin); it != end; ++it) {
		uint32_t d = *it - myPrev;
		myPrev = *it;
		if (CompactUintArray::minStorageBits(d) > optbits) {
			dv.push_back(0);
			outliers.push_back(d);
		}
		else {
			SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), d);
			dv.push_back(dv);
		}
	}
	CompactUintArray::create(dv, dest);
	for(uint32_t x : outliers) {
		dest.putVlPackedUint32(x);
	}
}

template<typename T_ITERATOR>
sserialize::SizeType PFoRCreator::storageSize(T_ITERATOR begin, T_ITERATOR end, uint32_t bits) {
	using std::distance;
	std::size_t dist = distance(begin, end);
	sserialize::SizeType s = CompactUintArray::minStorageBytes(bits, dist);
	for(; begin != end; ++begin) {
		if ( CompactUintArray::minStorageBits(*begin) > bits) {
			s += sserialize::psize_vu32(*begin);
		}
	}
	return s;
}

template<typename T_ITERATOR>
uint32_t PFoRCreator::optBits(uint32_t prev, T_ITERATOR begin, T_ITERATOR end) {
	using std::distance;
	std::size_t ds = distance(begin, end);
	
	if (ds < 1) {
		return 0;
	}
	
	std::vector<uint32_t> dv;
	dv.reserve(ds);
	dv.push_back(*begin - prev);
	uint32_t mindv = dv.back();
	uint32_t maxdv = dv.back();
	for(++begin; begin != end; ++begin) {
		dv.push_back(*begin - dv.back());
		mindv = std::max<uint32_t>(mindv, dv.back());
		maxdv = std::max<uint32_t>(maxdv, dv.back());
	}
	
	uint32_t minbits = CompactUintArray::minStorageBits(mindv);
	uint32_t maxbits = CompactUintArray::minStorageBits(maxdv);
	uint32_t optbits = maxbits;
	sserialize::SizeType optsize = std::numeric_limits<sserialize::SizeType>::max();
	for(uint32_t bits(minbits); bits < maxbits; ++bits) {
		auto s = storageSize(dv.begin(), dv.end(), bits);
		if (s < optsize) {
			optsize = s;
			optbits = bits;
		}
	}
	return optbits;
}

}} //end namespace detail::ItemIndexImpl


template<typename T_ITERATOR>
bool ItemIndexPrivatePFoR::create(T_ITERATOR begin, const T_ITERATOR & end, UByteArrayAdapter & dest) {
	SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(begin, end));
	using std::distance;
	detail::ItemIndexImpl::PForCreator creator(dest, distance(begin, end));
	for(; begin != end; ++begin) {
		creator.push(*begin);
	}
	creator.flush();
	return true;
}

template<typename TSortedContainer>
bool
ItemIndexPrivatePFoR::create(const TSortedContainer & src, UByteArrayAdapter & dest) {
	return create(src.begin(), src.end(), dest);
}

}

#endif