#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <sserialize/utility/assert.h>
#include <cstdint>
#include <atomic>
#include <utility>

//enable ref-counting stats
// #define SSERIALIZE_GATHER_STATS_REF_COUNTING

namespace sserialize {
namespace detail {
	template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
	class RCBase;
}

class RefCountObject;

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
class RCPtrWrapper;

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
class RCWrapper;

class RefCountObject {
	template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING> friend class detail::RCBase;
public:
	typedef uint32_t RCBaseType;
#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
public:
	static std::atomic<uint64_t> GlobalRc;
	static std::atomic<uint64_t> GlobalRcChanges;
	std::atomic<uint32_t> LocalRcChanges;
#endif
public:
	RefCountObject(const RefCountObject & other) = delete;
	RefCountObject & operator=(const RefCountObject & other) = delete;
	RefCountObject();
	virtual ~RefCountObject();

	RCBaseType rc() const;
public:
	void rcInc();
	void rcDec();
	void rcDecWithoutDelete();
	void disableRC();
	void enableRC();
	///relaxed memory order!
	bool enabledRC() const;
private:
	///the lower bit indicates if ref-counting is enabled or not
	std::atomic<RCBaseType> m_rc;
};
namespace detail {

template<typename RCObj>
class RCBase<RCObj, true > {
public:
	RCBase(RCObj* p) :
	m_priv(0),
	m_enabled(true)
	{
		reset(p);
	}
	RCBase(RCBase const &) = delete;
	virtual ~RCBase() {
		reset(0);
	}
	RCBase& operator=(RCBase const&) = delete;
	bool enabledRC() {
		return m_enabled;
	}
	void reset(RCObj* p) {
		if (p && p->enabledRC()) {
			p->rcInc();
		}
		rcDec();
		m_priv = p;
		m_enabled = !p || p->enabledRC();
	}
	void rcInc() {
		if (priv() && enabledRC()) {
			priv()->rcInc();
		}
	}
	void rcDec() {
		if (priv() && enabledRC()) {
			priv()->rcDec();
		}
	}
	RCObj * priv() const { return m_priv; }
	RCObj * priv() { return m_priv; }
	
	///Enable reference counting, this is NOT thread-safe
	///No other thread is allowed to change either the reference counter or the state of reference counting during this operation
	void enableRC() {
// 		return;
		if (priv() && !enabledRC()) {
			priv()->rcInc();
			priv()->enableRC();
			m_enabled = true;
		}
	}
	///Disable reference counting, this is NOT thread-safe
	///No other thread is allowed to change either the reference counter or the state of reference counting during this operation
	///Warning: this may leave the object without an owner
	void disableRC() {
// 		return;
		if (priv() && enabledRC()) {
			priv()->disableRC();
			priv()->rcDecWithoutDelete();
			m_enabled = false;
		}
	}
private:
	RCObj * m_priv;
	bool m_enabled;
};

template<typename RCObj>
class RCBase<RCObj, false > {
public:
	RCBase(RCObj* p) : m_priv(0) {
		reset(p);
	}
	RCBase(RCBase const &) = delete;
	virtual ~RCBase() {
		reset(0);
	}
	RCBase& operator=(RCBase const&) = delete;
	bool enabledRC() {
		return true;
	}
	void reset(RCObj* p) {
		if (p) {
			p->rcInc();
		}
		rcDec();
		m_priv = p;
	}
	void rcInc() {
		if (priv()) {
			priv()->rcInc();
		}
	}
	void rcDec() {
		if (priv()) {
			priv()->rcDec();
		}
	}
	RCObj * priv() const { return m_priv; }
	RCObj * priv() { return m_priv; }
private:
	RCObj * m_priv;
};

} //end namespace detail

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING = false>
class RCWrapper: public detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> {
public:
	typedef RCObj element_type;
	friend class RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>;
private:
	typedef detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> MyBaseClass;
public:
	RCWrapper() : MyBaseClass(0) {};
	RCWrapper(RCObj * data) : MyBaseClass(data) {}
	RCWrapper(const RCWrapper & other) : MyBaseClass(other.priv()) {}
	RCWrapper(const RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other);
	virtual ~RCWrapper() {}

	RCWrapper & operator=(const RCWrapper & other) {
		reset(other.priv());
		return *this;
	}
	
	bool operator==(const RCWrapper & other) { return priv() == other.priv(); }
	
	RefCountObject::RCBaseType privRc() const { return (priv() ? priv()->rc() : 0);}
public:
	using MyBaseClass::priv;
	using MyBaseClass::reset;
protected: 
	void setPrivate(RCObj * data) {
		MyBaseClass::reset(data);
	}
};

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING = false>
class RCPtrWrapper final : public detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> {
public:
	using Self = RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>;
	typedef RCObj element_type;
	friend class RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>;
private:
	typedef detail::RCBase<RCObj,T_CAN_DISABLE_REFCOUNTING> MyBaseClass;
public:
	void safe_bool_func() {}
	typedef void (RCPtrWrapper<RCObj>:: * safe_bool_type) ();
public:
	RCPtrWrapper() : MyBaseClass(0) {};
	explicit RCPtrWrapper(RCObj * data) : MyBaseClass(data) {}
	
	template<typename RcObjSubClass, typename TDummy = typename std::enable_if<std::is_base_of<RCObj, RcObjSubClass>::value>::type >
	RCPtrWrapper(const RCPtrWrapper<RcObjSubClass, T_CAN_DISABLE_REFCOUNTING> & other) :
	MyBaseClass(other.priv())
	{}
	
	template<typename RcObjSubClass, typename TDummy = typename std::enable_if<std::is_base_of<RCObj, RcObjSubClass>::value>::type >
	RCPtrWrapper(const RCWrapper<RcObjSubClass, T_CAN_DISABLE_REFCOUNTING> & other) :
	MyBaseClass(other.priv())
	{}
	
	RCPtrWrapper(const Self & other) :
	MyBaseClass(other.priv())
	{}

	RCPtrWrapper(const RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other) :
	MyBaseClass(other.priv())
	{}
	
	virtual ~RCPtrWrapper() {}
	
	template<typename RcObjSubClass, typename TDummy = typename std::enable_if<std::is_base_of<RCObj, RcObjSubClass>::value>::type >
	RCPtrWrapper & operator=(const RCPtrWrapper<RcObjSubClass, T_CAN_DISABLE_REFCOUNTING> & other) {
		reset(other.priv());
		return *this;
	}

	RCPtrWrapper & operator=(const Self & other) {
		reset(other.priv());
		return *this;
	}

	bool operator==(const RCPtrWrapper & other) { return priv() == other.priv(); }

	RefCountObject::RCBaseType privRc() const { return (priv() ? priv()->rc() : 0);}

	RCObj & operator*() { return *priv();}
	const RCObj & operator*() const { return *priv();}

	RCObj * operator->() { return priv();}
	const RCObj * operator->() const { return priv();}

	RCObj * get() { return priv(); }
	const RCObj * get() const { return priv(); }
	
	operator safe_bool_type() const {
		return priv() ? &RCPtrWrapper<RCObj>::safe_bool_func : 0;
	}
public:
	using MyBaseClass::reset;
	using MyBaseClass::priv;
};

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>::RCWrapper(const RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other) :
MyBaseClass(other.priv())
{}

}//end namespace 

#endif
