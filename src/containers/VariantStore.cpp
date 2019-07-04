#include <sserialize/containers/VariantStore.h>
#include <sserialize/algorithm/hashspecializations.h>

namespace sserialize {

VariantStore::VariantStore(UByteArrayAdapter dest, sserialize::MmappedMemoryType mmt) :
m_data(dest),
m_ac(&m_data, Serializer(), sserialize::MMVector<uint64_t>(mmt)),
m_hashList(mmt)
{}


VariantStore::VariantStore(sserialize::MmappedMemoryType mmt) :
VariantStore(sserialize::UByteArrayAdapter::createCache(1024, mmt), mmt)
{}

VariantStore::VariantStore(VariantStore && other) :
m_data(std::move(other.m_data)),
m_ac(std::move(other.m_ac)),
m_hash(std::move(other.m_hash)),
m_hashList(std::move(other.m_hashList)),
m_hitCount(other.m_hitCount.load()),
m_ddm(other.m_ddm)
{}

VariantStore::~VariantStore() {}

VariantStore & VariantStore::operator=(VariantStore && other) {
	m_data = std::move(other.m_data);
	m_ac = std::move(other.m_ac);
	m_hash = std::move(other.m_hash);
	m_hashList = std::move(other.m_hashList);
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

VariantStore::HashValue VariantStore::hashFunc(const UByteArrayAdapter::MemoryView & v) {
	uint64_t h = 0;
	for(UByteArrayAdapter::MemoryView::const_iterator it(v.cbegin()), end(v.cend()); it != end; ++it) {
		hash_combine(h, *it);
	}
	return h;
}

bool VariantStore::dataInStore(const UByteArrayAdapter::MemoryView & v, IdType id) {
	m_dataLock.acquireReadLock();
	UByteArrayAdapter tmp = m_ac.dataAt(id);
	if (tmp.size() != v.size()) {
		m_dataLock.releaseReadLock();
		return false;
	}
	else {
		UByteArrayAdapter::MemoryView mv( tmp.asMemView() );
		bool eq = memcmp(mv.get(), v.get(), v.size()) == 0;
		m_dataLock.releaseReadLock();
		return eq;
	}
}

VariantStore::IdType VariantStore::getStoreId(const UByteArrayAdapter::MemoryView & v, uint64_t hv) {
	m_hashLock.acquireReadLock();
	if (m_hash.count(hv) == 0) {
		m_hashLock.releaseReadLock();
		return nid;
	}
	else {
		HashListEntry hle = m_hashList.at(m_hash[hv]);
		m_hashLock.releaseReadLock();
		while (true) {
			if (dataInStore(v, hle.id)) {
				return hle.id;
			}
			if (hle.prev) {
				m_hashLock.acquireReadLock();
				hle = m_hashList.at(hle.prev);
				m_hashLock.releaseReadLock();
			}
			else {
				break;
			}
		}
	}
	return nid;
}

VariantStore::IdType VariantStore::insert(const sserialize::UByteArrayAdapter & data, DeduplicationMode ddm) {
	if (ddm == DDM_DEFAULT) {
		ddm = m_ddm;
	}
	sserialize::UByteArrayAdapter::MemoryView mv(data.asMemView());
	HashValue hv = hashFunc(mv);
	IdType id = (ddm == DDM_FORCE_ON ? getStoreId(mv, hv) : nid);
	if (id == VariantStore::nid) {
		m_dataLock.acquireWriteLock();
		id = m_ac.size();
		m_ac.put(data);
		m_dataLock.releaseWriteLock();
		
		m_hashLock.acquireWriteLock();
		uint64_t prevElement = 0;
		if (m_hash.count(hv)) {
			prevElement = m_hash[hv];
		}
		m_hash[hv] = m_hashList.size();
		m_hashList.emplace_back(prevElement, id);
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
