#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <sserialize/utility/assert.h>
#include <cstdint>
#include <atomic>
#include <utility>

namespace sserialize {
namespace detail {
	template<typename RCObj, typename TEnable = void>
	class RCBase;
}

class RefCountObjectWithDisable;

template<typename RCObj>
class RCPtrWrapper;

template<typename RCObj>
class RCWrapper;

template<class RCObj>
class RefObjRCWrapper;

class RefCountObject {
	friend class RefCountObjectWithDisable;
	template<typename RCObj, typename TEnable> friend class detail::RCBase;
public:
	typedef uint32_t RCBaseType;
public:
	RefCountObject(const RefCountObject & other) = delete;
	RefCountObject & operator=(const RefCountObject & other) = delete;
	RefCountObject() : m_rc(0) {}
	virtual ~RefCountObject() {}

	inline void rcReset() { m_rc = 0; }
	inline RCBaseType rc() const { return m_rc; }

private:
	inline void rcInc() { m_rc.fetch_add(1, std::memory_order_relaxed); }
	inline void rcDec() {
		SSERIALIZE_CHEAP_ASSERT(rc() > 0);
		if (m_rc.fetch_sub(1, std:: memory_order_acq_rel) == 1) { //check if we are the last
			delete this;
		}
	}
private:
	std::atomic<RCBaseType> m_rc;
};

class RefCountObjectWithDisable: public sserialize::RefCountObject {
	template<typename RCObj, typename TEnable> friend class detail::RCBase;
public:
	RefCountObjectWithDisable(const RefCountObjectWithDisable & other) = delete;
	RefCountObjectWithDisable & operator=(const RefCountObjectWithDisable & other) = delete;
	RefCountObjectWithDisable() : m_enabled(true) {}
	virtual ~RefCountObjectWithDisable() {}

	///@WARNING this is a dangerous thing to do. You have to make sure that at least one owner is alive during usage of data
	inline bool disableRc() {
		m_enabled = false;
		return true;
	}
	inline void enableRc() {
		m_enabled = true;
	}
	
	inline bool enabledRC() const {
		return m_enabled;
	}
private:
	using RefCountObject::rcInc;
	using RefCountObject::rcDec;
private:
	bool m_enabled;
};

namespace detail {

template<typename RCObj>
class RCBase<RCObj, typename std::enable_if< std::is_base_of<RefCountObjectWithDisable, RCObj>::value, void >::type > {
protected:
	RCBase(RCObj* p) : m_priv(0), m_enabled(false)
	{
		reset(p);
	}
	virtual ~RCBase() {
		reset(0);
	}
protected:
	bool enabledRC() {
		return m_enabled;
	}
	void reset(RCObj* p) {
		if (p && p->enabledRC()) {
			p->rcInc();
		}
		rcDec();
		m_priv = p;
		m_enabled = p ? p->enabledRC() : false;
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
	const RCObj * priv() const { return m_priv; }
	RCObj * priv() { return m_priv; }
private:
	RCObj * m_priv;
	bool m_enabled;
};

template<typename RCObj>
class RCBase<RCObj, typename std::enable_if<! std::is_base_of<RefCountObjectWithDisable, RCObj>::value, void >::type > {
protected:
	RCBase(RCObj* p) : m_priv(0) {
		reset(p);
	}
	virtual ~RCBase() {
		reset(0);
	}
protected:
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

template<typename RCObj>
class RCWrapper: private detail::RCBase<RCObj> {
public:
	typedef RCObj element_type;
	friend class RCPtrWrapper<RCObj>;
private:
	typedef detail::RCBase<RCObj> MyBaseClass;
public:
	RCWrapper() : MyBaseClass(0) {};
	RCWrapper(RCObj * data) : MyBaseClass(data) {}
	RCWrapper(const RCWrapper & other) : MyBaseClass(other.priv()) {}
	RCWrapper(const RCPtrWrapper<RCObj> & other);
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

template<typename RCObj>
class RCPtrWrapper final : private detail::RCBase<RCObj> {
public:
	typedef RCObj element_type;
	friend class RCWrapper<RCObj>;
private:
	typedef detail::RCBase<RCObj> MyBaseClass;
private:
	void safe_bool_func() {}
	typedef void (RCPtrWrapper<RCObj>:: * safe_bool_type) ();
public:
	RCPtrWrapper() : MyBaseClass(0) {};
	explicit RCPtrWrapper(RCObj * data) : MyBaseClass(data) {}
	RCPtrWrapper(const RCPtrWrapper<RCObj> & other) : MyBaseClass(other.priv()) {}
	RCPtrWrapper(const RCWrapper<RCObj> & other) : MyBaseClass(other.priv()) {}
	~RCPtrWrapper() {}

	RCPtrWrapper & operator=(const RCPtrWrapper& other) {
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

template<typename T, typename... Args>
RCPtrWrapper<T> make_rcptrwp(Args&&... args) {
	return RCPtrWrapper<T>( new T(std::forward<Args>(args)...));
}

template<typename TBase, typename TDerived, typename... Args>
RCPtrWrapper<TBase> make_rcptrwpBD(Args&&... args) {
	return RCPtrWrapper<TBase>( new TDerived(std::forward<Args>(args)...));
}

// template<class RCObj>
// class RefObjRCWrapper {
// public:
// 	RefObjRCWrapper() : m_Private(0), m_rc(0) {};
// 	RefObjRCWrapper(RCObj * data) : m_Private(data), m_rc(0) { if (m_Private) m_Private->rcInc(); }
// 	RefObjRCWrapper(const RefObjRCWrapper & other) : m_Private(other.m_Private), m_rc(0) { if (m_Private) m_Private->rcInc(); }
// 	virtual ~RefObjRCWrapper() { if (m_Private) m_Private->rcDec(); }
// 
// 	/** This operator just changes the storage pointed to by this object */
// 	RefObjRCWrapper & operator=(const RefObjRCWrapper & other) {
// 		if (m_Private)
// 			m_Private->rcDec();
// 		m_Private = other.m_Private;
// 		if (m_Private)
// 			m_Private->rcInc();
// 		return *this;
// 	}
// 
// 	bool operator==(const RefObjRCWrapper & other) { return m_Private == other.m_Private; }
// 
// 	inline void rcInc() { m_rc++; }
// 	inline void rcDec() {
// 		SSERIALIZE_CHEAP_ASSERT(m_rc);
// 		m_rc--;
// 		if (m_rc < 1) { //destructor will decrease privRc
// 			delete this;
// 		}
// 	}
// 
// 	inline int rc() const { return m_rc; }
// 
// 	inline int privRc() const { return m_Private->rc();}
// 	
// protected:
// 	RCObj * priv() { return m_Private; }
// 	
// private:
// 	RCObj * m_Private;
// 	int m_rc;
// };

template<typename RCObj>
RCWrapper<RCObj>::RCWrapper(const RCPtrWrapper<RCObj> & other) :
MyBaseClass(other.priv())
{}

}//end namespace 

#endif