#ifndef SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#define SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#include <array>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {

class MultiBitBackInserter {
	UByteArrayAdapter m_data;
	uint8_t m_curInByteOffset;
	std::array<uint8_t, 9> m_buffer;
public:
	MultiBitBackInserter();
	MultiBitBackInserter(UByteArrayAdapter & data);
	virtual ~MultiBitBackInserter();
	void push_back(uint64_t value, uint8_t length);
	void flush();
	UByteArrayAdapter & data();
};

}//end namespace

#endif