#ifndef SSERIALIZE_DYNAMIC_VECTOR_H
#define SSERIALIZE_DYNAMIC_VECTOR_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/Array.h>

namespace sserialize {
namespace Static {

template<typename TPushValue, typename TGetValue = TPushValue>
class DynamicVector {
public:
	uint32_t m_size;
	UByteArrayAdapter m_offsets;
	UByteArrayAdapter m_data;
private:
	DynamicVector(const DynamicVector & other);
	DynamicVector & operator=(const DynamicVector & other);
public:
	DynamicVector(uint32_t approxItemCount, OffsetType initalDataSize, sserialize::MmappedMemoryType mmt = sserialize::MM_FILEBASED);
	virtual ~DynamicVector();
	void swap(DynamicVector & other);
	uint32_t size() const;
	OffsetType reservedSize() const;
	void reserve(uint32_t /*size*/) { std::cerr << "Reserving is not supported by sserialize::Static::DynamicVector" << std::endl; }
	template<typename TStreamingSerializer = UByteArrayAdapter::StreamingSerializer<TPushValue> >
	void push_back(const TPushValue & value, const TStreamingSerializer & serializer = TStreamingSerializer());
	void pop_back();
	UByteArrayAdapter & beginRawPush();
	void endRawPush();
	template<typename TDeserializer = UByteArrayAdapter::Deserializer<TGetValue> >
	TGetValue at(uint32_t pos, const TDeserializer & derefer = TDeserializer()) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
	
	UByteArrayAdapter & toArray(UByteArrayAdapter & dest) const;
};

template<typename TPushValue, typename TGetValue>
void swap(sserialize::Static::DynamicVector<TPushValue, TGetValue> & a, sserialize::Static::DynamicVector<TPushValue, TGetValue> & b) {
	return a.swap(b);
}

template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::DynamicVector(uint32_t approxItemCount, OffsetType initalDataSize, sserialize::MmappedMemoryType mmt ) :
m_size(0),
m_offsets(UByteArrayAdapter::createCache(static_cast<OffsetType>(approxItemCount)*UByteArrayAdapter::OffsetTypeSerializedLength(), mmt)),
m_data(UByteArrayAdapter::createCache(initalDataSize, mmt))
{}


template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::~DynamicVector() {}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::swap(DynamicVector & other) {
	using std::swap;
	swap(m_size, other.m_size);
	swap(m_offsets, other.m_offsets);
	swap(m_data, other.m_data);
}

template<typename TPushValue, typename TGetValue>
uint32_t DynamicVector<TPushValue, TGetValue>::size() const {
	return m_size;
}

template<typename TPushValue, typename TGetValue>
OffsetType DynamicVector<TPushValue, TGetValue>::reservedSize() const {
	return m_data.size();
}

template<typename TPushValue, typename TGetValue>
template<typename TStreamingSerializer>
void DynamicVector<TPushValue, TGetValue>::push_back(const TPushValue & value, const TStreamingSerializer & serializer) {
	serializer.operator()(value, beginRawPush());
	endRawPush();
}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::pop_back() {
	if (m_size) {
		OffsetType tmp = m_offsets.getOffset(static_cast<OffsetType>(m_size-1)*UByteArrayAdapter::OffsetTypeSerializedLength());
		m_offsets.setPutPtr(static_cast<OffsetType>(m_size-1)*UByteArrayAdapter::OffsetTypeSerializedLength());
		--m_size;
		if (!m_size) {
			m_data.setPutPtr(0);
		}
		else {
			m_data.setPutPtr(tmp);
		}
	}
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter & DynamicVector<TPushValue, TGetValue>::beginRawPush() {
	m_offsets.putOffset(m_data.tellPutPtr());
	return m_data;
}

template<typename TPushValue, typename TGetValue>
void DynamicVector<TPushValue, TGetValue>::endRawPush() {
	++m_size;
}

template<typename TPushValue, typename TGetValue>
template<typename TDeserializer>
TGetValue DynamicVector<TPushValue, TGetValue>::at(uint32_t pos, const TDeserializer & deserializer) const {
	if (pos >= size())
		return TGetValue();
	return deserializer(dataAt(pos));
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter DynamicVector<TPushValue, TGetValue>::dataAt(uint32_t pos) const {
	if (pos >= size())
		return UByteArrayAdapter();
	OffsetType begin = m_offsets.getOffset(static_cast<sserialize::UByteArrayAdapter::OffsetType>(pos)*UByteArrayAdapter::OffsetTypeSerializedLength());
	OffsetType len;
	if (pos == m_size-1) {
		len = m_data.tellPutPtr()-begin;
	}
	else {
		len = m_offsets.getOffset(static_cast<sserialize::UByteArrayAdapter::OffsetType>(pos+1)*UByteArrayAdapter::OffsetTypeSerializedLength())-begin;
	}
	UByteArrayAdapter ret(m_data, begin, len);
	ret.resetPtrs();
	return ret;
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter & DynamicVector<TPushValue, TGetValue>::toArray(UByteArrayAdapter & dest) const {
	Static::ArrayCreator<UByteArrayAdapter> dc(dest);
	dc.reserveOffsets(size());
	for(uint32_t i = 0, s = size(); i < s; ++i) {
		dc.beginRawPut();
		dc.rawPut().put(dataAt(i));
		dc.endRawPut();
	}
	dc.flush();
	return dest;
}

}}//end namespace

template<typename TPushValue, typename TGetValue>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const sserialize::Static::DynamicVector<TPushValue, TGetValue> & src) {
	return src.toArray(dest);
}

#endif