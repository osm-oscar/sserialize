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
	
	DynamicBitSet operator&(const DynamicBitSet & other) const;
	DynamicBitSet operator|(const DynamicBitSet & other) const;
	DynamicBitSet operator-(const DynamicBitSet & other) const;
	DynamicBitSet operator^(const DynamicBitSet & other) const;
	DynamicBitSet operator~() const;
	DynamicBitSet operator!() const;
	
	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data;}

	bool isSet(uint32_t pos) const;
	void set(uint32_t pos);
	void unset(uint32_t pos);
	ItemIndex toIndex(int type) const;
	
	uint32_t size() const;
};

}//end namespace


#include <sserialize/containers/ItemIndex.h>
#endif