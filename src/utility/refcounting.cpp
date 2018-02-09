#include <sserialize/utility/refcounting.h>

namespace sserialize {

#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
std::atomic<uint64_t> RefCountObjectBase::GlobalRc{0};
std::atomic<uint64_t> RefCountObjectBase::GlobalRcChanges{0};
#endif

RefCountObjectBase::RefCountObjectBase() : m_rc(0) {}
RefCountObjectBase::~RefCountObjectBase() {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(RCBaseType(0), m_rc);
}

void RefCountObjectBase::rcReset() {
	m_rc = 0;
}

RefCountObjectBase::RCBaseType RefCountObjectBase::rc() const {
	return m_rc;
}

void RefCountObjectBase::rcInc() {
	m_rc.fetch_add(1, std::memory_order_relaxed);
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_add(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
}

void RefCountObjectBase::rcDec() {
	SSERIALIZE_CHEAP_ASSERT(rc() > 0);
	if (m_rc.fetch_sub(1, std::memory_order_acq_rel) == 1) { //check if we are the last
		delete this;
	}
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_sub(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
}

void RefCountObjectBase::rcDecWithoutDelete() {
	SSERIALIZE_CHEAP_ASSERT_LARGER(rc(), RCBaseType(0));
	m_rc.fetch_sub(1, std::memory_order_acq_rel);
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_sub(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
}

RefCountObject::~RefCountObject() {}

RefCountObjectWithDisable::~RefCountObjectWithDisable() {}

void RefCountObjectWithDisable::disableRC() {
	m_enabled = false;
}

void RefCountObjectWithDisable::enableRC() {
	m_enabled = true;
}

bool RefCountObjectWithDisable::enabledRC() const {
	return m_enabled;
}

}//end namespace
