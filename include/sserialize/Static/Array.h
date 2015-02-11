#ifndef SSERIALIZE_STATIC_ARRAY_H
#define SSERIALIZE_STATIC_ARRAY_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/SerializationInfo.h>
#include <sserialize/utility/mmappedfile.h>
#include <sserialize/utility/AtStlInputIterator.h>
#include <sserialize/templated/AbstractArray.h>
#include <fstream>
#define SSERIALIZE_STATIC_ARRAY_VERSION 3

/** FileFormat: v4
 *
 *-----------------------------------------------
 *Version|DataLen        |Data| Data offsets    |
 *-----------------------------------------------
 *  u8   |UBA::OffsetType|  * |SortedOffsetIndex|
 * SIZE = Size of Data
 */

namespace sserialize {
namespace Static {
namespace detail {
namespace ArrayCreator {
	template<typename TValue>
	struct DefaultStreamingSerializer {
		void operator()(sserialize::UByteArrayAdapter & dest, const TValue & src) {
			dest << src;
		}
	};
}}

template<typename TValue, typename T_STREAMING_SERIALIZER = detail::ArrayCreator::DefaultStreamingSerializer<TValue> >
class ArrayCreator {
	UByteArrayAdapter & m_dest;
	std::vector<OffsetType> m_offsets;
	OffsetType m_dataLenPtr;
	OffsetType m_beginOffSet;
	T_STREAMING_SERIALIZER m_ss;
public:
	///create a new Array at tellPutPtr()
	ArrayCreator(UByteArrayAdapter & destination, const T_STREAMING_SERIALIZER & ss = T_STREAMING_SERIALIZER() ) :
	m_dest(destination), m_ss(ss) {
		m_dest.putUint8(3);//version
		m_dataLenPtr = m_dest.tellPutPtr();
		m_dest.putOffset(0);
		
		m_beginOffSet = m_dest.tellPutPtr();
	}
	uint32_t size() const { return m_offsets.size(); }
	const std::vector<OffsetType> & offsets() const { return m_offsets; }
	virtual ~ArrayCreator() {}
	void reserveOffsets(uint32_t size) { m_offsets.reserve(size); }
	void put(const TValue & value) {
		m_offsets.push_back(m_dest.tellPutPtr() - m_beginOffSet);
		m_ss(m_dest, value);
	}
	void beginRawPut() {
		m_offsets.push_back(m_dest.tellPutPtr() - m_beginOffSet);
	}
	UByteArrayAdapter & rawPut() { return m_dest;}
	void endRawPut() {}
	///@return data to create the deque (NOT dest data)
	UByteArrayAdapter flush() {
#if defined(DEBUG_CHECK_ARRAY_OFFSET_INDEX) || defined(DEBUG_CHECK_ALL)
		OffsetType oiBegin = m_dest.tellPutPtr();
#endif
		m_dest.putOffset(m_dataLenPtr, m_dest.tellPutPtr() - m_beginOffSet); //datasize
		if (!sserialize::Static::SortedOffsetIndexPrivate::create(m_offsets, m_dest)) {
			throw sserialize::CreationException("Array::flush: Creating the offset");
		}
#if defined(DEBUG_CHECK_ARRAY_OFFSET_INDEX) || defined(DEBUG_CHECK_ALL)
		sserialize::UByteArrayAdapter tmp = m_dest;
		tmp.setPutPtr(oiBegin);
		tmp.shrinkToPutPtr();
		sserialize::Static::SortedOffsetIndex oIndex;
		try {
			oIndex = sserialize::Static::SortedOffsetIndex(tmp);
		}
		catch (const Exception & e) {
			std::cout << e.what() << std::endl;
			writeOutOffset();
		}
		if (offsets() != oIndex) {
			writeOutOffset();
			throw sserialize::CreationException("Array::flush Offset index is unequal");
		}
		if (oIndex.getSizeInBytes() != (m_dest.tellPutPtr()-oiBegin)) {
			writeOutOffset();
			throw sserialize::CreationException("Array::flush Offset index reports wrong sizeInBytes()");
		}
#endif
		return m_dest + (m_beginOffSet-1-UByteArrayAdapter::OffsetTypeSerializedLength());
	}
	
	void writeOutOffset() {
		std::ofstream of;
		std::string str = MmappedFile::findLockFilePath(UByteArrayAdapter::getLogFilePrefix() + "Array_broken_OffsetIndex", 1024);
		of.open(str);
		if (of.is_open()) {
			for(uint64_t x : offsets()) {
				of << x << std::endl;
			}
			of.close();
		}
		
	}
};

template<typename TValue>
class Array: public RefCountObject {
public:
	typedef TValue value_type;
	typedef sserialize::ReadOnlyAtStlIterator< Array<TValue>*, TValue > iterator;
	typedef sserialize::ReadOnlyAtStlIterator< const Array<TValue>*, TValue > const_iterator;
	typedef value_type const_reference;
	typedef value_type reference;
	typedef enum {TI_FIXED_LENGTH=1} TypeInfo;
	static constexpr uint32_t npos = 0xFFFFFFFF;
private:
	SortedOffsetIndex m_index;
	UByteArrayAdapter m_data;
public:
	Array();
	/** Creates a new deque at data.putPtr */
	Array(const UByteArrayAdapter & data);
	/** This does not copy the ref count, but inits it */
	Array(const Array & other) : RefCountObject(), m_index(other.m_index), m_data(other.m_data) {}
	virtual ~Array() {}
	
	iterator begin() { return iterator(0, this); }
	const_iterator cbegin() const { return const_iterator(0, this); }
	iterator end() { return iterator(size(), this); }
	const_iterator cend() const { return const_iterator(size(), this); }
	
	/** This does not copy the ref count, it leaves it intact */
	Array & operator=(const Array & other) {
		m_data = other.m_data;
		m_index = other.m_index;
		return *this;
	}
	
	uint32_t size() const { return m_index.size();}
	UByteArrayAdapter::OffsetType getSizeInBytes() const {return 1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_index.getSizeInBytes()+m_data.size();}
	TValue at(uint32_t pos) const;
	TValue operator[](uint32_t pos) const;
	UByteArrayAdapter::OffsetType dataSize(uint32_t pos) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
	uint32_t find(const TValue & value) const;
	TValue front() const;
	TValue back() const;

	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data; }
	
	template<typename T_ORDER_MAP>
	static void reorder(const Array & src, const T_ORDER_MAP & /*order*/, ArrayCreator<TValue> & dest) {
		for(uint32_t i = 0; i < src.size(); ++i) {
			dest.beginRawPut();
			dest.rawPut().put(src.dataAt(i));
			dest.endRawPut();
		}
	}
	std::ostream & printStats(std::ostream & out) const {
		out << "sserialize::Static::Array::stats--BEGIN" << std::endl;
		out << "size=" << size() << std::endl;
		out << "data size=" << m_data.size() << std::endl;
		out << "total size=" << getSizeInBytes() << std::endl;
		out << "sserialize::Static::Array::stats--END" << std::endl;
		return out;
	}
};

namespace detail {

template<typename TReturnType>
class VectorAbstractArrayIterator: public sserialize::detail::AbstractArrayIterator<TReturnType> {
	const Array<TReturnType> * m_data;
	uint32_t m_pos;
public:
	VectorAbstractArrayIterator() : m_data(0), m_pos(0) {}
	VectorAbstractArrayIterator(const Array<TReturnType> * data, uint32_t pos) : m_data(data), m_pos(pos) {}
	VectorAbstractArrayIterator(const VectorAbstractArrayIterator & other) : m_data(other.m_data), m_pos(other.m_pos) {}
	virtual ~VectorAbstractArrayIterator() {}
	virtual TReturnType get() const { return m_data->at(m_pos);}
	virtual void next() { m_pos += (m_data->size() > m_pos ? 1 : 0);}
	virtual bool notEq(const sserialize::detail::AbstractArrayIterator<TReturnType> * other) const {
		const detail::VectorAbstractArrayIterator<TReturnType> * oIt = dynamic_cast<const detail::VectorAbstractArrayIterator<TReturnType>* >(other);
		return !oIt || oIt->m_data != m_data || oIt->m_pos != m_pos;
	}
	virtual sserialize::detail::AbstractArrayIterator<TReturnType> * copy() const {
		return new VectorAbstractArrayIterator(*this);
	}
};

template<typename TValue>
class VectorAbstractArray: public sserialize::detail::AbstractArray<TValue> {
public:
	typedef sserialize::detail::AbstractArray<TValue> MyBaseClass;
	typedef typename MyBaseClass::const_iterator const_iterator;
private:
	Array<TValue> m_data;
public:
	VectorAbstractArray() {}
	VectorAbstractArray(const Array<TValue> & d) : m_data(d) {}
	virtual ~VectorAbstractArray() {}
	virtual uint32_t size() const { return m_data.size(); }
	virtual TValue at(uint32_t pos) const { return m_data.at(pos); }
	virtual const_iterator cbegin() const { return new VectorAbstractArrayIterator<TValue>(&m_data, 0);}
	virtual const_iterator cend() const { return new VectorAbstractArrayIterator<TValue>(&m_data, m_data.size());}
};

}//end namespace detail
//----------Definitions-----------------------------------

template<typename TValue>
Array<TValue>::Array() : RefCountObject() {}

template<typename TValue>
Array<TValue>::Array(const UByteArrayAdapter & data) :
RefCountObject(),
m_index(data + (1 + UByteArrayAdapter::OffsetTypeSerializedLength() + data.getOffset(1))),
m_data(UByteArrayAdapter(data, 1+UByteArrayAdapter::OffsetTypeSerializedLength(), data.getOffset(1)))
{
SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_ARRAY_VERSION, data.at(0), "Static::Array");
SSERIALIZE_LENGTH_CHECK(m_index.size()*sserialize::SerializationInfo<TValue>::min_length, m_data.size(), "Static::Array::Array::Insufficient data");
}

template<typename TValue>
TValue
Array<TValue>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return TValue();
	}
	return TValue(dataAt(pos));
}

template<typename TValue>
TValue
Array<TValue>::operator[](uint32_t pos) const {
	return TValue(dataAt(pos));
}

template<typename TValue>
TValue
Array<TValue>::front() const {
	return at(0);
}

template<typename TValue>
TValue
Array<TValue>::back() const {
	return at(size()-1);
}

template<typename TValue>
UByteArrayAdapter::OffsetType
Array<TValue>::dataSize(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return 0;
	}
	UByteArrayAdapter::OffsetType begin = m_index.at(pos);
	UByteArrayAdapter::OffsetType len;
	if (pos+1 == size()) {
		len = m_data.size()-begin;
	}
	else {
		len = m_index.at(pos+1) - begin;
	}
	return len;
}

template<typename TValue>
UByteArrayAdapter
Array<TValue>::dataAt(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return UByteArrayAdapter();
	}
	UByteArrayAdapter::OffsetType begin = m_index.at(pos);
	UByteArrayAdapter::OffsetType len;
	if (pos+1 == size()) {
		len = m_data.size()-begin;
	}
	else {
		len = m_index.at(pos+1) - begin;
	}
	return UByteArrayAdapter(m_data, begin, len);
}

template<typename TValue>
uint32_t Array<TValue>::find(const TValue& value) const {
	for(uint32_t i = 0; i < size(); i++) {
		if (at(i) == value)
			return i;
	}
	return npos;
}

//Template specialications for integral types

template<>
int32_t
Array<int32_t>::at(uint32_t pos) const;

template<>
uint32_t
Array<uint32_t>::at(uint32_t pos) const;


template<>
uint16_t
Array<uint16_t>::at(uint32_t pos) const;

template<>
uint8_t
Array<uint8_t>::at(uint32_t pos) const;

template<>
std::string
Array<std::string>::at(uint32_t pos) const;

template<>
UByteArrayAdapter
Array<UByteArrayAdapter>::at(uint32_t pos) const;

template<typename TValue>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::Static::Array<TValue> & destination) {
	sserialize::UByteArrayAdapter tmpAdap(source);
	tmpAdap.shrinkToGetPtr();
	destination = sserialize::Static::Array<TValue>(tmpAdap);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}

template<typename TValue>
bool operator==(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	if (dequeA.size() != dequeB.size())
		return false;
	uint32_t size = dequeA.size();
	for(uint32_t i = 0; i < size; i++) {
		if (dequeA.at(i) != dequeB.at(i))
			return false;
	}
	return true;
}

template<typename TValue>
bool operator==(const sserialize::Static::Array<TValue> & dequeA, const std::deque<TValue> & dequeB) {
	if (dequeA.size() != dequeB.size())
		return false;
	uint32_t size = dequeA.size();
	for(uint32_t i = 0; i < size; i++) {
		if (dequeA.at(i) != dequeB.at(i))
			return false;
	}
	return true;
}

template<typename TValue>
bool operator==(const std::deque<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return dequeB == dequeA;
}

template<typename TValue>
bool operator<(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	uint32_t dequeItA = 0;
	uint32_t dequeItB = 0;
	while (dequeItA<dequeA.size()) {
		if (dequeB.size() <= dequeItB || dequeB.at(dequeItB) < dequeA.at(dequeItA)) {
			return false;
		}
		TValue vA(dequeA.at(dequeItA));
		TValue vB(dequeB.at(dequeItB));
		if (vB < vA) {
			return false;
		}
		else if (vA < vB)
			return true;
		dequeItA++;
		dequeItB++;
	}
	return (dequeItB < dequeB.size());
}

template<typename TValue>
bool operator<(const sserialize::Static::Array<TValue> & dequeA, const std::deque<TValue> & dequeB) {
	uint32_t dequeItA = 0;
	uint32_t dequeItB = 0;
	while (dequeItA<dequeA.size()) {
		if (dequeB.size() <= dequeItB || dequeB.at(dequeItB) < dequeA.at(dequeItA)) {
			return false;
		}
		TValue vA(dequeA.at(dequeItA));
		TValue vB(dequeB.at(dequeItB));
		if (vB < vA) {
			return false;
		}
		else if (vA < vB)
			return true;
		dequeItA++;
		dequeItB++;
	}
	return (dequeItB < dequeB.size());
}

template<typename TValue>
bool operator<(const std::deque<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	uint32_t dequeItA = 0;
	uint32_t dequeItB = 0;
	while (dequeItA<dequeA.size()) {
		if (dequeB.size() <= dequeItB || dequeB.at(dequeItB) < dequeA.at(dequeItA)) {
			return false;
		}
		TValue vA(dequeA.at(dequeItA));
		TValue vB(dequeB.at(dequeItB));
		if (vB < vA) {
			return false;
		}
		else if (vA < vB)
			return true;
		dequeItA++;
		dequeItB++;
	}
	return (dequeItB < dequeB.size());
}

template<typename TValue>
bool operator!=(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator!=(const sserialize::Static::Array<TValue> & dequeA, const std::deque<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator!=(const std::deque<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator>(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return dequeB < dequeA;
}

template<typename TValue>
bool operator<=(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return ! (dequeA > dequeB);
}

template<typename TValue>
bool operator>=(const sserialize::Static::Array<TValue> & dequeA, const sserialize::Static::Array<TValue> & dequeB) {
	return ! (dequeA < dequeB);
}

}//end namespace Static

template<typename TValue>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::deque<TValue> & source) {
	sserialize::Static::ArrayCreator<TValue> dc(destination);
	dc.reserveOffsets(source.size());
	for(std::size_t i = 0, s = source.size(); i < s; ++i) {
		dc.put(source[i]);
	}
	dc.flush();
	return destination;
}

template<typename TValue, typename T_STREAMING_SERIALIZER = sserialize::Static::detail::ArrayCreator::DefaultStreamingSerializer<TValue> >
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::vector<TValue> & source) {
	sserialize::Static::ArrayCreator<TValue, T_STREAMING_SERIALIZER> dc(destination);
	dc.reserveOffsets(source.size());
	for(std::size_t i = 0, s = source.size(); i < s; ++i) {
		dc.put(source[i]);
	}
	dc.flush();
	return destination;
}


}//end namespace sserialize

#endif
