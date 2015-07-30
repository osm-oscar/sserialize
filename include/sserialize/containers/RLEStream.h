#ifndef SSERIALIZE_STATIC_RLE_STREAM_H
#define SSERIALIZE_STATIC_RLE_STREAM_H
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
/**
  * We have to encode in each byte the following information in the first 2 Bits:
  * 0x0 => no rle, positive delta
  * 0x1 => no rle, negative delta
  * 0x2 => no rle, no delta 
  * 0x3 => rle
  * If rle => next codeword is signed
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
		///create checkpoint with value, call flush before to get the offset of the checkpoint
		void checkpoint(uint32_t value);
		///flush current temp data to dest, but keep last inserted value for further delta encoding
		void flush();

	};
public:
	RLEStream();
	RLEStream(const RLEStream & other);
	RLEStream(RLEStream && other);
	RLEStream(const sserialize::UByteArrayAdapter & begin);
	~RLEStream() {}
	RLEStream & operator=(const RLEStream & other);
	RLEStream & operator=(RLEStream && other);
	const UByteArrayAdapter & data() const { return m_d; }
	inline bool hasNext() const { return m_d.getPtrHasNext(); }
	RLEStream & operator++();
	inline uint32_t operator*() const {
		return m_curId;
	}
};

}//end namespace

#endif