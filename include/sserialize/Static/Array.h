#ifndef SSERIALIZE_STATIC_ARRAY_H
#define SSERIALIZE_STATIC_ARRAY_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/SerializationInfo.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/iterator/AtStlInputIterator.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/assert.h>
#include <sserialize/utility/VersionChecker.h>
#include <fstream>
#include <functional>
#define SSERIALIZE_STATIC_ARRAY_VERSION 4

/** FileFormat: v4
 *
 *-------------------------------------------------------------
 *Version|DataLen        |Data| (Data offsets or entry length)
 *-------------------------------------------------------------
 *  u8   |UBA::OffsetType|  * | (SortedOffsetIndex or vu32)
 * SIZE = Size of Data
 * 
 * data offsets/entry length: entry length if entry has constant length but is not integral, otherweise offset index
 * 
 * Changelog:
 * v4: remove Data Offsets index for constant-length types,
 * 
 * 
 */

namespace sserialize {
namespace Static {
namespace detail {
namespace ArrayCreator {
	template<typename TValue>
	struct DefaultStreamingSerializer {
		inline void operator()(sserialize::UByteArrayAdapter & dest, const TValue & src) {
			dest << src;
		}
	};
	
	template<>
	struct DefaultStreamingSerializer<UByteArrayAdapter> {
		inline void operator()(sserialize::UByteArrayAdapter & dest, const UByteArrayAdapter & src) {
			dest.putData(src);
		}
	};
	
}}

template<typename TValue, typename T_STREAMING_SERIALIZER = detail::ArrayCreator::DefaultStreamingSerializer<TValue>, typename T_OFFSET_STORAGE = std::vector<OffsetType> >
class ArrayCreator {
public:
	typedef T_OFFSET_STORAGE OffsetContainer;
	typedef uint32_t SizeType;
private:
	UByteArrayAdapter * m_dest;
	uint32_t m_size;
	OffsetContainer m_offsets;
	OffsetType m_dataLenPtr;
	OffsetType m_dataBegin;
	T_STREAMING_SERIALIZER m_ss;
public:
	///create a new Array at tellPutPtr()
	ArrayCreator(UByteArrayAdapter & destination, const T_STREAMING_SERIALIZER & ss = T_STREAMING_SERIALIZER(), const OffsetContainer & ofs = OffsetContainer()) :
	m_dest(&destination),
	m_size(0),
	m_offsets(ofs),
	m_ss(ss)
	{
		m_dest->putUint8(4);//version
		m_dataLenPtr = m_dest->tellPutPtr();
		m_dest->putOffset(0);
		
		m_dataBegin = m_dest->tellPutPtr();
	}
	ArrayCreator(UByteArrayAdapter * destination, const T_STREAMING_SERIALIZER & ss = T_STREAMING_SERIALIZER(), const OffsetContainer & ofs = OffsetContainer() ) :
	m_dest(destination),
	m_size(0),
	m_offsets(ofs),
	m_ss(ss)
	{
		m_dest->putUint8(4);//version
		m_dataLenPtr = m_dest->tellPutPtr();
		m_dest->putOffset(0); //data len
		
		m_dataBegin = m_dest->tellPutPtr();
	}
	ArrayCreator() :
	m_dest(0),
	m_size(0),
	m_dataLenPtr(0),
	m_dataBegin(0)
	{}
	
	ArrayCreator(ArrayCreator && other) :
	m_dest(std::move(other.m_dest)),
	m_size(std::move(other.m_size)),
	m_offsets(std::move(other.m_offsets)),
	m_dataLenPtr(std::move(other.m_dataLenPtr)),
	m_dataBegin(std::move(other.m_dataBegin)),
	m_ss(std::move(other.m_ss))
	{}
	virtual ~ArrayCreator() {}
	ArrayCreator & operator=(ArrayCreator && other) {
		m_dest = std::move(other.m_dest);
		m_size = std::move(other.m_size);
		m_dataLenPtr = std::move(other.m_dataLenPtr);
		m_dataBegin = std::move(other.m_dataBegin);
		m_offsets = std::move(other.m_offsets);
		m_ss = std::move(other.m_ss);
		return *this;
	}
	void clear() {
		m_offsets.clear();
		m_dest->setPutPtr(m_dataBegin);
	}
	uint32_t size() const { return m_size; }
	const OffsetContainer & offsets() const { return m_offsets; }
	void reserveOffsets(uint32_t size) { m_offsets.reserve(size); }
	void reserve(uint32_t size) { reserveOffsets(size); }
	void put(const TValue & value) {
		beginRawPut();
		m_ss(rawPut(), value);
		endRawPut();
	}
	void beginRawPut() {
		if (!sserialize::SerializationInfo<TValue>::is_fixed_length) {
			m_offsets.push_back(m_dest->tellPutPtr() - m_dataBegin);
		}
		++m_size;
	}
	UByteArrayAdapter & rawPut() { return *m_dest;}
	void endRawPut() {}
	UByteArrayAdapter dataAt(uint32_t id) const {
		sserialize::UByteArrayAdapter::OffsetType begin = 0;
		sserialize::UByteArrayAdapter::OffsetType len = 0;
		if (sserialize::SerializationInfo<TValue>::is_fixed_length) {
			begin = id*sserialize::SerializationInfo<TValue>::length;
			len = sserialize::SerializationInfo<TValue>::length;
		}
		else {
			begin = m_offsets.at(id);
			if (id+1 < m_offsets.size()) {
				len = m_offsets.at(id+1) - begin;
			}
			else {
				len = m_dest->tellPutPtr() - (m_dataBegin + begin);
			}
		}
		return UByteArrayAdapter(*m_dest, begin+m_dataBegin, len).resetPtrs();
	}
	///@return data to create the Array (NOT dest data)
	UByteArrayAdapter flush() {
		#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
		OffsetType oiBegin = m_dest->tellPutPtr();
		#endif
		m_dest->putOffset(m_dataLenPtr, m_dest->tellPutPtr() - m_dataBegin); //datasize
		if (sserialize::SerializationInfo<TValue>::is_fixed_length) {
			if (!std::is_integral<TValue>::value) {
				m_dest->putVlPackedUint32(sserialize::SerializationInfo<TValue>::length);
			}
		}
		else {
			if (!sserialize::Static::SortedOffsetIndexPrivate::create(m_offsets, *m_dest)) {
				throw sserialize::CreationException("Array::flush: Creating the offset");
			}
			#if defined(SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
			sserialize::UByteArrayAdapter tmp = *m_dest;
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
			if (oIndex.getSizeInBytes() != (m_dest->tellPutPtr()-oiBegin)) {
				writeOutOffset();
				throw sserialize::CreationException("Array::flush Offset index reports wrong sizeInBytes()");
			}
			#endif
		}
		return getFlushedData();
	}
	//this is only valid after flushing
	UByteArrayAdapter getFlushedData() const {
		sserialize::UByteArrayAdapter::OffsetType offsetToBegin = (m_dataBegin-1-UByteArrayAdapter::OffsetTypeSerializedLength());
		return UByteArrayAdapter(*m_dest, offsetToBegin, m_dest->tellPutPtr()-offsetToBegin).resetPtrs();
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

namespace detail {

template<typename TValue, typename TEnable = void>
class ArrayOffsetIndex;

template<typename TValue>
class ArrayOffsetIndex<TValue, typename std::enable_if<sserialize::SerializationInfo<TValue>::is_fixed_length>::type > {
public:
	typedef ArrayOffsetIndex<TValue, typename std::enable_if<sserialize::SerializationInfo<TValue>::is_fixed_length>::type > MyType;
private:
	uint32_t m_size;
public:
	ArrayOffsetIndex() {}
	ArrayOffsetIndex(const MyType & other) : m_size(other.m_size) {}
	ArrayOffsetIndex(sserialize::UByteArrayAdapter::OffsetType dataSize, const sserialize::UByteArrayAdapter & d) :
	m_size((uint32_t)(dataSize/sserialize::SerializationInfo<TValue>::length))
	{
		SSERIALIZE_CHEAP_ASSERT((sserialize::UByteArrayAdapter::OffsetType)m_size*sserialize::SerializationInfo<TValue>::length == dataSize);
		if (!std::is_integral<TValue>::value) {
			if (d.getVlPackedUint32(0) != sserialize::SerializationInfo<TValue>::length) {
				throw sserialize::CorruptDataException("sserialize::Static::Array: sizeof(value_type) - WANT=" +
					std::to_string(sserialize::SerializationInfo<TValue>::length) +
					", IS=" + std::to_string(d.getVlPackedUint32(0)));
			}
		}
	}
	~ArrayOffsetIndex() {}
	sserialize::UByteArrayAdapter::OffsetType at(uint32_t pos) const { return sserialize::SerializationInfo<TValue>::length*pos; }
	uint32_t size() const { return m_size; }
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const {
		if (std::is_integral<TValue>::value) {
			return 0;
		}
		else {
			return psize_v<uint32_t>(sserialize::SerializationInfo<TValue>::length);
		}
	}
};

template<typename TValue>
class ArrayOffsetIndex<TValue, typename std::enable_if<!sserialize::SerializationInfo<TValue>::is_fixed_length>::type > {
public:
	typedef ArrayOffsetIndex<TValue, typename std::enable_if<!sserialize::SerializationInfo<TValue>::is_fixed_length>::type > MyType;
private:
	sserialize::Static::SortedOffsetIndex m_index;
public:
	ArrayOffsetIndex() {}
	ArrayOffsetIndex(const MyType & other) : m_index(other.m_index) {}
	ArrayOffsetIndex(sserialize::UByteArrayAdapter::OffsetType /*dataSize*/, const sserialize::UByteArrayAdapter & d) : m_index(d) {}
	~ArrayOffsetIndex() {}
	inline sserialize::UByteArrayAdapter::OffsetType at(uint32_t pos) const { return m_index.at(pos); }
	uint32_t size() const { return m_index.size(); }
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const { return m_index.getSizeInBytes();}
};

}//end namespace detail::Array

template<typename TValue>
class Array: public RefCountObject {
public:
	typedef TValue value_type;
	typedef sserialize::ReadOnlyAtStlIterator< Array<TValue>*, TValue > iterator;
	typedef sserialize::ReadOnlyAtStlIterator< const Array<TValue>*, TValue > const_iterator;
	typedef value_type const_reference;
	typedef value_type reference;
	typedef enum {TI_FIXED_LENGTH=1} TypeInfo;
	typedef uint32_t SizeType;
	static constexpr SizeType npos = 0xFFFFFFFF;
private:
	UByteArrayAdapter m_data;
	detail::ArrayOffsetIndex<TValue> m_index;
	sserialize::UByteArrayAdapter::Deserializer<TValue> m_ds;
public:
	Array();
	/** Creates a new deque at data.putPtr */
	Array(const UByteArrayAdapter & data);
	/** This does not copy the ref count, but inits it */
	Array(const Array & other) : RefCountObject(), m_data(other.m_data), m_index(other.m_index) {}
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
	
	SizeType size() const { return m_index.size();}
	UByteArrayAdapter::OffsetType getSizeInBytes() const {return 1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_index.getSizeInBytes()+m_data.size();}
	TValue at(SizeType pos) const;
	TValue operator[](SizeType pos) const;
	UByteArrayAdapter::OffsetType dataSize(SizeType pos) const;
	UByteArrayAdapter dataAt(SizeType pos) const;
	SizeType find(const TValue & value) const;
	TValue front() const;
	TValue back() const;

	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data; }
	
	template<typename T_ORDER_MAP>
	static void reorder(const Array & src, const T_ORDER_MAP & /*order*/, ArrayCreator<TValue> & dest) {
		for(SizeType i(0), s(src.size()); i < s; ++i) {
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
public:
	typedef sserialize::detail::AbstractArrayIterator<TReturnType> MyBaseClass;
private:
	typedef typename Array<TReturnType>::SizeType SizeType;
private:
	const Array<TReturnType> * m_data;
	SizeType m_pos;
public:
	VectorAbstractArrayIterator() : m_data(0), m_pos(0) {}
	VectorAbstractArrayIterator(const Array<TReturnType> * data, uint32_t pos) : m_data(data), m_pos(pos) {}
	VectorAbstractArrayIterator(const VectorAbstractArrayIterator & other) : m_data(other.m_data), m_pos(other.m_pos) {}
	virtual ~VectorAbstractArrayIterator() override {}
	virtual TReturnType get() const override { return m_data->at(m_pos);}
	virtual void next() override { m_pos += (m_data->size() > m_pos ? 1 : 0);}
	virtual bool notEq(const MyBaseClass * other) const override {
		const detail::VectorAbstractArrayIterator<TReturnType> * oIt = dynamic_cast<const detail::VectorAbstractArrayIterator<TReturnType>* >(other);
		return !oIt || oIt->m_data != m_data || oIt->m_pos != m_pos;
	}
	virtual bool eq(const MyBaseClass * other) const override {
		const detail::VectorAbstractArrayIterator<TReturnType> * oIt = dynamic_cast<const detail::VectorAbstractArrayIterator<TReturnType>* >(other);
		return oIt && oIt->m_data == m_data && oIt->m_pos != m_pos;
	};
	virtual sserialize::detail::AbstractArrayIterator<TReturnType> * copy() const override {
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
	virtual ~VectorAbstractArray() override {}
	virtual uint32_t size() const override { return m_data.size(); }
	virtual TValue at(uint32_t pos) const override { return m_data.at(pos); }
	virtual const_iterator cbegin() const override { return new VectorAbstractArrayIterator<TValue>(&m_data, 0);}
	virtual const_iterator cend() const override { return new VectorAbstractArrayIterator<TValue>(&m_data, m_data.size());}
};

}//end namespace detail
//----------Definitions-----------------------------------

template<typename TValue>
Array<TValue>::Array() : RefCountObject() {}

template<typename TValue>
Array<TValue>::Array(const UByteArrayAdapter & data) :
RefCountObject(),
m_data(
	sserialize::VersionChecker::check(
		UByteArrayAdapter(data, 1+UByteArrayAdapter::OffsetTypeSerializedLength(), data.getOffset(1)),
		SSERIALIZE_STATIC_ARRAY_VERSION, 
		data.at(0),
		"Static::Array"
	)
),
m_index(m_data.size(), data + (1 + UByteArrayAdapter::OffsetTypeSerializedLength() + m_data.size()))
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
	return m_ds(dataAt(pos));
}

template<typename TValue>
TValue
Array<TValue>::operator[](uint32_t pos) const {
	return m_ds(dataAt(pos));
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
	typedef sserialize::Static::ArrayCreator<TValue, T_STREAMING_SERIALIZER> ArrayCreator;
	typedef typename ArrayCreator::SizeType SizeType;
	if (source.size() > std::numeric_limits<SizeType>::max()) {
		throw sserialize::TypeOverflowException("std::vector -> Static::Array serialization");
	}
	ArrayCreator dc(destination);
	dc.reserveOffsets((SizeType)source.size());
	for(const auto & x : source) {
		dc.put(x);
	}
	dc.flush();
	return destination;
}


}//end namespace sserialize

#endif
