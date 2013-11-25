#ifndef SSERIALIZE_STATIC_DEQUE_H
#define SSERIALIZE_STATIC_DEQUE_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/containers/SortedOffsetIndex.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/SerializationInfo.h>
#include <sserialize/utility/mmappedfile.h>
#include <sserialize/utility/AtStlInputIterator.h>
#include <fstream>
#define SSERIALIZE_STATIC_DEQUE_VERSION 3

/** FileFormat: v3
 *
 *-------------------------------
 *Version|DataLen|Data|DataIndex|
 *-------------------------------
 *  1    |  5    |  * |   *     |
 * SIZE = Size of Data
 */

namespace sserialize {
namespace Static {

template<typename TValue>
class DequeCreator {
	UByteArrayAdapter & m_dest;
	std::vector<OffsetType> m_offsets;
	OffsetType m_dataLenPtr;
	OffsetType m_beginOffSet;
public:
	DequeCreator(UByteArrayAdapter & destination) : m_dest(destination) {
		m_dest.putUint8(SSERIALIZE_STATIC_DEQUE_VERSION);
		m_dataLenPtr = m_dest.tellPutPtr();
		m_dest.putOffset(0);
		
		m_beginOffSet = m_dest.tellPutPtr();
	}
	const std::vector<OffsetType> & offsets() const { return m_offsets; }
	virtual ~DequeCreator() {}
	void reserveOffsets(uint32_t size) { m_offsets.reserve(size); }
	void put(const TValue & value) {
		m_offsets.push_back(m_dest.tellPutPtr() - m_beginOffSet);
		m_dest << value;
	}
	void beginRawPut() {
		m_offsets.push_back(m_dest.tellPutPtr() - m_beginOffSet);
	}
	UByteArrayAdapter & rawPut() { return m_dest;}
	void endRawPut() {}
	
	///@return data to create the deque (NOT dest data)
	UByteArrayAdapter flush() {
#if defined(DEBUG_DEQUE_OFFSET_INDEX) || defined(DEBUG_CHECK_ALL)
		OffsetType oiBegin = m_dest.tellPutPtr();
#endif
		m_dest.putOffset(m_dataLenPtr, m_dest.tellPutPtr() - m_beginOffSet); //datasize
		if (!sserialize::Static::SortedOffsetIndexPrivate::create(m_offsets, m_dest)) {
			throw sserialize::CreationException("Deque::flush: Creating the offset");
		}
#if defined(DEBUG_DEQUE_OFFSET_INDEX) || defined(DEBUG_CHECK_ALL)
		sserialize::UByteArrayAdapter tmp = m_dest;
		tmp.setPutPtr(oiBegin);
		tmp.shrinkToPutPtr();
		sserialize::Static::SortedOffsetIndex oIndex(tmp);
		if (offsets() != oIndex) {
			writeOutOffset();
			throw sserialize::CreationException("Deque::flush Offset index is unequal");
		}
		if (oIndex.getSizeInBytes() != (m_dest.tellPutPtr()-oiBegin)) {
			writeOutOffset();
			throw sserialize::CreationException("Deque::flush Offset index reports wrong sizeInBytes()");
		}
#endif
		return m_dest + (m_beginOffSet-1-UByteArrayAdapter::OffsetTypeSerializedLength());
	}
	
	void writeOutOffset() {
		std::ofstream of;
		std::string str = MmappedFile::findLockFilePath(UByteArrayAdapter::getTempFilePrefix(), 1024);
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
class Deque: public RefCountObject {
public:
	typedef TValue value_type;
	typedef sserialize::ReadOnlyAtStlIterator< Deque<TValue>*, TValue > iterator;
	typedef sserialize::ReadOnlyAtStlIterator< const Deque<TValue>*, TValue > const_iterator; 
private:
	SortedOffsetIndex m_index;
	UByteArrayAdapter m_data;
public:
	Deque();
	/** Creates a new deque at data.putPtr */
	Deque(const UByteArrayAdapter & data);
	/** This does not copy the ref count, but inits it */
	Deque(const Deque & other) : RefCountObject(), m_index(other.m_index), m_data(other.m_data) {}
	virtual ~Deque() {}
	
	iterator begin() { return iterator(0, this); }
	const_iterator cbegin() const { return const_iterator(0, this); }
	iterator end() { return iterator(size(), this); }
	const_iterator cend() const { return const_iterator(size(), this); }
	
	/** This does not copy the ref count, it leaves it intact */
	Deque & operator=(const Deque & other) {
		m_data = other.m_data;
		m_index = other.m_index;
		return *this;
	}
	
	uint32_t size() const { return m_index.size();}
	UByteArrayAdapter::OffsetType getSizeInBytes() const {return 1+UByteArrayAdapter::OffsetTypeSerializedLength()+m_index.getSizeInBytes()+m_data.size();}
	TValue at(uint32_t pos) const;
	UByteArrayAdapter dataAt(uint32_t pos) const;
	int32_t find(const TValue & value) const;
	TValue front() const;
	TValue back() const;

	UByteArrayAdapter & data() { return m_data;}
	const UByteArrayAdapter & data() const { return m_data; }
	
	template<typename T_ORDER_MAP>
	static void reorder(const Deque & src, const T_ORDER_MAP & order, DequeCreator<TValue> & dest) {
		for(uint32_t i = 0; i < src.size(); ++i) {
			dest.beginRawPut();
			dest.rawPut().put(src.dataAt(i));
			dest.endRawPut();
		}
	}

};


template<typename TValue>
Deque<TValue>::Deque() : RefCountObject() {}

template<typename TValue>
Deque<TValue>::Deque(const UByteArrayAdapter & data) :
RefCountObject(),
m_index(data + (1 + UByteArrayAdapter::OffsetTypeSerializedLength() + data.getOffset(1))),
m_data(UByteArrayAdapter(data, 1+UByteArrayAdapter::OffsetTypeSerializedLength(), data.getOffset(1)))
{
SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_DEQUE_VERSION, data.at(0), "Static::Deque");
SSERIALIZE_LENGTH_CHECK(m_index.size()*sserialize::SerializationInfo<TValue>::min_length, m_data.size(), "Static::Deque::Deque::Insufficient data");
}

template<typename TValue>
TValue
Deque<TValue>::at(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return TValue();
	}
	return TValue(dataAt(pos));
}

template<typename TValue>
TValue
Deque<TValue>::front() const {
	return at(0);
}

template<typename TValue>
TValue
Deque<TValue>::back() const {
	return at(size()-1);
}

template<typename TValue>
UByteArrayAdapter
Deque<TValue>::dataAt(uint32_t pos) const {
	if (pos >= size() || size() == 0) {
		return UByteArrayAdapter();
	}
	UByteArrayAdapter::OffsetType begin = m_index.at(pos);
	UByteArrayAdapter::OffsetType len;
	if (pos+1 == size()) {
		len = m_data.size()-begin;
	}
	else {
		len = m_index.at(pos+1);
	}
	return UByteArrayAdapter(m_data, begin, len);
}

template<typename TValue>
int32_t Deque<TValue>::find(const TValue& value) const {
	for(uint32_t i = 0; i < size(); i++) {
		if (at(i) == value)
			return i;
	}
	return -1;
}

//Template specialications for integral types

template<>
int32_t
Deque<int32_t>::at(uint32_t pos) const;

template<>
uint32_t
Deque<uint32_t>::at(uint32_t pos) const;


template<>
uint16_t
Deque<uint16_t>::at(uint32_t pos) const;

template<>
uint8_t
Deque<uint8_t>::at(uint32_t pos) const;

template<>
std::string
Deque<std::string>::at(uint32_t pos) const;

template<>
UByteArrayAdapter
Deque<UByteArrayAdapter>::at(uint32_t pos) const;

}}//end namespace

template<typename TValue>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::deque<TValue> & source) {
	sserialize::Static::DequeCreator<TValue> dc(destination);
	dc.reserveOffsets(source.size());
	for(std::size_t i = 0, s = source.size(); i < s; ++i) {
		dc.put(source[i]);
	}
	dc.flush();
	return destination;
}

template<typename TValue>
sserialize::UByteArrayAdapter& operator<<(sserialize::UByteArrayAdapter & destination, const std::vector<TValue> & source) {
	sserialize::Static::DequeCreator<TValue> dc(destination);
	dc.reserveOffsets(source.size());
	for(std::size_t i = 0, s = source.size(); i < s; ++i) {
		dc.put(source[i]);
	}
	dc.flush();
	return destination;
}

template<typename TValue>
sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::Static::Deque<TValue> & destination) {
	sserialize::UByteArrayAdapter tmpAdap(source);
	tmpAdap.shrinkToGetPtr();
	destination = sserialize::Static::Deque<TValue>(tmpAdap);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}

template<typename TValue>
bool operator==(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
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
bool operator==(const sserialize::Static::Deque<TValue> & dequeA, const std::deque<TValue> & dequeB) {
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
bool operator==(const std::deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return dequeB == dequeA;
}

template<typename TValue>
bool operator<(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
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
bool operator<(const sserialize::Static::Deque<TValue> & dequeA, const std::deque<TValue> & dequeB) {
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
bool operator<(const std::deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
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
bool operator!=(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator!=(const sserialize::Static::Deque<TValue> & dequeA, const std::deque<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator!=(const std::deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return !(dequeA == dequeB);
}

template<typename TValue>
bool operator>(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return dequeB < dequeA;
}

template<typename TValue>
bool operator<=(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return ! (dequeA > dequeB);
}

template<typename TValue>
bool operator>=(const sserialize::Static::Deque<TValue> & dequeA, const sserialize::Static::Deque<TValue> & dequeB) {
	return ! (dequeA < dequeB);
}

#endif
