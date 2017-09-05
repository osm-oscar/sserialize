#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateEliasFano.h>
#include <sserialize/storage/pack_unpack_functions.h>

namespace sserialize {
namespace detail {
namespace ItemIndexImpl {


EliasFanoIterator::EliasFanoIterator(const CompactUintArray::const_iterator & lb, const UnaryCodeIterator & ub, uint32_t lastUb, uint8_t numLowerBits) :
m_lb(lb),
m_ub(ub),
m_lastUb(lastUb),
m_baseValue(0),
m_numLowerBits(numLowerBits)
{}

EliasFanoIterator::EliasFanoIterator(const CompactUintArray::const_iterator & lb, const UnaryCodeIterator & ub) :
m_lb(lb),
m_ub(ub),
m_lastUb(0),
m_baseValue(0),
m_numLowerBits(lb.data().bpn())
{}

EliasFanoIterator::EliasFanoIterator(const CompactUintArray::const_iterator & lb) :
m_lb(lb),
m_ub(),
m_lastUb(0),
m_baseValue(0),
m_numLowerBits(lb.data().bpn())
{}

EliasFanoIterator::~EliasFanoIterator() {}

EliasFanoIterator::value_type
EliasFanoIterator::get() const {
	if (m_numLowerBits) {
		return (((m_lastUb + *m_ub) << m_numLowerBits) | *m_lb) + m_baseValue;
	}
	else {
		return (m_lastUb + *m_ub) + m_baseValue;
	}
}

void
EliasFanoIterator::next() {
	m_lastUb += *m_ub;
	m_baseValue += 1;
	
	++m_ub;
	++m_lb;
}

bool
EliasFanoIterator::notEq(const MyBaseClass * other) const {
	const EliasFanoIterator * myOther = dynamic_cast<const EliasFanoIterator*>(other);
	
	if (!myOther) {
		return true;
	}

	return m_lb != myOther->m_lb;
}

bool EliasFanoIterator::eq(const MyBaseClass * other) const {
	const EliasFanoIterator * myOther = dynamic_cast<const EliasFanoIterator*>(other);
	
	if (!myOther) {
		return false;
	}
	
	return m_lb == myOther->m_lb;
}

EliasFanoIterator::MyBaseClass * EliasFanoIterator::copy() const {
	return new EliasFanoIterator(m_lb, m_ub, m_lastUb, m_numLowerBits);
}

//BEGIN CREATOR

EliasFanoCreator::EliasFanoCreator(uint32_t /*maxId*/) :
m_data(new UByteArrayAdapter( UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY) )),
m_putPtr(0),
m_delete(true)
{}

EliasFanoCreator::EliasFanoCreator(sserialize::UByteArrayAdapter& data, uint32_t /*maxId*/) :
m_data(&data),
m_putPtr(data.tellPutPtr()),
m_delete(false)
{}

EliasFanoCreator::EliasFanoCreator(EliasFanoCreator&& other) :
m_values(std::move(other.m_values)),
m_data(other.m_data),
m_putPtr(other.m_putPtr),
m_delete(other.m_delete)
{
	other.m_data = 0;
	other.m_delete = false;
}

EliasFanoCreator::~EliasFanoCreator() {
	if (m_delete) {
		delete m_data;
	}
}

uint32_t EliasFanoCreator::size() const {
	return m_values.size();
}

void EliasFanoCreator::push_back(uint32_t id) {
	if (size() && m_values.back() >= id) {
		throw std::domain_error("sserialize::ItemIndexPrivateEliasFanoCreator: ids have to be strongly-monotone");
	}
	m_values.push_back(id);
}

void EliasFanoCreator::flush() {
	data().setPutPtr(m_putPtr);
	ItemIndexPrivateEliasFano::create(m_values, data());
}


UByteArrayAdapter EliasFanoCreator::flushedData() const {
	UByteArrayAdapter d(data());
	d.setGetPtr(m_putPtr);
	return d;
}

ItemIndex EliasFanoCreator::getIndex() {
	return ItemIndex(flushedData(), ItemIndex::T_ELIAS_FANO);
}

ItemIndexPrivate * EliasFanoCreator::getPrivateIndex() {
	return new ItemIndexPrivateEliasFano(flushedData());
}

UByteArrayAdapter& EliasFanoCreator::data() {
	return *m_data;
}

const UByteArrayAdapter& EliasFanoCreator::data() const {
	return *m_data;
}



//END CREATOR

//BEGIN GenericSetOpExecuterInit

template<>
struct GenericSetOpExecuterInit<EliasFanoCreator, sserialize::detail::ItemIndexImpl::IntersectOp> {
	using Creator = EliasFanoCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::IntersectOp;
	
	static Creator init(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		const ItemIndexPrivateEliasFano * mfirst = dynamic_cast<const ItemIndexPrivateEliasFano *>(first);
		const ItemIndexPrivateEliasFano * msecond = dynamic_cast<const ItemIndexPrivateEliasFano *>(second);
		return Creator(std::min<uint32_t>(mfirst->maxId(), msecond->maxId()));
	}
};

template<>
struct GenericSetOpExecuterInit<EliasFanoCreator, sserialize::detail::ItemIndexImpl::UniteOp> {
	using Creator = EliasFanoCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::UniteOp;
	
	static Creator init(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		const ItemIndexPrivateEliasFano * mfirst = dynamic_cast<const ItemIndexPrivateEliasFano *>(first);
		const ItemIndexPrivateEliasFano * msecond = dynamic_cast<const ItemIndexPrivateEliasFano *>(second);
		return Creator(std::max<uint32_t>(mfirst->maxId(), msecond->maxId()));
	}
};

template<>
struct GenericSetOpExecuterInit<EliasFanoCreator, sserialize::detail::ItemIndexImpl::DifferenceOp> {
	using Creator = EliasFanoCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::DifferenceOp;
	
	static Creator init(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* /*second*/) {
		const ItemIndexPrivateEliasFano * mfirst = dynamic_cast<const ItemIndexPrivateEliasFano *>(first);
		return Creator(mfirst->maxId());
	}
};

template<>
struct GenericSetOpExecuterInit<EliasFanoCreator, sserialize::detail::ItemIndexImpl::SymmetricDifferenceOp> {
	using Creator = EliasFanoCreator;
	using SetOpTraits = sserialize::detail::ItemIndexImpl::UniteOp;
	
	static Creator init(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		const ItemIndexPrivateEliasFano * mfirst = dynamic_cast<const ItemIndexPrivateEliasFano *>(first);
		const ItemIndexPrivateEliasFano * msecond = dynamic_cast<const ItemIndexPrivateEliasFano *>(second);
		return Creator(std::max<uint32_t>(mfirst->maxId(), msecond->maxId()));
	}
};

//END GenericSetOpExecuterInit

}} //end namespace detail::ItemIndexImpl

//BEGIN INDEX

ItemIndexPrivateEliasFano::ItemIndexPrivateEliasFano(const UByteArrayAdapter & d) :
m_d(d),
m_size(m_d.getVlPackedUint32(0)),
m_maxIdBegin(sserialize::psize_v<uint32_t>(m_size)),
m_lowerBitsBegin(m_size ? m_maxIdBegin+sserialize::psize_v<uint32_t>(maxId()) : m_maxIdBegin),
m_upperBitsBegin(m_size ? sserialize::psize_v<uint32_t>(upperBitsDataSize()) : m_maxIdBegin),
m_it(cbegin())
{
	sserialize::UByteArrayAdapter::SizeType totalSize = 0;
	if (m_size) {
		totalSize += m_lowerBitsBegin;
		totalSize += CompactUintArray::minStorageBytes(numLowerBits(), size());
		totalSize += m_upperBitsBegin;
		totalSize += upperBitsDataSize();
	}
	else {
		totalSize += m_maxIdBegin;
	}
	m_d.resize(totalSize);
}

ItemIndexPrivateEliasFano::~ItemIndexPrivateEliasFano() {}

ItemIndex::Types ItemIndexPrivateEliasFano::type() const {
	return ItemIndex::T_ELIAS_FANO;
}

void
ItemIndexPrivateEliasFano::loadIntoMemory() {
	;
}

UByteArrayAdapter ItemIndexPrivateEliasFano::data() const {
    return m_d;
}

uint8_t ItemIndexPrivateEliasFano::bpn() const {
	if (size()) {
		return sserialize::multiplyDiv64(getSizeInBytes(), 8, size());
	}
	else {
		return std::numeric_limits<uint8_t>::max();
	}
}

uint32_t
ItemIndexPrivateEliasFano::at(uint32_t pos) const {
	if (pos >= m_size) {
		throw std::out_of_range("ItemIndex::at with pos=" + std::to_string(pos) + "; size=" + std::to_string(size()));
	}
	
	while(m_cache.size() <= pos) {
		m_cache.emplace_back(*m_it);
		++m_it;
	}
	
	return m_cache[pos];
}

uint32_t
ItemIndexPrivateEliasFano::first() const {
	return at(0);
}

uint32_t
ItemIndexPrivateEliasFano::last() const {
	return at(m_size-1);
}

ItemIndexPrivateEliasFano::const_iterator
ItemIndexPrivateEliasFano::cbegin() const {
	return new detail::ItemIndexImpl::EliasFanoIterator(lowerBits().cbegin(), upperBits());
}

ItemIndexPrivateEliasFano::const_iterator
ItemIndexPrivateEliasFano::cend() const {
	return new detail::ItemIndexImpl::EliasFanoIterator(lowerBits().cbegin()+m_size);
}

uint32_t
ItemIndexPrivateEliasFano::size() const {
	return m_size;
}

sserialize::UByteArrayAdapter::SizeType
ItemIndexPrivateEliasFano::getSizeInBytes() const {
	return m_d.size();
}

bool
ItemIndexPrivateEliasFano::is_random_access() const {
	return false;
}

void
ItemIndexPrivateEliasFano::putInto(DynamicBitSet & bitSet) const {
	bitSet.set(m_cache.cbegin(), m_cache.cend());
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i(m_cache.size()); i < m_size; ++i, ++it) {
			bitSet.set(*it);
		}
	}
}

void
ItemIndexPrivateEliasFano::putInto(uint32_t* dest) const {
	dest = std::copy(m_cache.cbegin(), m_cache.cend(), dest);
	
	if (m_cache.size() < m_size) {
		auto it(m_it);
		for(uint32_t i(m_cache.size()); i < m_size; ++i, ++dest, ++it) {
			*dest = *it;
		}
	}
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::uniteK(const sserialize::ItemIndexPrivate * other, uint32_t /*numItems*/) const {
	return unite(other);
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateEliasFano * cother = dynamic_cast<const ItemIndexPrivateEliasFano*>(other);
	if (!cother) {
		return ItemIndexPrivate::doIntersect(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::IntersectOp,
		detail::ItemIndexImpl::EliasFanoCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateEliasFano * cother = dynamic_cast<const ItemIndexPrivateEliasFano*>(other);
	if (!cother) {
		return ItemIndexPrivate::doUnite(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::UniteOp,
		detail::ItemIndexImpl::EliasFanoCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::difference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateEliasFano * cother = dynamic_cast<const ItemIndexPrivateEliasFano*>(other);
	if (!cother) {
		return ItemIndexPrivate::doDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::DifferenceOp,
		detail::ItemIndexImpl::EliasFanoCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::symmetricDifference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateEliasFano * cother = dynamic_cast<const ItemIndexPrivateEliasFano*>(other);
	if (!cother) {
		return ItemIndexPrivate::doSymmetricDifference(other);
	}
	typedef detail::ItemIndexImpl::GenericSetOpExecuter<
		detail::ItemIndexImpl::SymmetricDifferenceOp,
		detail::ItemIndexImpl::EliasFanoCreator,
		sserialize::ItemIndex::const_iterator
		> SetOpExecuter;
	return SetOpExecuter::execute(this, other);
}

ItemIndexPrivate *
ItemIndexPrivateEliasFano::fromBitSet(const DynamicBitSet & bitSet) {
	sserialize::UByteArrayAdapter tmp(UByteArrayAdapter::createCache(4, sserialize::MM_PROGRAM_MEMORY));
	create(bitSet.cbegin(), bitSet.cend(), tmp);
	tmp.resetPtrs();
	return new ItemIndexPrivateEliasFano(tmp);
}

uint8_t ItemIndexPrivateEliasFano::numLowerBits(uint32_t count, uint32_t max)
{
	if (!count) {
		return 0;
	}

// 	if (max+1 < count) {
// 		throw std::domain_error("sserialize::ItemIndexPrivateEliasFano: expecting strongly monotone sequence");
// 	}
	
	if (max < count) { //exactly one entry which is 0
		return 0;
	}
	
	return std::floor( sserialize::logTo2(max) - sserialize::logTo2(count) );
}

uint32_t ItemIndexPrivateEliasFano::maxId() const {
	if (size()) {
		return m_d.getVlPackedUint32(m_maxIdBegin);
	}
	else {
		return 0;
	}
}

uint32_t ItemIndexPrivateEliasFano::upperBitsDataSize() const {
	if (size()) {
		return m_d.getVlPackedUint32(m_lowerBitsBegin+CompactUintArray::minStorageBytes(numLowerBits(), size()));
	}
	else {
		return 0;
	}
}

CompactUintArray
ItemIndexPrivateEliasFano::lowerBits() const {
	uint8_t nlb = numLowerBits();
	if (size() && nlb) {
		return CompactUintArray(m_d+m_lowerBitsBegin, nlb, size());
	}
	else {
		return CompactUintArray();
	}
}

UnaryCodeIterator ItemIndexPrivateEliasFano::upperBits() const {
	if (size()) {
		sserialize::UByteArrayAdapter::OffsetType ubBegin = m_lowerBitsBegin +
			CompactUintArray::minStorageBytes(numLowerBits(), size()) +
			m_upperBitsBegin;
		return UnaryCodeIterator(m_d+ubBegin);
	}
	else {
		return UnaryCodeIterator();
	}
}

uint8_t ItemIndexPrivateEliasFano::numLowerBits() const {
	return numLowerBits(size(), maxId());
}


//END INDEX

}//end namespace sserialize