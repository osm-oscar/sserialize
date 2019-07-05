#include <sserialize/containers/VariantStore.h>
#include <sserialize/algorithm/hashspecializations.h>

namespace sserialize {

VariantStore::VariantStore(UByteArrayAdapter dest, sserialize::MmappedMemoryType mmt) :
m_data(dest),
m_ac(&m_data, Serializer(), sserialize::MMVector<uint64_t>(mmt))
{}


VariantStore::VariantStore(sserialize::MmappedMemoryType mmt) :
VariantStore(sserialize::UByteArrayAdapter::createCache(1024, mmt), mmt)
{}

VariantStore::VariantStore(VariantStore && other) :
m_data(std::move(other.m_data)),
m_ac(std::move(other.m_ac)),
m_hash(std::move(other.m_hash)),
m_hitCount(other.m_hitCount.load()),
m_ddm(other.m_ddm)
{}

VariantStore::~VariantStore() {}

VariantStore & VariantStore::operator=(VariantStore && other) {
	m_data = std::move(other.m_data);
	m_ac = std::move(other.m_ac);
	m_hash = std::move(other.m_hash);
	m_hitCount.store(other.m_hitCount.load());
	return *this;
}

void VariantStore::setDDM(bool useDeduplication) {
	if (useDeduplication) {
		m_ddm = DDM_FORCE_ON;
	}
	else {
		m_ddm = DDM_FORCE_OFF;
	}
}

VariantStore::SizeType VariantStore::size() const {
	return m_ac.size();
}

VariantStore::SizeType VariantStore::hitCount() const {
	return m_hitCount.load();
}

VariantStore::DataHashKey VariantStore::hashFunc(const UByteArrayAdapter & v) {
	sserialize::ShaHasher<UByteArrayAdapter> hasher;
	return hasher(v);
}

VariantStore::IdType VariantStore::getStoreId(DataHashKey const & hv) {
	m_hashLock.acquireReadLock();
	if (m_hash.count(hv) == 0) {
		m_hashLock.releaseReadLock();
		return nid;
	}
	else {
		IdType id = m_hash.at(hv);
		m_hashLock.releaseReadLock();
		return id;
	}
}

VariantStore::IdType VariantStore::insert(const sserialize::UByteArrayAdapter & data, DeduplicationMode ddm) {
	if (ddm == DDM_DEFAULT) {
		ddm = m_ddm;
	}
	auto hv = hashFunc(data);
	IdType id = (ddm == DDM_FORCE_ON ? getStoreId(hv) : nid);
	if (id == VariantStore::nid) {
		m_dataLock.acquireWriteLock();
		id = m_ac.size();
		m_ac.put(data);
		m_dataLock.releaseWriteLock();
		
		m_hashLock.acquireWriteLock();
		m_hash[hv] = id;
		m_hashLock.releaseWriteLock();
	}
	else {
		++m_hitCount;
	}
	return id;
}

UByteArrayAdapter VariantStore::at(IdType id) const {
	if (id >= size()) {
		throw sserialize::OutOfBoundsException("sserialize::DataSetFactory::at");
	}
	return m_ac.dataAt(id);
}

void VariantStore::flush() {
	m_ac.flush();
}

UByteArrayAdapter VariantStore::getFlushedData() const {
	return m_ac.getFlushedData();
}

}//end namespace
