#ifndef SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#define SSERIALIZE_MULTI_BIT_BACK_INSERTER_H
#include <array>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {

class MultiBitBackInserter final {
	UByteArrayAdapter * m_data;
	bool m_delete;
	uint8_t m_curInByteOffset;
	std::array<uint8_t, 9> m_buffer;
public:
	MultiBitBackInserter(const MultiBitBackInserter &) = delete;
	MultiBitBackInserter & operator=(const MultiBitBackInserter &) = delete;
public:
	MultiBitBackInserter();
	MultiBitBackInserter(UByteArrayAdapter & data);
	~MultiBitBackInserter();
	void push_back(uint64_t value, uint8_t length);
	void flush();
	UByteArrayAdapter & data();
};

}//end namespace

#endif