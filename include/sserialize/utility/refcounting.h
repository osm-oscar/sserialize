#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <cstdint>
#include <assert.h>
#include <atomic>

//stolen from osmpbf
namespace sserialize {

class RefCountObject {
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

	inline int rc() const { return m_rc; }
private:
	RefCountObject(const RefCountObject & other);
	RefCountObject & operator=(const RefCountObject & other);

	std::atomic<uint32_t> m_rc;
};

template<typename RCObj>
class RCWrapper {
public:
	RCWrapper() : m_Private(0) {};
	RCWrapper(RCObj * data) : m_Private(data) { if (m_Private) m_Private->rcInc(); }
	RCWrapper(const RCWrapper & other) : m_Private(other.m_Private) { if (m_Private) m_Private->rcInc(); }
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
	
	inline int privRc() const { return m_Private->rc();}

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
class RCPtrWrapper: public RCWrapper<RCObj> {
public:
	RCPtrWrapper() : RCWrapper<RCObj>() {};
	explicit RCPtrWrapper(RCObj * data) : RCWrapper<RCObj>(data) {}
	RCPtrWrapper(const RCWrapper<RCObj> & other) : RCWrapper<RCObj>(other) {}
	virtual ~RCPtrWrapper() {}

	RCPtrWrapper & operator=(const RCPtrWrapper& other) {
		RCWrapper<RCObj>::operator=(other);
		return *this;
	}
	
	RCObj & operator*() { return *RCWrapper<RCObj>::priv();}
	const RCObj & operator*() const { return *RCWrapper<RCObj>::priv();}

	RCObj * operator->() { return RCWrapper<RCObj>::priv();}
	const RCObj * operator->() const { return RCWrapper<RCObj>::priv();}

	RCObj * priv() { return RCWrapper<RCObj>::priv(); }
	const RCObj * priv() const { return RCWrapper<RCObj>::priv(); }

};

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


}
#endif