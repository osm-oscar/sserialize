#include <sserialize/containers/SortedOffsetIndexPrivate.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/log.h>
#include <sserialize/storage/pack_unpack_functions.h>

namespace sserialize {
namespace Static {

SortedOffsetIndexPrivate::SortedOffsetIndexPrivate() :
m_size(0),
m_slopenom(0),
m_yintercept(0),
m_idOffset(0)
{}

void SortedOffsetIndexPrivate::init(UByteArrayAdapter data) {
	
	int curLen;
	uint64_t size = data.getVlPackedUint64(0, &curLen);
	if (curLen < 0) {
		throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Failed to retrieve m_size");
		return;
	}
	data += curLen;
	uint8_t bpn = (size & 0x3F)+1;
	m_size = narrow_check<std::decay_t<decltype(m_size)>>(size >> 6);
	
	if (m_size > 1) {
		m_yintercept = data.getVlPackedInt64(0, &curLen);
		if (curLen < 0) {
			m_size = 0;
			m_yintercept = 0;
			throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Failed to retrieve m_yintercept");
			return;
		}
		data += curLen;
		m_slopenom = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Failed to retrieve m_slopenom");
			return;
		}
		data += curLen;

		m_idOffset = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			m_idOffset = 0;
			throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Failed to retrieve m_idOffset");
			return;
		}
		data += curLen;
		m_idStore = CompactUintArray(data, bpn);
		if (m_idStore.maxCount() < m_size) {
			m_size = 0;
			m_yintercept = 0;
			m_slopenom = 0;
			m_idOffset = 0;
			throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Id store has not enought ids");
			return;
		}
	}
	else if (m_size) {
		m_slopenom = 0;
		m_idOffset = 0;
		m_yintercept = data.getVlPackedUint64(0, &curLen);
		if (curLen < 0) {
			m_size = 0;
			m_yintercept = 0;
			throw sserialize::CorruptDataException("sserialize::SortedOffsetIndexPrivate::init: Failed to retrieve m_yinterecept with m_size=1");
			return;
		}
	}
}

INLINE_WITH_LTO uint64_t SortedOffsetIndexPrivate::getRegLineSlopeCorrectionValue(const uint64_t slopenom, const uint64_t size, const uint64_t pos) {
	#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
	using namespace sserialize::checked;
	#else
	using namespace sserialize::unchecked;
	#endif
	uint64_t int_part = mult(slopenom/(size-1), pos);
	uint64_t frac_part;
	if (__builtin_mul_overflow(slopenom % (size-1), pos, &frac_part)) {
		frac_part = narrow<uint64_t>((__uint128_t(slopenom % (size-1))*__uint128_t(pos))/(size-1));
	}
	else {
		frac_part /= (size-1);
	}
	return add(int_part, frac_part);
}

SortedOffsetIndexPrivate::SortedOffsetIndexPrivate(const UByteArrayAdapter & data) :
	m_size(0),
	m_slopenom(0),
	m_yintercept(0),
	m_idOffset(0)
{
	init(data);
}

SortedOffsetIndexPrivate::~SortedOffsetIndexPrivate() {}

UByteArrayAdapter::OffsetType SortedOffsetIndexPrivate::getSizeInBytes() const {
	switch (m_size) {
	case 0: return 1;
	case 1: return 1+psize_vu64(m_yintercept);
	default:
		UByteArrayAdapter::OffsetType size = psize_vu64( static_cast<uint64_t>(m_size) << 6);
		size += psize_vu64(m_slopenom);
		size += psize_vs64(m_yintercept);
		size += psize_vu64(m_idOffset);
		uint64_t idStoreBitCount = static_cast<uint64_t>(m_size)*m_idStore.bpn();
		size += idStoreBitCount/8 + (idStoreBitCount % 8 ? 1 : 0);
		return size;
	}
}


sserialize::SizeType SortedOffsetIndexPrivate::size() const {
	return m_size;
}

UByteArrayAdapter::OffsetType SortedOffsetIndexPrivate::at(sserialize::SizeType pos) const {
	if (pos >= m_size) {
		throw OutOfBoundsException(pos, m_size);
	}
	if (m_size > 1) {
		#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
		using namespace sserialize::checked;
		#else
		using namespace sserialize::unchecked;
		#endif
		uint64_t positive = add(getRegLineSlopeCorrectionValue(m_slopenom, m_size, pos), m_idStore.at64(pos));
		int64_t negative = sub(m_yintercept, m_idOffset);
		if (negative > 0) {
			return add(positive, uint64_t(negative));
		}
		else {
			return sub(positive, uint64_t(-negative));
		}
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
