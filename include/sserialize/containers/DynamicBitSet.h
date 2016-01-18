#ifndef SSERIALIZE_DYNAMIC_BIT_SET_H
#define SSERIALIZE_DYNAMIC_BIT_SET_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/AbstractArray.h>

namespace sserialize {

class DynamicBitSet;

namespace detail {
namespace DynamicBitSet {

class DynamicBitSetIdIterator: public sserialize::detail::AbstractArrayIterator<SizeType> {
private:
	friend class sserialize::DynamicBitSet;
private:
	const sserialize::DynamicBitSet * m_p;
	UByteArrayAdapter::OffsetType m_off;
	SizeType m_curId;
	uint8_t m_d;
	uint8_t m_curShift;
	///@param p has to be valid
	DynamicBitSetIdIterator(const sserialize::DynamicBitSet * p, SizeType offset);
public:
	DynamicBitSetIdIterator();
	virtual ~DynamicBitSetIdIterator();
	virtual SizeType get() const override;
	virtual void next() override;
	virtual bool notEq(const AbstractArrayIterator<SizeType> * other) const override;
	virtual AbstractArrayIterator<SizeType> * copy() const override;
};


}}

class ItemIndex;

class DynamicBitSet {
public:
	///An iterator the iterates over the set ids
	typedef AbstractArrayIterator<SizeType> const_iterator;
private:
	UByteArrayAdapter m_data;
public:
	///creates a DynamicBitSet with a in-memory cache as backend
	DynamicBitSet();
	///Create DynamicBitSet at the beginning of data
	DynamicBitSet(const sserialize::UByteArrayAdapter & data);
	virtual ~DynamicBitSet();
	void resize(UByteArrayAdapter::OffsetType size);
	///@param shift: number of bytes to align to expressed as aa power of two. i.e. 0 => 1 byte, 1 => 2 bytes, 2 => 4 bytes, 3 => 8 bytes
	bool align(uint8_t shift);
	
	IdType smallestEntry() const;
	IdType largestEntry() const;
	
	DynamicBitSet operator&(const DynamicBitSet & other) const;
	DynamicBitSet operator|(const DynamicBitSet & other) const;
	DynamicBitSet operator-(const DynamicBitSet & other) const;
	DynamicBitSet operator^(const DynamicBitSet & other) const;
	DynamicBitSet operator~() const;
	
	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data;}

	bool isSet(SizeType pos) const;
	void set(SizeType pos);
	template<typename T_IT>
	void set(T_IT begin, T_IT end) {
		for(; begin != end; ++begin) {
			set(*begin);
		}
	}
	void unset(SizeType pos);
	ItemIndex toIndex(int type) const;
	
	template<typename T_OUTPUT_ITERATOR>
	void putInto(T_OUTPUT_ITERATOR out) {
		UByteArrayAdapter::OffsetType s = m_data.size();
		UByteArrayAdapter::OffsetType i = 0;
		SizeType id = 0;
		for(; i < s; ++i) {
			uint8_t d = m_data.getUint8(i);
			for(SizeType curId(id); d; ++curId, d >>= 1) {
				if (d & 0x1) {
					*out = curId;
					++out;
				}
			}
			id += 8;
		}
	}
	
	SizeType size() const;
	
	const_iterator cbegin() const;
	const_iterator cend() const;
};

}//end namespace


#include <sserialize/containers/ItemIndex.h>
#endif