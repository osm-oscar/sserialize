#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_PFOR_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/iterator/MultiBitIterator.h>
#include <sserialize/iterator/MultiBitBackInserter.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/assert.h>
#include <limits>
#include <array>

namespace sserialize {

class ItemIndexPrivatePFoR;

namespace detail {
namespace ItemIndexImpl {

class PForCreator;

/** A single frame of reference block
  * Data format is as follows:
  * 
  * struct {
  *   MultiBitIterator data;
  *   List< v_unsigned<32> > outliers;
  * }
  * 
  * Since the delta between two successive entries is at least 1,
  * a zero encodes an exception located in the outliers section
  * 
  */

class PFoRBlock final {
public:
	typedef std::vector<uint32_t>::const_iterator const_iterator;
public:
	PFoRBlock();
	PFoRBlock(const PFoRBlock&) = default;
	PFoRBlock(PFoRBlock&&) = default;
	explicit PFoRBlock(const sserialize::UByteArrayAdapter & d, uint32_t prev, uint32_t size, uint32_t bpn);
	~PFoRBlock() = default;
	PFoRBlock & operator=(const PFoRBlock &) = default;
	PFoRBlock & operator=(PFoRBlock &&) = default;
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
public:
	template<typename T_OUTPUT_ITERATOR>
	SizeType decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out);
private:
	std::vector<uint32_t> m_values;
	sserialize::UByteArrayAdapter::SizeType m_dataSize;
};

class PFoRIterator final: public detail::AbstractArrayIterator<uint32_t>{
public:
	using MyBaseClass = detail::AbstractArrayIterator<uint32_t>;
public:
	PFoRIterator(const PFoRIterator &) = default;
	PFoRIterator(PFoRIterator &&) = default;
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
	///begin iterator
	explicit PFoRIterator(uint32_t idxSize, const sserialize::CompactUintArray & bits, const sserialize::UByteArrayAdapter & data);
	///end iterator
	explicit PFoRIterator(uint32_t idxSize);
	
	bool fetchBlock(const sserialize::UByteArrayAdapter& d, uint32_t prev);
	uint32_t blockCount() const;
private:
	sserialize::UByteArrayAdapter m_data;
	sserialize::CompactUintArray m_bits;
	uint32_t m_indexPos;
	uint32_t m_indexSize;
	uint32_t m_blockPos;
	PFoRBlock m_block;
};

class PFoRCreator final {
public:
	using BlockCache = std::vector<uint32_t>;
	struct OptimizerData {
		class Entry {
		public:
			Entry();
			Entry(uint32_t id);
			Entry(const Entry &) = default;
			Entry & operator=(const Entry&) = default;
			uint8_t vsize() const;
			/// bits() == 0 iff id == 0, otherwise bits() == CompactUintArray::minStorageBits(id)
			uint8_t bits() const;
		private:
			uint8_t m_bits;
		};
		std::vector<Entry> entries;
		
		OptimizerData() = default;
		OptimizerData(const OptimizerData &) = default;
		OptimizerData(OptimizerData &&) = default;
		OptimizerData & operator=(const OptimizerData&) = default;
		OptimizerData & operator=(OptimizerData &&) = default;
		
		inline std::size_t size() const { return entries.size(); }
		inline std::vector<Entry>::const_iterator begin() const { return entries.begin(); }
		inline std::vector<Entry>::const_iterator end() const { return entries.end(); }
		
		template<bool T_ABSOLUTE, typename T_ITERATOR>
		void init(T_ITERATOR begin, T_ITERATOR end);
	};
public:
	PFoRCreator(const PFoRCreator& other) = delete;
	PFoRCreator & operator=(const PFoRCreator & other) = delete;
public:
	PFoRCreator();
	PFoRCreator(uint32_t blockSizeOffset);
	PFoRCreator(UByteArrayAdapter & data, uint32_t blockSizeOffset);
	PFoRCreator(UByteArrayAdapter & data, uint32_t finalSize, uint32_t blockSizeOffset);
	PFoRCreator(PFoRCreator && other);
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
	
	///begin->end are delta values, data after dest.tellPutPtr()
	template<typename T_ITERATOR, typename T_OD_ITERATOR>
	static uint32_t encodeBlock(sserialize::UByteArrayAdapter& dest, T_ITERATOR begin, T_ITERATOR end, T_OD_ITERATOR odbegin, T_OD_ITERATOR odend);

	///begin->end are delta values
	template<typename T_ITERATOR>
	static sserialize::SizeType storageSize(T_ITERATOR begin, T_ITERATOR end, uint32_t bits);
	
	///begin->end are delta values
	template<typename T_ITERATOR>
	static void optBits(T_ITERATOR begin, T_ITERATOR end, uint32_t & optBits, uint32_t & optStorageSize);
	
	///begin->end are iterators to OptimizerData::Entry
	template<typename T_ITERATOR>
	static void optBitsOD(T_ITERATOR begin, T_ITERATOR end, uint32_t & optBits, uint32_t & optStorageSize);
	
	///begin->end are absolute values
	template<typename T_ITERATOR>
	static bool create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter& dest);
	
	static void optBlockCfg(const OptimizerData & od, uint32_t & optBlockSizeOffset, uint32_t & optBlockStorageSize);
	
private:
	static const std::array<uint32_t, 32> BlockSizeTestOrder;
private:
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;
	void flushBlock();
private:
	bool m_fixedSize;
	uint32_t m_size;
	uint32_t m_blockSizeOffset;
	//holds delta values!
	BlockCache m_values;
	std::vector<OptimizerData::Entry> m_od;
	uint32_t m_prev;
	std::vector<uint8_t> m_blockBits;
	UByteArrayAdapter m_data;
	UByteArrayAdapter * m_dest;
	sserialize::UByteArrayAdapter::OffsetType m_putPtr;
	bool m_delete;
};


}} //end namespace detail::ItemIndexImpl

/** Default format is:
  * 
  * struct {
  *     v_unsigned<32> size; //number of entries
  *     v_unsigned<32> dataSize; //size of the blockData section
  *     List<PFoRBlock> blocks; //pfor blocks
  *     //first entry is the size of a block given as offset into ItemIndexPrivatePFoR::BlockSizes
  *     //following entries encode the bit size of a block
  *     CompactUintArray<5> blockDesc;
  * }
  * 
  **/

class ItemIndexPrivatePFoR: public ItemIndexPrivate {
public:
	//maximum number of entries a single block may hold
	static const uint32_t DefaultBlockSizeOffset;
	///Available block sizes. Note that these are NOT ordered
	static const std::array<const uint32_t, 32> BlockSizes;
	static constexpr uint32_t BlockDescBitWidth = 5;
public:
	ItemIndexPrivatePFoR(sserialize::UByteArrayAdapter d);
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


template<typename T_OUTPUT_ITERATOR>
sserialize::SizeType PFoRBlock::decodeBlock(sserialize::UByteArrayAdapter d, uint32_t prev, uint32_t size, uint32_t bpn, T_OUTPUT_ITERATOR out) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(UByteArrayAdapter::SizeType(0), d.tellGetPtr());
	sserialize::SizeType getPtr = d.tellGetPtr();
	MultiBitIterator it(d);
	d.incGetPtr(CompactUintArray::minStorageBytes(bpn, size));
	for(uint32_t i(0); i < size; ++i, it += bpn) {
		uint32_t v = it.get32(bpn);
		if (v == 0) {
			v = d.getVlPackedUint32();
		}
		prev += v;
		*out = prev;
		++out;
	}
	return d.tellGetPtr() - getPtr;
}

template<typename T_ITERATOR, typename T_OD_ITERATOR>
uint32_t PFoRCreator::encodeBlock(sserialize::UByteArrayAdapter& dest, T_ITERATOR begin, T_ITERATOR end, T_OD_ITERATOR odbegin, T_OD_ITERATOR odend) {
	using std::distance;
	auto blockSize = distance(odbegin, odend);
	
	SSERIALIZE_CHEAP_ASSERT_EQUAL(std::ptrdiff_t(distance(begin, end)), std::ptrdiff_t(blockSize));
	SSERIALIZE_CHEAP_ASSERT_ASSIGN(auto blockDataBegin, dest.tellPutPtr());
	
	
	uint32_t optBits(32), optStorageSize(0);
	PFoRCreator::optBitsOD(odbegin, odend, optBits, optStorageSize);

	UByteArrayAdapter::OffsetType dvStorageSize = CompactUintArray::minStorageBytes(optBits, blockSize);
	if (!dest.reserveFromPutPtr(dvStorageSize)) {
		throw sserialize::CreationException("Could not allocate storage");
	}
	MultiBitBackInserter dvit(dest);
	std::vector<uint32_t> outliers;
	
	auto odit = odbegin;
	for(T_ITERATOR it(begin); it != end; ++it, ++odit) {
		if (odit->bits() > optBits || *it == 0) {
			outliers.push_back(*it);
			dvit.push_back(0, optBits);
		}
		else {
			SSERIALIZE_CHEAP_ASSERT_SMALLER(uint32_t(0), *it);
			dvit.push_back(*it, optBits);
		}
	}
	dvit.flush();
	for(uint32_t x : outliers) {
		dest.putVlPackedUint32(x);
	}
	
	SSERIALIZE_CHEAP_ASSERT_EQUAL(optStorageSize, dest.tellPutPtr() - blockDataBegin);
	
	#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	{
		UByteArrayAdapter blockData(dest, blockDataBegin);
		PFoRBlock block(blockData, 0, blockSize, optBits);
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(blockSize, block.size());
		uint32_t realId = 0;
		uint32_t i = 0;
		for(auto it(begin); it != end; ++it, ++i) {
			realId += *it;;
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(realId, block.at(i));
		}
	}
	#endif
	
	return optBits;
}

template<bool T_ABSOLUTE, typename T_ITERATOR>
void PFoRCreator::OptimizerData::init(T_ITERATOR begin, T_ITERATOR end) {
	using std::distance;
	std::size_t ds = distance(begin, end);
	entries.resize(ds);
	auto odIt = entries.begin();
	if (T_ABSOLUTE) {
		uint32_t prev = 0;
		for(auto it(begin); it != end; ++it, ++odIt) {
			*odIt = Entry(*it - prev);
			prev = *it;
		}
	}
	else {
		for(auto it(begin); it != end; ++it, ++odIt) {
			*odIt = Entry(*it);
		}
	}
}

template<typename T_ITERATOR>
sserialize::SizeType PFoRCreator::storageSize(T_ITERATOR begin, T_ITERATOR end, uint32_t bits) {
	using std::distance;
	std::size_t dist = distance(begin, end);
	sserialize::SizeType s = CompactUintArray::minStorageBytes(bits, dist);
	for(auto it(begin); it != end; ++it) {
		if ( CompactUintArray::minStorageBits(*it) > bits) {
			s += sserialize::psize_vu32(*it);
		}
	}
	return s;
}

///begin->end are delta values
template<typename T_ITERATOR>
void PFoRCreator::optBits(T_ITERATOR begin, T_ITERATOR end, uint32_t & optBits, uint32_t & optStorageSize) {
	OptimizerData od;
	od.init<false>(begin, end);
	optBitsOD(od.entries.begin(), od.entries.end(), optBits, optStorageSize);
}

template<typename T_IT>
void PFoRCreator::optBitsOD(T_IT begin, T_IT end, uint32_t & optBits, uint32_t & optStorageSize) {
	using std::distance;
	std::size_t inputSize = distance(begin, end);

	if (inputSize < 1) {
		optBits = 0;
		optStorageSize = 0;
		return;
	}
	
	//storageSizes[i] = storage size if i-1 bits are used
	std::array<uint32_t, 33> storageSizes;
	storageSizes.fill(0);
	
	//we know the following: input is > 0 except for begin
	//for increasing bits we have the following
	//the fixed array part strongly increases
	//the variable part decreases
	//we now simply compute for each bit the added amount of storage
	for(auto it(begin); it != end; ++it) {
		storageSizes[it->bits()] += it->vsize();
	}
	
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

template<typename T_ITERATOR>
bool PFoRCreator::create(T_ITERATOR begin, T_ITERATOR end, sserialize::UByteArrayAdapter & dest) {
	SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(begin, end));
	std::vector<uint32_t> dv;
	OptimizerData od;
	uint32_t blockSizeOffset(0), blockStorageSize(0);
	uint32_t blockSize(0), blockCount(0);
	std::vector<uint8_t> metadata;

	{
		using std::distance;
		dv.resize(distance(begin, end), 0);
		uint32_t prev = 0;
		auto dvit = dv.begin();
		for(auto it(begin); it != end; ++it, ++dvit) {
			*dvit = *it - prev;
			prev = *it;
		}
	}
	od.init<false>(dv.begin(), dv.end());

	if (dv.size()) {
		PFoRCreator::optBlockCfg(od, blockSizeOffset, blockStorageSize);
		blockSize = ItemIndexPrivatePFoR::BlockSizes[blockSizeOffset];
		blockCount = dv.size()/blockSize + ((dv.size()%blockSize)>0);
	}
	
	metadata.resize(blockCount+1);
	metadata.front() = blockSizeOffset;
	
	dest.putVlPackedUint32(dv.size()); //idx size
	dest.putVlPackedUint32(blockStorageSize); // block data size
	
	SSERIALIZE_CHEAP_ASSERT_ASSIGN(auto blockDataBegin, dest.tellPutPtr());
	
	if (od.size()) { //now comes the block data
		auto odit = od.begin();
		auto mdit = metadata.begin()+1;
		for(auto dvit(dv.begin()), dvend(dv.end()); dvit < dvend; dvit += blockSize, odit += blockSize, ++mdit) {
			uint32_t myBlockSize = std::min<uint32_t>(blockSize, dvend-dvit);
			uint32_t blockBits = encodeBlock(dest, dvit, dvit+myBlockSize, odit, odit+myBlockSize);
			*mdit = blockBits;
		}
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(blockStorageSize, dest.tellPutPtr() - blockDataBegin);

	sserialize::CompactUintArray::create(metadata, dest, ItemIndexPrivatePFoR::BlockDescBitWidth);
	return true;
}

}} //end namespace detail::ItemIndexImpl

template<typename T_ITERATOR>
bool ItemIndexPrivatePFoR::create(T_ITERATOR begin, const T_ITERATOR & end, UByteArrayAdapter & dest) {
	return detail::ItemIndexImpl::PFoRCreator::create(begin, end, dest);
}

template<typename TSortedContainer>
bool
ItemIndexPrivatePFoR::create(const TSortedContainer & src, UByteArrayAdapter & dest) {
	return create(src.begin(), src.end(), dest);
}

}

#endif