#ifndef ITEM_INDEX_PRIVATE_NATIVE_H
#define ITEM_INDEX_PRIVATE_NATIVE_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/utility/checks.h>
#include <string.h>

namespace sserialize {
namespace detail {
namespace ItemIndexPrivate {

/** Layout:
  *
  * ----------------------------------------------
  * Size|Ids*
  * ----------------------------------------------
  *  u32|uint32_t in native format
  *
  */

class ItemIndexPrivateNative final: public sserialize::ItemIndexPrivate {
public:
	ItemIndexPrivateNative();
	ItemIndexPrivateNative(const sserialize::UByteArrayAdapter & data);
	virtual ~ItemIndexPrivateNative();
	virtual sserialize::ItemIndex::Types type() const override;
	
public:
	///uses at(pos) by default
	virtual uint32_t uncheckedAt(uint32_t pos) const override;
	///checked at
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;

	virtual uint32_t size() const override;

	///return the mean bit per number including header
	virtual uint8_t bpn() const override;
	

	virtual const_iterator cbegin() const override;
	virtual const_iterator cend() const override;
	
	virtual void putInto(sserialize::DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t* dest) const override;
	
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;
	
    virtual UByteArrayAdapter data() const override;
	
	virtual sserialize::ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual sserialize::ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual sserialize::ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual sserialize::ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
	
	template<typename T_UINT32_ITERATOR>
	static bool create(T_UINT32_ITERATOR begin, const T_UINT32_ITERATOR & end, UByteArrayAdapter & dest);
	
	template<typename T_SORTED_CONTAINER>
	static bool create(const T_SORTED_CONTAINER & src, UByteArrayAdapter & dest) {
		return create(src.cbegin(), src.cend(), dest);
	}
private:
	class MyIterator: public ItemIndexPrivate::const_iterator_base_type {
	public:
		MyIterator(const MyIterator & other) = default;
		MyIterator(uint8_t const * data);
		virtual ~MyIterator();
		virtual uint32_t get() const override;
		virtual void next() override;
		virtual bool notEq(const ItemIndexPrivate::const_iterator_base_type * other) const override;
		virtual bool eq(const ItemIndexPrivate::const_iterator_base_type * other) const override;
		virtual ItemIndexPrivate::const_iterator_base_type * copy() const override;
	private:
		uint8_t const * m_data;
	};
private:
	inline uint32_t uncheckedAt(uint32_t pos, const uint8_t * data) const {
		data += sizeof(uint32_t)*pos;
		uint32_t res;
		::memmove(&res, data, sizeof(uint32_t));
		return res;
	}
	
	template<typename TFunc>
	sserialize::ItemIndexPrivate * genericSetOp(const ItemIndexPrivateNative * other) const;
private:
	uint32_t m_size;
	UByteArrayAdapter::MemoryView m_dataMem;

};

template<typename T_UINT32_ITERATOR>
bool ItemIndexPrivateNative::create(T_UINT32_ITERATOR begin, const T_UINT32_ITERATOR & end, UByteArrayAdapter & dest) {
	using std::distance;
	uint32_t size = narrow_check<uint32_t>(distance(begin, end));
	uint8_t * tmp = new uint8_t[sizeof(uint32_t)*size];
	uint8_t * tmpPtr = tmp;
	for(; begin != end; ++begin, tmpPtr += sizeof(uint32_t)) {
		uint32_t src = *begin;
		memmove(tmpPtr, &src, sizeof(uint32_t));
	}
	dest.putUint32(size);
	dest.putData((uint8_t*)tmp, sizeof(uint32_t)*size);
	delete[] tmp;
	return true;
}

template<typename TFunc>
sserialize::ItemIndexPrivate * ItemIndexPrivateNative::genericSetOp(const ItemIndexPrivateNative * cother) const {
	size_t maxResultSize = TFunc::maxSize(this, cother);
	sserialize::MmappedMemory<uint8_t> mm((maxResultSize+1)*sizeof(uint32_t), MM_PROGRAM_MEMORY);
	
	uint8_t * tmpResultIt = mm.begin()+sizeof(uint32_t);
	
	const uint8_t * myD = m_dataMem.begin();
	const uint8_t * myDEnd = m_dataMem.end();
	const uint8_t * oD = cother->m_dataMem.begin();
	const uint8_t * oDEnd = cother->m_dataMem.end();
	for( ;myD < myDEnd && oD < oDEnd; ) {
		uint32_t myId, oId;
		::memmove(&myId, myD, sizeof(uint32_t));
		::memmove(&oId, oD, sizeof(uint32_t));
		if (myId < oId) {
			if (TFunc::pushFirstSmaller) {
				::memmove(tmpResultIt, myD, sizeof(uint32_t));
				tmpResultIt += sizeof(uint32_t);
			}
			myD += sizeof(uint32_t);
		}
		else if (oId < myId) {
			if (TFunc::pushSecondSmaller) {
				::memmove(tmpResultIt, oD, sizeof(uint32_t));
				tmpResultIt += sizeof(uint32_t);
			}
			oD += sizeof(uint32_t);
		}
		else {
			if (TFunc::pushEqual) {
				::memmove(tmpResultIt, myD, sizeof(uint32_t));
				tmpResultIt += sizeof(uint32_t);
			}
			oD += sizeof(uint32_t);
			myD += sizeof(uint32_t);
		}
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpResultIt, mm.cend());
	}
	if (TFunc::pushFirstRemainder && myDEnd-myD > 0) {
		size_t remainderSize = (size_t)(myDEnd-myD);
		::memmove(tmpResultIt, myD, remainderSize);
		tmpResultIt += remainderSize;
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpResultIt, mm.cend());
	}
	else if (TFunc::pushSecondRemainder && oDEnd-oD > 0) {
		size_t remainderSize = (size_t)(oDEnd - oD);
		::memmove(tmpResultIt, oD, remainderSize);
		tmpResultIt += remainderSize;
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpResultIt, mm.cend());
	}
	
	uint32_t tmpResultSize = narrow_check<uint32_t>((uint64_t)(tmpResultIt - (mm.begin()+sizeof(uint32_t)))/sizeof(uint32_t));
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpResultSize, maxResultSize);
	
	if (!tmpResultSize) {
		return new ItemIndexPrivateEmpty();
	}
	
	mm.resize((tmpResultSize+1)*sizeof(uint32_t));
	
	UByteArrayAdapter tmpD(mm);
	tmpD.putUint32(0, tmpResultSize);
	return new ItemIndexPrivateNative(tmpD);
}

class ItemIndexNativeCreator final {
public:
	ItemIndexNativeCreator(uint32_t maxSize);
	~ItemIndexNativeCreator();
	uint32_t size() const;
	///push only in ascending order (id need to be unique and larger than the one before! otherwise this will eat your kitten!
	void push_back(uint32_t id);
	///you should not push anything after calling this function
	void flush();
	///you should not push anything after calling this function
	ItemIndex getIndex();
	///you should not push anything after calling this function
	sserialize::ItemIndexPrivate * getPrivateIndex();
	UByteArrayAdapter data();
private:
	sserialize::MmappedMemory<uint8_t> m_mem;
	uint8_t * m_it;
};

}}}//end namespace

#endif
