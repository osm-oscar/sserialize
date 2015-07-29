#ifndef SSERIALIZE_STATIC_DENSE_GEOPOINT_VECTOR_H
#define SSERIALIZE_STATIC_DENSE_GEOPOINT_VECTOR_H
#include <iterator>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/templated/AbstractArray.h>
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {
namespace Static {
namespace spatial {

/** File Format
  *
  *----------------------------------------------
  *DATASIZE|SIZE|(IntLat|IntLon)|(IntLat|IntLon)
  *----------------------------------------------
  * vu32   |vu32|  (vu32|vu32)  |(vs32|vs32)
  *
  *
  */

class DenseGeoPointVector {
public:
	class ForwardIterator: public std::iterator<std::input_iterator_tag, GeoPoint, int> {
		UByteArrayAdapter m_d;
		uint32_t m_pos;
		uint32_t m_size;
		uint32_t m_intLat;
		uint32_t m_intLon;
	public:
		ForwardIterator();
		ForwardIterator(const ForwardIterator & other);
		ForwardIterator(const UByteArrayAdapter & d, uint32_t size, bool end);
		virtual ~ForwardIterator() {}
		inline GeoPoint operator*() const {
			return GeoPoint::fromIntLatLon(m_intLat, m_intLon);
		}
		ForwardIterator & operator++();
		
		ForwardIterator operator++(int num);
		
		ForwardIterator operator+(uint32_t diff) const;
		
		inline bool operator!=(const ForwardIterator & o) const {
			return (m_pos != o.m_pos);
		}
		
		inline bool operator<(const ForwardIterator & o) const {
			return m_pos < o.m_pos;
		}
		
		inline int32_t operator-(const ForwardIterator & o) {
			return m_pos - o.m_pos;
		}
		
	};
	
	typedef ForwardIterator const_iterator;
	typedef ForwardIterator iterator;
	typedef sserialize::spatial::GeoPoint const_reference;
public:
	uint32_t m_size;
	UByteArrayAdapter m_data;
public:
	DenseGeoPointVector() : m_size(0) {}
	DenseGeoPointVector(sserialize::UByteArrayAdapter data) :
	m_size(data.resetPtrs().getVlPackedUint32() ) {
		if (m_size) {
			uint32_t dS = data.getVlPackedUint32();
			data.shrinkToGetPtr();
			m_data = UByteArrayAdapter(data, 0, dS);
		}
	} 
	virtual ~DenseGeoPointVector() {}
	inline uint32_t getSizeInBytes() const {
		uint32_t res = psize_vu32(m_size);
		if (m_size) {
			res += m_data.size();
			res += psize_vu32(m_data.size());
		}
		return res;
	}
	inline uint32_t size() const { return m_size; }
	inline ForwardIterator cbegin() const { return const_iterator(m_data, size(), false); }
	inline ForwardIterator cend() const { return const_iterator(m_data, size(), true); }

	inline GeoPoint at(uint32_t pos) const {
		if (pos < m_size) {
			return *(cbegin()+pos);
		}
		return GeoPoint();
	}
	
	inline GeoPoint front() const {
		return at(0);
	}
	
	inline GeoPoint back() const {
		return at(m_size-1);
	}

	inline ForwardIterator begin() const { return cbegin(); }
	inline ForwardIterator end() const { return cend(); }

	template<typename TGeoPointIterator>
	static UByteArrayAdapter & append(TGeoPointIterator begin, TGeoPointIterator end, sserialize::UByteArrayAdapter & dest);
};

namespace detail {

typedef sserialize::detail::AbstractArrayIteratorDefaultImp< DenseGeoPointVector::ForwardIterator, GeoPoint > DenseGeoPointVectorAbstractArrayIterator;
typedef sserialize::detail::AbstractArrayDefaultImp< DenseGeoPointVector, GeoPoint, DenseGeoPointVectorAbstractArrayIterator > DenseGeoPointVectorAbstractArray;

}

typedef sserialize::AbstractArray<GeoPoint> DenseGeoPointAbstractArray;

//---------Definitions---------------------

template<typename TGeoPointIterator>
UByteArrayAdapter& DenseGeoPointVector::append(TGeoPointIterator begin, TGeoPointIterator end, sserialize::UByteArrayAdapter& dest) {
	uint32_t size = std::distance(begin, end);
	dest.putVlPackedUint32(size);
	if (size) {
		std::vector<uint8_t> d;
		UByteArrayAdapter tempDest(&d, false);
		sserialize::spatial::GeoPoint gp = *begin;
		int64_t prevLat = gp.toIntLat(gp.lat());
		int64_t prevLon = gp.toIntLon(gp.lon());
		
		tempDest.putVlPackedUint32(prevLat);
		tempDest.putVlPackedUint32(prevLon);
		
		for(++begin; begin != end; ++begin) {
			gp = *begin;
			int64_t intLat = gp.toIntLat(gp.lat());
			int64_t intLon = gp.toIntLon(gp.lon());
			int64_t diffLat = intLat - prevLat;
			int64_t diffLon = intLon - prevLon;
			
			prevLat = intLat;
			prevLon = intLon;
			tempDest.putVlPackedInt32(diffLat);
			tempDest.putVlPackedInt32(diffLon);
		}
		dest.putVlPackedUint32(d.size());
		dest.putData(d);
	}
	
	return dest;
}

}}}//end namespace

#endif