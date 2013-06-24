#ifndef SSERIALIZE_UDW_ITERATOR_H
#define SSERIALIZE_UDW_ITERATOR_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/delegate.h>

namespace sserialize {

class UDWIteratorPrivate {
public:
	UDWIteratorPrivate() {}
	virtual ~UDWIteratorPrivate() {}
	virtual uint32_t next() = 0;
	virtual bool hasNext() = 0;
	virtual void reset() = 0;
	virtual UDWIteratorPrivate * copy() const = 0;
	virtual UByteArrayAdapter::OffsetType dataSize() const = 0;
};

class UDWIteratorPrivateEmpty: public UDWIteratorPrivate {
public:
	UDWIteratorPrivateEmpty() {}
	virtual ~UDWIteratorPrivateEmpty() {}
	virtual uint32_t next() { return 0; }
	virtual bool hasNext() { return false; }
	virtual void reset() {}
	virtual UDWIteratorPrivate * copy() const { return new UDWIteratorPrivateEmpty(); }
	virtual UByteArrayAdapter::OffsetType dataSize() const { return 0; }
};

class UDWIteratorPrivateDirect: public UDWIteratorPrivate {
	UByteArrayAdapter m_data;
protected:
	UByteArrayAdapter & data() { return m_data; }
	const UByteArrayAdapter & data() const { return m_data; }
public:
	UDWIteratorPrivateDirect() {}
	UDWIteratorPrivateDirect(const UByteArrayAdapter & data) : m_data(data) {}
	virtual ~UDWIteratorPrivateDirect() {}
	virtual uint32_t next() { return m_data.getUint32(); }
	virtual bool hasNext() { return m_data.getPtrHasNext(); }
	virtual void reset() { return m_data.resetGetPtr();}
	virtual UDWIteratorPrivate * copy() const { return new UDWIteratorPrivateDirect(m_data); }
	virtual UByteArrayAdapter::OffsetType dataSize() const { return m_data.size(); }
};


class UDWIteratorPrivateVarDirect: public UDWIteratorPrivateDirect {
public:
	UDWIteratorPrivateVarDirect() {}
	UDWIteratorPrivateVarDirect(const UByteArrayAdapter & data) : UDWIteratorPrivateDirect(data) {}
	virtual ~UDWIteratorPrivateVarDirect() {}
	virtual uint32_t next() { return data().getVlPackedUint32(); }
	virtual UDWIteratorPrivate * copy() const { return new UDWIteratorPrivateVarDirect(data()); }
};

class UDWIterator: protected DelegateWrapper<UDWIteratorPrivate> {
	typedef DelegateWrapper<UDWIteratorPrivate> MyBaseClass;
public:
	///creates a direct UDW Iterator
	UDWIterator(const UByteArrayAdapter & data) : MyBaseClass(new UDWIteratorPrivateDirect(data)) {}
	UDWIterator(UDWIteratorPrivate *  priv) : MyBaseClass(priv->copy()){}
	UDWIterator(const UDWIterator & other) : MyBaseClass(other.getPrivate()->copy()) {}
	UDWIterator() : MyBaseClass(new UDWIteratorPrivateEmpty() ) {}
	virtual ~UDWIterator() {}
	UDWIterator & operator=(const UDWIterator & other) {
		setPrivate(other.getPrivate()->copy());
		return *this;
	}
	uint32_t next() { return priv()->next(); }
	bool hasNext() { return priv()->hasNext(); }
	UByteArrayAdapter::OffsetType dataSize() const {
		return priv()->dataSize();
	}
	void reset() {
		priv()->reset();
	}
	const UDWIteratorPrivate * getPrivate() const { return priv(); }
	UDWIteratorPrivate * getPrivate() { return priv(); }
};

}


#endif