#ifndef SSERIALIZE_UDW_CONSTRAINED_ITERATOR_H
#define SSERIALIZE_UDW_CONSTRAINED_ITERATOR_H
#include <sserialize/containers/UDWIterator.h>

namespace sserialize {

class UDWConstrainedIterator: public DelegateWrapper<UDWIteratorPrivate> {
	typedef DelegateWrapper<UDWIteratorPrivate> MyBaseClass;
private:
	uint32_t m_size;
	uint32_t m_curOffset;
public:
	///creates a direct UDW Iterator
	UDWConstrainedIterator(const UByteArrayAdapter & data, uint32_t size)
	: MyBaseClass(new UDWIteratorPrivateDirect(data)), m_size(size), m_curOffset(0) {}
	///@param priv own pointer, does not create a copy of priv
	UDWConstrainedIterator(const UDWIteratorPrivate *  priv, uint32_t size) :
	MyBaseClass((priv ? priv->copy() : new UDWIteratorPrivateEmpty())), m_size(size), m_curOffset(0) {}
	UDWConstrainedIterator(const UDWConstrainedIterator & other) : MyBaseClass(other.priv()->copy()), m_size(other.m_size), m_curOffset(other.m_curOffset) {}
	UDWConstrainedIterator() : MyBaseClass(new UDWIteratorPrivateEmpty() ), m_size(0), m_curOffset(0) {}
	virtual ~UDWConstrainedIterator() {}
	UDWConstrainedIterator & operator=(const UDWConstrainedIterator & other) {
		setPrivate(other.priv()->copy());
		m_size = other.m_size;
		m_curOffset = other.m_curOffset;
		return *this;
	}
	inline uint32_t next() {
		++m_curOffset;
		return priv()->next();
	}
	inline bool hasNext() {
		return m_curOffset < m_size;
	}
	UByteArrayAdapter::OffsetType dataSize() const {
		return priv()->dataSize();
	}
	void reset() {
		priv()->reset();
		m_curOffset = 0;
	}
};

}//end namespace

#endif