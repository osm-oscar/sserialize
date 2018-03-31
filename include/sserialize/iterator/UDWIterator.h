#ifndef SSERIALIZE_UDW_ITERATOR_H
#define SSERIALIZE_UDW_ITERATOR_H
#include <sserialize/storage/UByteArrayAdapter.h>
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

class UDWIteratorPrivateEmpty final: public UDWIteratorPrivate {
public:
	UDWIteratorPrivateEmpty() {}
	virtual ~UDWIteratorPrivateEmpty() {}
	virtual uint32_t next() override { return 0; }
	virtual bool hasNext() override { return false; }
	virtual void reset() override {}
	virtual UDWIteratorPrivate * copy() const override { return new UDWIteratorPrivateEmpty(); }
	virtual UByteArrayAdapter::OffsetType dataSize() const override { return 0; }
};

class UDWIteratorPrivateDirect final: public UDWIteratorPrivate {
	UByteArrayAdapter m_data;
protected:
	UByteArrayAdapter & data() { return m_data; }
	const UByteArrayAdapter & data() const { return m_data; }
public:
	UDWIteratorPrivateDirect() {}
	UDWIteratorPrivateDirect(const UByteArrayAdapter & data) : m_data(data) {}
	virtual ~UDWIteratorPrivateDirect() override {}
	virtual uint32_t next() override { return m_data.getUint32(); }
	virtual bool hasNext() override { return m_data.getPtrHasNext(); }
	virtual void reset() override { m_data.resetGetPtr();}
	virtual UDWIteratorPrivate * copy() const override { return new UDWIteratorPrivateDirect(m_data); }
	virtual UByteArrayAdapter::OffsetType dataSize() const override { return m_data.size(); }
};


class UDWIteratorPrivateVarDirect final: public UDWIteratorPrivate {
	UByteArrayAdapter m_data;
protected:
	UByteArrayAdapter & data() { return m_data; }
	const UByteArrayAdapter & data() const { return m_data; }
public:
	UDWIteratorPrivateVarDirect() {}
	UDWIteratorPrivateVarDirect(const UByteArrayAdapter & data) : m_data(data) {}
	virtual ~UDWIteratorPrivateVarDirect() override {}
	virtual uint32_t next() override { return data().getVlPackedUint32(); }
	virtual bool hasNext() override { return m_data.getPtrHasNext(); }
	virtual void reset() override { m_data.resetGetPtr();}
	virtual UDWIteratorPrivate * copy() const override { return new UDWIteratorPrivateVarDirect(data()); }
	virtual UByteArrayAdapter::OffsetType dataSize() const override { return m_data.size(); }
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
