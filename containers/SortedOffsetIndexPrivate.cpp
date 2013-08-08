#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/log.h>
#include <sserialize/utility/pack_unpack_functions.h>

namespace sserialize {
namespace Static {

SortedOffsetIndexPrivate::SortedOffsetIndexPrivate() :
m_size(0),
m_slopenom(0),
m_yintercept(0),
m_idOffset(0)
{}

bool SortedOffsetIndexPrivate::init(UByteArrayAdapter data) {
	
	int curLen;
	uint64_t size = data.getVlPackedUint64(0, &curLen);
	if (curLen < 0) {
		osmfindlog::err("SortedOffsetIndexPrivate::init", "ERROR: Failed to retrieve m_size");
		return false;
	}
	data += curLen;
	uint8_t bpn = (size & 0x3F)+1;
	m_size = size >> 6;
	
	if (m_size > 1) {
		m_yintercept = data.getVlPackedInt64(0, &curLen);
		if (curLen < 0) {
			osmfindlog::err("SortedOffsetIndexPrivate::init", "ERROR: Failed to retrieve m_yintercept");
			m_size = 0;
			m_yintercept = 0;
			return false;
		}
		data += curLen;
		m_slopenom = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			osmfindlog::err("SortedOffsetIndexPrivate::init","ERROR:  Failed to retrieve m_slopenom");
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			return false;
		}
		data += curLen;

		m_idOffset = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			osmfindlog::err("SortedOffsetIndexPrivate::init","ERROR:  Failed to retrieve m_idOffset");
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			m_idOffset = 0;
			return false;
		}
		data += curLen;
		m_idStore = CompactUintArray(data, bpn);
		bool idStoreHasEnoughIds = (m_idStore.maxCount() >= m_size);
		if (!idStoreHasEnoughIds) {
			osmfindlog::err("SortedOffsetIndexPrivate::init","ERROR:  Id store has not enought ids");
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			m_idOffset = 0;
		}
		return idStoreHasEnoughIds;
	}
	else {
		m_slopenom = 0;
		m_idOffset = 0;
		m_yintercept = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			osmfindlog::err("SortedOffsetIndexPrivate::init", "ERROR: Failed to retrieve m_yinterecept with m_size=1");
			m_size = 0;
			m_yintercept = 0;
			return false;
		}
		return true;
	}
}

SortedOffsetIndexPrivate::SortedOffsetIndexPrivate(const UByteArrayAdapter & data) :
	m_size(0),
	m_slopenom(0),
	m_yintercept(0),
	m_idOffset(0)
{
	if (!init(data))
		throw sserialize::CorruptDataException("SortedOffsetIndexPrivate");
}

SortedOffsetIndexPrivate::~SortedOffsetIndexPrivate() {}

UByteArrayAdapter::OffsetType SortedOffsetIndexPrivate::getSizeInBytes() const {
	switch (m_size) {
	case 0: return 1;
	case 1: return 1+vl_pack_uint64_t_size(m_yintercept);
	default:
		UByteArrayAdapter::OffsetType size = vl_pack_uint64_t_size( static_cast<uint64_t>(m_size) << 6);
		size += vl_pack_uint64_t_size(m_slopenom);
		size += vl_pack_int64_t_size(m_yintercept);
		size += vl_pack_uint64_t_size(m_idOffset);
		uint64_t idStoreBitCount = static_cast<uint64_t>(m_size)*m_idStore.bpn();
		size += idStoreBitCount/8 + (idStoreBitCount % 8 ? 1 : 0);
		return size;
	}
}


uint32_t SortedOffsetIndexPrivate::size() const {
	return m_size;
}

UByteArrayAdapter::OffsetType SortedOffsetIndexPrivate::at(uint32_t pos) const {
	if (pos >= m_size)
		pos = m_size-1; //if size==1 => pos = 0
	if (m_size > 1) {
		return getRegLineSlopeCorrectionValue(m_slopenom, m_size, pos) + m_idStore.at64(pos) + m_yintercept-m_idOffset;
	}
	else {
		return m_yintercept;
	}
}

SortedOffsetIndexPrivateEmpty::SortedOffsetIndexPrivateEmpty() : SortedOffsetIndexPrivate() {}
SortedOffsetIndexPrivateEmpty::~SortedOffsetIndexPrivateEmpty() {}
UByteArrayAdapter::OffsetType SortedOffsetIndexPrivateEmpty::getSizeInBytes() const {
	return 0;
}




}}//end namespace