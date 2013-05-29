#ifndef SSERIALIZE_MULTI_BIT_ITERATOR_H
#define SSERIALIZE_MULTI_BIT_ITERATOR_H
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

class MultiBitIterator {
	UByteArrayAdapter m_data;
	uint8_t m_bitOffset;
public:
	MultiBitIterator();
	MultiBitIterator(const UByteArrayAdapter & data);
	virtual ~MultiBitIterator();
	uint32_t get32() const;
	uint64_t get64() const;
	///get the first @len bits with up to 32 bits
	uint32_t get32(uint8_t len) const;
	///get the first @len bits with up to 64 bits
	uint64_t get64(uint8_t len) const;
	///@param bitCount this should not be larger than INT_MAX
	MultiBitIterator & operator+=(uint32_t bitCount);
	void reset();
	bool hasNext() const;
};

}//end namespace


#endif