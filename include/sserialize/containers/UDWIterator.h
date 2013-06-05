#ifndef SSERIALIZE_UDW_ITERATOR_H
#define SSERIALIZE_UDW_ITERATOR_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>

namespace sserialize {

class UDWIteratorPrivate: public RefCountObject {
public:
	UDWIteratorPrivate() {}
	virtual ~UDWIteratorPrivate() {}
	virtual uint32_t next() { return 0; }
	virtual bool hasNext() { return false; }
	virtual void reset() {}
	virtual UDWIteratorPrivate * copy() { return new UDWIteratorPrivate(); }
	virtual UByteArrayAdapter::OffsetType dataSize() const { return 0; }
};

class UDWIteratorPrivateDirect: public UDWIteratorPrivate {
	UByteArrayAdapter m_data;
public:
	UDWIteratorPrivateDirect() {}
	UDWIteratorPrivateDirect(const UByteArrayAdapter & data) : m_data(data) {}
	virtual ~UDWIteratorPrivateDirect() {}
	virtual uint32_t next() { return m_data.getUint32(); }
	virtual bool hasNext() { return m_data.getPtrHasNext(); }
	virtual void reset() { return m_data.resetGetPtr();}
	virtual UDWIteratorPrivate * copy() { return new UDWIteratorPrivateDirect(m_data); }
	virtual UByteArrayAdapter::OffsetType dataSize() const { return m_data.size(); }
};

class UDWIterator: protected RCWrapper<UDWIteratorPrivate> {
	typedef RCWrapper<UDWIteratorPrivate> MyBaseClass;
public:
	///creates a direct UDW Iterator
	UDWIterator(const UByteArrayAdapter & data) : MyBaseClass(new UDWIteratorPrivateDirect(data)) {}
	UDWIterator(UDWIteratorPrivate *  priv) : MyBaseClass(priv){}
	UDWIterator(const UDWIterator & other) : MyBaseClass(other) {}
	UDWIterator() : MyBaseClass(new UDWIteratorPrivate() ) {}
	virtual ~UDWIterator() {}
	UDWIterator & operator=(const UDWIterator & other) {
		MyBaseClass::operator=(other);
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
};



}


#endif