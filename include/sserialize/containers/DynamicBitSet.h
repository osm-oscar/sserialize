#ifndef SSERIALIZE_DYNAMIC_BIT_SET_H
#define SSERIALIZE_DYNAMIC_BIT_SET_H
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

class ItemIndex;

class DynamicBitSet {
	UByteArrayAdapter m_data;
public:
	///creates a DynamicBitSet with a in-memory cache as backend
	DynamicBitSet();
	///Create DynamicBitSet at the beginning of data
	DynamicBitSet(const sserialize::UByteArrayAdapter & data);
	virtual ~DynamicBitSet();
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
	void unset(SizeType pos);
	ItemIndex toIndex(int type) const;
	
	template<typename T_OUTPUT_ITERATOR>
	void putInto(T_OUTPUT_ITERATOR out) {
		UByteArrayAdapter::OffsetType s = m_data.size();
		UByteArrayAdapter::OffsetType i = 0;
		SizeType id = 0;
		for(; i < s; ++i) {
			uint8_t d = m_data.getUint8(i);
			for(uint32_t curId = id; d; ++curId, d >>= 1) {
				if (d & 0x1) {
					*out = curId;
					++out;
				}
			}
			id += 8;
		}
	}
	
	SizeType size() const;
};

}//end namespace


#include <sserialize/containers/ItemIndex.h>
#endif