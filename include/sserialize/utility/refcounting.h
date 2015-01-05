#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <cstdint>
#include <assert.h>
#include <atomic>
#include <utility>

namespace sserialize {


class RefCountObject {
public:
	typedef uint32_t RCBaseType;
public:
	RefCountObject() : m_rc(0) {}
	virtual ~RefCountObject() {}

	inline void rcReset() { m_rc = 0; }
	inline void rcInc() { ++m_rc; }
	inline void rcDec() {
		assert(m_rc);
		if (m_rc.fetch_sub(1) == 1) //check if we are the last
			delete this;
	}

	inline RCBaseType rc() const { return m_rc; }
private:
	RefCountObject(const RefCountObject & other);
	RefCountObject & operator=(const RefCountObject & other);

	std::atomic<RCBaseType> m_rc;
};

template<typename RCObj>
class RCPtrWrapper;

template<typename RCObj>
class RCWrapper {
public:
	typedef RCObj element_type;
	friend class RCPtrWrapper<RCObj>;
public:
	RCWrapper() : m_Private(0) {};
	RCWrapper(RCObj * data) : m_Private(data) { if (m_Private) m_Private->rcInc(); }
	RCWrapper(const RCWrapper & other) : m_Private(other.m_Private) { if (m_Private) m_Private->rcInc(); }
	RCWrapper(const RCPtrWrapper<RCObj> & other);
	virtual ~RCWrapper() {
		if (m_Private)
			m_Private->rcDec();
	}

	RCWrapper & operator=(const RCWrapper & other) {
		if (other.m_Private)
			other.m_Private->rcInc();
		if (m_Private)
			m_Private->rcDec();
		m_Private = other.m_Private;
		return *this;
	}
	
	bool operator==(const RCWrapper & other) { return m_Private == other.m_Private; }
	
	inline RefCountObject::RCBaseType privRc() const { return (m_Private ? m_Private->rc() : 0);}

protected:
	void setPrivate(RCObj * data) {
		if (data)
			data->rcInc();
		if (m_Private)
			m_Private->rcDec();
		m_Private = data;
	}

	inline RCObj * priv() const { return m_Private; }
	
private:
	RCObj * m_Private;
};

template<typename RCObj>
class RCPtrWrapper final {
public:
	typedef RCObj element_type;
	friend class RCWrapper<RCObj>;
private:
	void safe_bool_func() {}
	typedef void (RCPtrWrapper<RCObj>:: * safe_bool_type) ();
private:
	RCObj * m_priv;
public:
	RCPtrWrapper() : m_priv(0) {};
	explicit RCPtrWrapper(RCObj * data) : m_priv(data) {if (m_priv) m_priv->rcInc();}
	RCPtrWrapper(const RCPtrWrapper<RCObj> & other) : m_priv(other.m_priv) { if (m_priv) m_priv->rcInc(); }
	RCPtrWrapper(const RCWrapper<RCObj> & other) : m_priv(other.priv()) { if (m_priv) m_priv->rcInc(); }
	~RCPtrWrapper() {
		if (m_priv) {
			m_priv->rcDec();
		}
	}

	RCPtrWrapper & operator=(const RCPtrWrapper& other) {
		if (other.m_priv)
			other.m_priv->rcInc();
		if (m_priv)
			m_priv->rcDec();
		m_priv = other.m_priv;
		return *this;
	}

	bool operator==(const RCPtrWrapper & other) { return m_priv == other.m_priv; }
	
	inline RefCountObject::RCBaseType privRc() const { return (m_priv ? m_priv->rc() : 0);}

	
	RCObj & operator*() { return *priv();}
	const RCObj & operator*() const { return *priv();}

	RCObj * operator->() { return priv();}
	const RCObj * operator->() const { return priv();}

	RCObj * priv() { return m_priv; }
	const RCObj * priv() const { return m_priv; }
	
	RCObj * get() { return priv(); }
	const RCObj * get() const { return priv(); }
	
	operator safe_bool_type() const {
		return priv() ? &RCPtrWrapper<RCObj>::safe_bool_func : 0;
	}
	void reset(RCObj * data) {
		if (data)
			data->rcInc();
		if (m_priv)
			m_priv->rcDec();
		m_priv = data;
	}
};

template<typename T, typename... Args>
RCPtrWrapper<T> make_rcptrwp(Args&&... args) {
	return RCPtrWrapper<T>( new T(std::forward<Args>(args)...));
}

template<typename TBase, typename TDerived, typename... Args>
RCPtrWrapper<TBase> make_rcptrwpBD(Args&&... args) {
	return RCPtrWrapper<TBase>( new TDerived(std::forward<Args>(args)...));
}

template<class RCObj>
class RefObjRCWrapper {
public:
	RefObjRCWrapper() : m_Private(0), m_rc(0) {};
	RefObjRCWrapper(RCObj * data) : m_Private(data), m_rc(0) { if (m_Private) m_Private->rcInc(); }
	RefObjRCWrapper(const RefObjRCWrapper & other) : m_Private(other.m_Private), m_rc(0) { if (m_Private) m_Private->rcInc(); }
	virtual ~RefObjRCWrapper() { if (m_Private) m_Private->rcDec(); }

	/** This operator just changes the storage pointed to by this object */
	RefObjRCWrapper & operator=(const RefObjRCWrapper & other) {
		if (m_Private)
			m_Private->rcDec();
		m_Private = other.m_Private;
		if (m_Private)
			m_Private->rcInc();
		return *this;
	}

	bool operator==(const RefObjRCWrapper & other) { return m_Private == other.m_Private; }

	inline void rcInc() { m_rc++; }
	inline void rcDec() {
		assert(m_rc);
		m_rc--;
		if (m_rc < 1) { //destructor will decrease privRc
			delete this;
		}
	}

	inline int rc() const { return m_rc; }

	inline int privRc() const { return m_Private->rc();}
	
protected:
	RCObj * priv() { return m_Private; }
	
private:
	RCObj * m_Private;
	int m_rc;
};

template<typename RCObj>
RCWrapper<RCObj>::RCWrapper(const RCPtrWrapper<RCObj> & other) : m_Private(other.m_priv) {
	if(m_Private)
		m_Private->rcInc();
}

}//end namespace 

#endif