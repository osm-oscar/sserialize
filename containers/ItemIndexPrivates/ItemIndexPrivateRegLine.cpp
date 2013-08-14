#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>
#include <sserialize/utility/LinearRegressionFunctions.h>
#include <sserialize/utility/log.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/exceptions.h>
#include <iostream>
#include <cmath>

namespace sserialize {

bool ItemIndexPrivateRegLine::init(UByteArrayAdapter index) {
	
	int curLen;
	m_size = index.getVlPackedUint32(0, &curLen); //vl_unpack_uint32_t(m_index.data(), &curLen);
	if (curLen < 0) {
		sserialize::err("ItemIndexPrivateRegLine::init", "ERROR: Index is invalid");
		return false;
	}
	index += curLen;
	m_bpn = (m_size & 0x1F)+1;
	m_size = m_size >> 5;
	
	if (m_size > 1) {
		m_yintercept = index.getVlPackedInt32(0, &curLen); //vl_unpack_int32_t(m_index.data(), &curLen);
		if (curLen < 0) {
			sserialize::err("ItemIndexPrivateRegLine::init", "ERROR: Index is invalid");
			m_size = 0;
			m_yintercept = 0;
			return false;
		}
		index += curLen;
		m_slopenom = index.getVlPackedUint32(0, &curLen); //vl_unpack_uint32_t(m_index.data(), &curLen);
		if (curLen < 0) {
			sserialize::err("ItemIndexPrivateRegLine::init","ERROR: Index is invalid");
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			return false;
		}
		index += curLen;

		m_idOffset = index.getVlPackedUint32(0, &curLen); //vl_unpack_uint32_t(m_index.data(), &curLen);
		if (curLen < 0) {
			sserialize::err("ItemIndexPrivateRegLine::init","ERROR: Index is invalid");
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			m_idOffset = 0;
			return false;
		}
		index += curLen;
	}
	else {
		m_yintercept = 0;
		m_slopenom = 0;
		m_idOffset = 0;
	}
	
	if (m_size > 0)
		m_idStore = CompactUintArray(index, m_bpn);
		
	
	return (m_idStore.maxCount() >= m_size);
}

ItemIndexPrivateRegLine::ItemIndexPrivateRegLine() :
	ItemIndexPrivate(),
	m_size(0),
	m_bpn(0),
	m_slopenom(0),
	m_yintercept(0),
	m_idOffset(0)
{}

ItemIndexPrivateRegLine::ItemIndexPrivateRegLine(const ItemIndexPrivateRegLine & idx) :
	ItemIndexPrivate(),
	m_idStore(idx.m_idStore),
	m_size(idx.m_size),
	m_bpn(idx.m_bpn),
	m_slopenom(idx.m_slopenom),
	m_yintercept(idx.m_yintercept),
	m_idOffset(idx.m_idOffset)
{}

ItemIndexPrivateRegLine::ItemIndexPrivateRegLine(const UByteArrayAdapter & index) :
	m_bpn(0),
	m_slopenom(0),
	m_yintercept(0),
	m_idOffset(0)
{
	if (!init(index))
		throw sserialize::CorruptDataException("ItemIndexPrivateRegLine");
}


ItemIndexPrivateRegLine::
ItemIndexPrivateRegLine(uint32_t size, const CompactUintArray & idStore, uint8_t bpn, uint32_t slopenom, int32_t yintercept, uint32_t idOffSet) :
ItemIndexPrivate(),
m_idStore(idStore),
m_size(size),
m_bpn(bpn),
m_slopenom(slopenom),
m_yintercept(yintercept),
m_idOffset(idOffSet)
{}

ItemIndex::Types ItemIndexPrivateRegLine::type() const {
	return ItemIndex::T_REGLINE;
}


int ItemIndexPrivateRegLine::binarySearchIdInIndex(uint32_t id) const {
	uint32_t len = size();
	if (len == 0) return -1;
	int left = 0;
	int right = len-1;
	int mid = (right-left)/2+left;
	uint32_t curId;
	while( left < right ) {
		curId = at(mid);
		if (curId == id) return mid;
		if (curId < id) { // key should be to the right
			left = mid+1;
		}
		else { // key should be to the left of mid
			right = mid-1;
		}
		mid = (right-left)/2+left;
	}
	curId = at(mid);
	return ((curId == id) ? mid : -1);
}

uint32_t ItemIndexPrivateRegLine::at(uint32_t pos) const {
	if (pos >= m_size)
		pos = m_size-1; //if size==1 => pos = 0
	uint32_t id;
	if (m_size > 1) {
		id =  getRegLineSlopeCorrectionValue(m_slopenom, m_size, pos) + m_idStore.at(pos) + m_yintercept-m_idOffset;
	}
	else {
		id = m_idStore.at(0);
	}
	return id;
}

uint32_t ItemIndexPrivateRegLine::first() const {
	if (m_size)
		return at(0);
	else
		return 0;
}

uint32_t ItemIndexPrivateRegLine::last() const {
	if (m_size)
		return at(m_size-1);
	else
		return 0;
}

uint32_t ItemIndexPrivateRegLine::size() const {
	return m_size;
}

uint32_t ItemIndexPrivateRegLine::rawIdAt(const uint32_t pos) const {
	if (pos >= m_size)
		return 0;
	return m_idStore.at(pos);
}

//Statistics

uint32_t ItemIndexPrivateRegLine::getSizeInBytes() const {
	return getHeaderbytes() + getRegressionLineBytes() + getIdBytes();
}

uint32_t ItemIndexPrivateRegLine::getHeaderbytes() const {
	if (m_size == 0)
		return 1;
	return vl_pack_uint32_t_size(m_size << 5);
}

uint32_t ItemIndexPrivateRegLine::getRegressionLineBytes() const {
	if (m_size == 0) return 0;
	uint32_t size = 0;
	if (m_size > 1) {
		size += vl_pack_uint32_t_size(m_slopenom);
		size += vl_pack_int32_t_size(m_yintercept);
		size += vl_pack_uint32_t_size(m_idOffset);
	}
	return size;
}

uint32_t ItemIndexPrivateRegLine::getIdBytes() const {
	if (m_size == 0)
		return 0;
	uint32_t size = 0;
	size += (m_size*m_bpn)/8 + ((m_size*m_bpn) % 8 ? 1 : 0);
	return size;
}


void ItemIndexPrivateRegLine::addFixedSizeHeaderRegLine(uint32_t idsInSet, uint8_t storageBits, uint32_t lowestId, const CompactUintArray & carray, UByteArrayAdapter & adap) {
	uint32_t hCountBits = (idsInSet << 5);
	if (idsInSet != 1) {
		hCountBits |= (storageBits-1);
		adap.putVlPackedPad4Uint32(0, hCountBits);

		adap.putVlPackedPad4Int32(4, lowestId); //y-intercept
		adap.putVlPackedPad4Uint32(8, 0); // slopenom
		adap.putVlPackedPad4Uint32(12, 0); //idoffset
	}
	else {
		lowestId += carray.at(0);
		storageBits = CompactUintArray::minStorageBits(lowestId);
		hCountBits |= (storageBits-1);
		adap.putVlPackedPad4Uint32(0, hCountBits);
		CompactUintArray carr(adap+4, storageBits);
		carr.set(0, lowestId);
	}
}

}//end namespace