#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/debuggerfunctions.h>

namespace sserialize {
	

#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
std::atomic<uint64_t> RefCountObject::GlobalRc{0};
std::atomic<uint64_t> RefCountObject::GlobalRcChanges{0};
#endif

RefCountObject::RefCountObject() :
#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
LocalRcChanges(0),
#endif
m_rc(0)
{}

RefCountObject::~RefCountObject() {
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	sserialize::breakHereIf(LocalRcChanges > 10);
	#endif
	
	SSERIALIZE_CHEAP_ASSERT_EQUAL(RCBaseType(0), m_rc);
}

RefCountObject::RCBaseType RefCountObject::rc() const {
	return m_rc >> 1;
}

void RefCountObject::rcInc() {
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_add(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	LocalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
	
	m_rc.fetch_add(2, std::memory_order_relaxed);
}

void RefCountObject::rcDec() {
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_sub(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	LocalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
	
	SSERIALIZE_CHEAP_ASSERT(rc() > 0);
	if (m_rc.fetch_sub(2, std::memory_order_acq_rel) == 2) { //check if we are the last
		delete this;
	}
}

void RefCountObject::rcDecWithoutDelete() {
	#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
	GlobalRc.fetch_sub(1, std::memory_order_relaxed);
	GlobalRcChanges.fetch_add(1, std::memory_order_relaxed);
	LocalRcChanges.fetch_add(1, std::memory_order_relaxed);
	#endif
	
	SSERIALIZE_CHEAP_ASSERT_LARGER(rc(), RCBaseType(0));
	m_rc.fetch_sub(2, std::memory_order_acq_rel);
}

void RefCountObject::disableRC() {
	m_rc.fetch_or(1, std::memory_order_seq_cst);
}

void RefCountObject::enableRC() {
	m_rc.fetch_and(~RCBaseType(1), std::memory_order_seq_cst);
}

bool RefCountObject::enabledRC() const {
	return (m_rc.load(std::memory_order_relaxed) & 0x1) == 0;
}

}//end namespace
