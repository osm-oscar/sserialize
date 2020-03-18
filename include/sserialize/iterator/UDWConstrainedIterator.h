#ifndef SSERIALIZE_UDW_CONSTRAINED_ITERATOR_H
#define SSERIALIZE_UDW_CONSTRAINED_ITERATOR_H
#include <sserialize/iterator/UDWIterator.h>

namespace sserialize {
namespace detail {
namespace UDWConstrainedIterator {

template<typename T_IMPLEMENTATION>
class Base: public DelegateWrapper<T_IMPLEMENTATION> {
public:
	using Implementation = T_IMPLEMENTATION;
	using MyBaseClass = DelegateWrapper<Implementation>;
public:
	///@param priv creates a copy of priv
	Base(const Implementation * priv, uint32_t size, uint32_t offset = 0) :
	MyBaseClass( static_cast<Implementation*>(priv->copy()) ),
	m_size(size),
	m_curOffset(offset)
	{}
	///@param priv own pointer, does not create a copy of priv
	Base(Implementation * priv, uint32_t size, uint32_t offset = 0) :
	MyBaseClass(priv),
	m_size(size),
	m_curOffset(offset)
	{}
	Base(const Base & other) :
	MyBaseClass( static_cast<Implementation*>(other.priv()->copy()) ),
	m_size(other.m_size),
	m_curOffset(other.m_curOffset) {}
	virtual ~Base() {}
	inline Base & operator=(const Base & other) {
		MyBaseClass::setPrivate( static_cast<Implementation*>(other.priv()->copy()) );
		m_size = other.m_size;
		m_curOffset = other.m_curOffset;
		return *this;
	}
	inline uint32_t next() {
		++m_curOffset;
		return MyBaseClass::priv()->next();
	}
	inline bool hasNext() {
		return m_curOffset < m_size;
	}
	inline uint32_t size() const {
		return m_size;
	}
	inline UByteArrayAdapter::OffsetType dataSize() const {
		return MyBaseClass::priv()->dataSize();
	}
	inline void reset() {
		MyBaseClass::priv()->reset();
		m_curOffset = 0;
	}
protected:
	Base() : MyBaseClass(0), m_size(0), m_curOffset(0) {}
	uint32_t offset() const { return m_curOffset; }
private:
	uint32_t m_size;
	uint32_t m_curOffset;
};
	
}} //end namespace detail::UDWConstrainedIterator
	
class UDWConstrainedIteratorDirectOnly;
	
class UDWConstrainedIterator: public detail::UDWConstrainedIterator::Base<UDWIteratorPrivate> {
	friend class UDWConstrainedIteratorDirectOnly;
	using MyBaseClass = detail::UDWConstrainedIterator::Base<UDWIteratorPrivate>;
public:
	///creates a direct UDW Iterator
	UDWConstrainedIterator(const UByteArrayAdapter & data, uint32_t size) :
	MyBaseClass(new UDWIteratorPrivateDirect(data), size)
	{}
	///@param priv own pointer, does not create a copy of priv
	UDWConstrainedIterator(const UDWIteratorPrivate *  priv, uint32_t size) :
	MyBaseClass((priv ? priv->copy() : new UDWIteratorPrivateEmpty()), size)
	{}
	UDWConstrainedIterator(const UDWConstrainedIterator & other) :
	MyBaseClass(other)
	{}
	UDWConstrainedIterator() :
	MyBaseClass(new UDWIteratorPrivateEmpty(), 0)
	{}
	virtual ~UDWConstrainedIterator() {}
	UDWConstrainedIterator & operator=(const UDWConstrainedIterator & other) {
		MyBaseClass::operator=(other);
		return *this;
	}
};

class UDWConstrainedIteratorDirectOnly: public detail::UDWConstrainedIterator::Base<UDWIteratorPrivateDirect> {
public:
	using MyBaseClass = detail::UDWConstrainedIterator::Base<UDWIteratorPrivateDirect>;
public:
	///creates a direct UDW Iterator
	UDWConstrainedIteratorDirectOnly(const UByteArrayAdapter & data, uint32_t size) :
	MyBaseClass(new UDWIteratorPrivateDirect(data), size)
	{}
	UDWConstrainedIteratorDirectOnly(const UDWConstrainedIteratorDirectOnly & other) :
	MyBaseClass(other)
	{}
	UDWConstrainedIteratorDirectOnly(const UDWConstrainedIterator & other) : 
	MyBaseClass(try_copy_private(other.priv()), other.size(), other.offset())
	{}
	UDWConstrainedIteratorDirectOnly() :
	MyBaseClass(new UDWIteratorPrivateDirect(), 0)
	{}
	virtual ~UDWConstrainedIteratorDirectOnly() {}
	UDWConstrainedIteratorDirectOnly & operator=(const UDWConstrainedIteratorDirectOnly & other) {
		MyBaseClass::operator=(other);
		return *this;
	}
private:
	static UDWIteratorPrivateDirect * try_copy_private(const UDWIteratorPrivate * other) {
		if (dynamic_cast<const UDWIteratorPrivateDirect *>(other)) {
			return (UDWIteratorPrivateDirect*) other->copy();
		}
		throw sserialize::InvalidReferenceException("UDWConstrainedIteratorDirectOnly invalid assignment from UDWConstrainedIterator");
		return new UDWIteratorPrivateDirect();
	}
};

}//end namespace

#endif
