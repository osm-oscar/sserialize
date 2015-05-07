#ifndef SSERIALIZE_STATIC_RLE_STREAM_H
#define SSERIALIZE_STATIC_RLE_STREAM_H
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
/**
  * We have to encode in each byte the following information:
  * rle-byte => count is unsigned, next code-word is signed
  * no-rle-byte => remaining bits encode a signed word
  *
  *
  */

class RLEStream final {
private:
	sserialize::UByteArrayAdapter m_d;
	uint32_t m_curRleCount;
	int32_t m_curRleDiff;
	uint32_t m_curId;
public:
	class Creator {
	private:
		UByteArrayAdapter * m_dest;
		uint32_t m_prevId;
		int32_t m_rleDiff;
		uint32_t m_rleCount;
	public:
		///Create stream at dest.tellPutPtr()
		Creator(UByteArrayAdapter & dest);
		~Creator();
		void put(uint32_t value);
		void flush();
	};
public:
	RLEStream(const sserialize::UByteArrayAdapter & begin);
	~RLEStream() {}
	RLEStream & operator++();
	inline uint32_t operator*() const {
		return m_curId;
	}
};

}//end namespace

#endif