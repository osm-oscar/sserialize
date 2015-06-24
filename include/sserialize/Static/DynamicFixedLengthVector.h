#ifndef SSERIALIZE_STATIC_FIXED_LENGTH_VECTOR_H
#define SSERIALIZE_STATIC_FIXED_LENGTH_VECTOR_H
#define SSERIALIZE_STATIC_FIXED_LENGTH_VECTOR_VERSION 0
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/SerializationInfo.h>
#include <sserialize/utility/exceptions.h>
#include <type_traits>

namespace sserialize {
namespace Static {

/**
  * Layout:
  *-------------------------------------
  *VERSION|COUNT|*
  *-------------------------------------
  *u8     |u64
  */

template<typename TPushValue, typename TGetValue = TPushValue, OffsetType TLENGTH = SerializationInfo<TGetValue>::length >
class DynamicFixedLengthVector {
public:
	typedef uint64_t SizeType;
private:
	UByteArrayAdapter m_data;
private:
	void setSize(SizeType size) { m_data.put<SizeType>(1, size); }
	UByteArrayAdapter dataAt(SizeType pos) const {
		return UByteArrayAdapter(m_data, 1 + SerializationInfo<uint64_t>::length + pos*TLENGTH, TLENGTH);
	}
	void movePutPtrToEnd() {
		m_data.setPutPtr(1 + SerializationInfo<uint64_t>::length + size()*TLENGTH);
	}
public:
	DynamicFixedLengthVector() {}
	DynamicFixedLengthVector(const UByteArrayAdapter & data, bool initialize = true) : m_data(data) {
		m_data.shrinkToPutPtr();
		if (initialize) {
			m_data.putUint8(0, SSERIALIZE_STATIC_FIXED_LENGTH_VECTOR_VERSION);
			setSize(0);
		}
		movePutPtrToEnd();
	}
	virtual ~DynamicFixedLengthVector() {}
	static OffsetType spaceUsage(SizeType elementCount) { return 1 + SerializationInfo<uint64_t>::length + elementCount*TLENGTH; }
	SizeType size() const { return m_data.getUint64(1); }
	OffsetType getSizeInBytes() { return spaceUsage(size()); }
	bool resize(SizeType size) {
		bool ok = m_data.resize( spaceUsage(size) );
		if (ok) {
			setSize(size);
			movePutPtrToEnd();
		}
		return ok;
	}
	void push_back(const TPushValue & value) {
		m_data << value;
		setSize(1+size());
	}
	void pop_back() {
		uint64_t s = size();
		if (s) {
			m_data.decPutPtr(TLENGTH);
			setSize(s-1);
		}
	}
	void set(SizeType pos, const TPushValue & value) {
		if (pos < size()) {
			UByteArrayAdapter d(dataAt(pos));
			d << value;
		}
		else {
			throw sserialize::OutOfBoundsException("sserialize::Static::DynamicFixedLengthVector::set");
		}
	}
	TGetValue at(SizeType pos) const {
		if (pos < size()) {
			return TGetValue( dataAt(pos) );
		}
		else {
			throw sserialize::OutOfBoundsException("sserialize::Static::DynamicFixedLengthVector::set");
		}
		return TGetValue();
	}
	void swap(SizeType pos1, SizeType pos2) {
		uint64_t s = size();
		if (pos1 < s && pos2 < s) {
			UByteArrayAdapter a(m_data, 1 + SerializationInfo<uint64_t>::length + pos1*TLENGTH, TLENGTH);
			UByteArrayAdapter b(m_data, 1 + SerializationInfo<uint64_t>::length + pos2*TLENGTH, TLENGTH);
			uint8_t * tmp = new uint8_t[TLENGTH];
			a.get(tmp, TLENGTH);
			a.put(0, b);
			b.put(0, tmp);
			delete tmp;
		}
	}
	
};

}}//end namespace

#endif