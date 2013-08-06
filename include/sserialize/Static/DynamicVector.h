#ifndef SSERIALIZE_DYNAMIC_VECTOR_H
#define SSERIALIZE_DYNAMIC_VECTOR_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/Static/Deque.h>

namespace sserialize {
namespace Static {

template<typename TPushValue, typename TGetValue = TPushValue>
class DynamicVector {
public:
	uint32_t m_size;
	UByteArrayAdapter m_offsets;
	UByteArrayAdapter m_data;
public:
	DynamicVector(uint32_t approxItemCount, OffsetType initalDataSize);
	virtual ~DynamicVector();
	uint32_t size() const;
	void reserve(uint32_t size) {}
	template<typename TStreamingSerializer = UByteArrayAdapter::StreamingSerializer<TPushValue> >
	void push_back(const TPushValue & value, const TStreamingSerializer & serializer = TStreamingSerializer());
	void pop_back();
	UByteArrayAdapter & beginRawPush();
	void endRawPush();
	template<typename TDeserializer = UByteArrayAdapter::Deserializer<TGetValue> >
	TGetValue at(uint32_t pos, const TDeserializer & derefer = TDeserializer()) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
	
	UByteArrayAdapter & toDeque(UByteArrayAdapter & dest) const;
};

template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::DynamicVector(uint32_t approxItemCount, OffsetType initalDataSize) :
m_size(0),
m_offsets(UByteArrayAdapter::createCache(approxItemCount*UByteArrayAdapter::OffsetTypeSerializedLength(), true)),
m_data(UByteArrayAdapter::createCache(initalDataSize, true))
{}


template<typename TPushValue, typename TGetValue>
DynamicVector<TPushValue, TGetValue>::~DynamicVector() {}

template<typename TPushValue, typename TGetValue>
uint32_t DynamicVector<TPushValue, TGetValue>::size() const {
	return m_size;
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
		OffsetType tmp = m_offsets.getOffset((m_size-1)*UByteArrayAdapter::OffsetTypeSerializedLength());
		m_offsets.setPutPtr((m_size-1)*UByteArrayAdapter::OffsetTypeSerializedLength());
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
	OffsetType begin = m_offsets.getOffset(pos*UByteArrayAdapter::OffsetTypeSerializedLength());
	OffsetType len;
	if (pos == m_size-1) {
		len = m_data.tellPutPtr()-begin;
	}
	else {
		len = m_offsets.getOffset((pos+1)*UByteArrayAdapter::OffsetTypeSerializedLength())-begin;
	}
	return UByteArrayAdapter(m_data, begin, len);
}

template<typename TPushValue, typename TGetValue>
UByteArrayAdapter & DynamicVector<TPushValue, TGetValue>::toDeque(UByteArrayAdapter & dest) const {
	Static::DequeCreator<UByteArrayAdapter> dc(dest);
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
	return src.toDeque(dest);
}

#endif