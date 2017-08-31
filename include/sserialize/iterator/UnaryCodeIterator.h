#ifndef SSERIALIZE_UNARY_CODE_ITERATOR_H
#define SSERIALIZE_UNARY_CODE_ITERATOR_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/storage/SerializationInfo.h>

namespace sserialize {

/** Simple unary code:
  * A number n is encoded as n zeros followed by a 1:
  * 4 -> 00001
  *
  * A stream is split into chunks with the least-significant bit to the right
  *
  */
  
class UnaryCodeCreator;

class UnaryCodeIterator: std::iterator<std::input_iterator_tag, uint32_t> {
public:
	UnaryCodeIterator();
	UnaryCodeIterator(const sserialize::UByteArrayAdapter & d);
	UnaryCodeIterator(const UnaryCodeIterator & other) = default;
	~UnaryCodeIterator();
public:
	value_type operator*() const;
	UnaryCodeIterator & operator++();
public:
	bool operator=(UnaryCodeIterator const & other) const;
	bool operator!=(UnaryCodeIterator const & other) const;
private:
	friend class UnaryCodeCreator;
private:
	using chunk_type = uint8_t;
	static constexpr uint32_t chunk_bits = std::numeric_limits<chunk_type>::digits;
	static constexpr uint32_t chunk_size = SerializationInfo<chunk_type>::length;
	static constexpr chunk_type chunk_max_bit = chunk_type(1) << (chunk_bits-1);
private:
	void loadNextChunk();
private:
	UByteArrayAdapter m_d;
	UByteArrayAdapter::SizeType m_pos; //always points to the next entry
	value_type m_last;
	chunk_type m_lastChunk;
	chunk_type m_chunkBitPtr;
};

class UnaryCodeCreator {
public:
	UnaryCodeCreator(UByteArrayAdapter & d);
	~UnaryCodeCreator();
	void put(UnaryCodeIterator::value_type v);
	template<typename T_ITERATOR>
	void put(T_ITERATOR begin, const T_ITERATOR & end) {
		for(; begin != end; ++begin) {
			put(*begin);
		}
	}
	void flush();
private:
	using chunk_type = UnaryCodeIterator::chunk_type;
	static constexpr uint32_t chunk_bits = std::numeric_limits<chunk_type>::digits;
	static constexpr uint32_t chunk_size = SerializationInfo<chunk_type>::length;
private:
	UByteArrayAdapter & m_d;
	chunk_type m_lastChunk;
	int8_t m_inChunkBits; //number of remaining bits in chunk
};

} //end namespace

#endif