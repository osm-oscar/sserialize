#ifndef SSERIALIZE_ABSTRACT_ARRAY_H
#define SSERIALIZE_ABSTRACT_ARRAY_H
#include <vector>
#include <memory>

namespace sserialize {

namespace detail {

template<typename TReturnType>
class AbstractArrayIterator {
public:
	AbstractArrayIterator() {}
	virtual ~AbstractArrayIterator() {}
	virtual TReturnType get() const = 0;
	virtual void next() = 0;
	virtual bool notEq(const AbstractArrayIterator * other) const = 0;
	virtual AbstractArrayIterator * copy() const = 0;
};

template<typename TReturnType>
class AbstractArray {
public:
	typedef AbstractArrayIterator<TReturnType> * const_iterator;
public:
	AbstractArray() {}
	virtual ~AbstractArray() {}
	virtual uint32_t size() const = 0;
	virtual TReturnType at(uint32_t pos) const = 0;
	///function should return a pointer to a AbstractArrayIterator which is then owned by a shared_ptr
	virtual const_iterator cbegin() const = 0;
	virtual const_iterator cend() const = 0;
};

template<typename TMyIteratorType, typename TReturnType>
class AbstractArrayIteratorDefaultImp: public AbstractArrayIterator<TReturnType> {
	TMyIteratorType m_it;
public:
	AbstractArrayIteratorDefaultImp(const TMyIteratorType & it) : m_it(it) {}
	AbstractArrayIteratorDefaultImp(const AbstractArrayIteratorDefaultImp & other) : m_it(other.m_it) {}
	virtual ~AbstractArrayIteratorDefaultImp() {}
	virtual TReturnType get() const { return *m_it;}
	virtual void next() { ++m_it;}
	virtual bool notEq(const AbstractArrayIterator<TReturnType> * other) const {
		const AbstractArrayIteratorDefaultImp * oIt = dynamic_cast<const AbstractArrayIteratorDefaultImp* >(other);
		return !oIt || oIt->m_it != m_it;
	}
	virtual AbstractArrayIterator<TReturnType> * copy() const {
		return new AbstractArrayIteratorDefaultImp(*this);
	}
};

template<typename TContainerType, typename TReturnType, typename TAbstractArrayIteratorType = AbstractArrayIteratorDefaultImp<typename TContainerType::const_iterator, TReturnType> >
class AbstractArrayDefaultImp: public AbstractArray<TReturnType> {
public:
	typedef AbstractArray<TReturnType> MyBaseClass;
	typedef TAbstractArrayIteratorType const_iterator;
private:
	TContainerType m_data;
public:
	AbstractArrayDefaultImp() {}
	AbstractArrayDefaultImp(const TContainerType & d) : m_data(d) {}
	virtual ~AbstractArrayDefaultImp() {}
	virtual uint32_t size() const { return m_data.size(); }
	virtual TReturnType at(uint32_t pos) const { return m_data.at(pos); }
	///function should return a pointer to a AbstractArrayIterator which is then owned by a shared_ptr
	virtual typename MyBaseClass::const_iterator cbegin() const { return new const_iterator(m_data.cbegin());}
	virtual typename MyBaseClass::const_iterator cend() const { return new const_iterator(m_data.cend());}
};

}//end namespace detail

template<typename TReturnType>
class AbstractArrayIterator {
	std::unique_ptr< detail::AbstractArrayIterator<TReturnType> > m_priv;
public:
	AbstractArrayIterator(detail::AbstractArrayIterator<TReturnType> * it) : m_priv(it) {}
	AbstractArrayIterator(const AbstractArrayIterator & other) : m_priv((other.m_priv.get() ? other.m_priv->copy() : 0)) {}
	virtual ~AbstractArrayIterator() {}
	
	AbstractArrayIterator & operator=(const AbstractArrayIterator & other) {
		m_priv.reset((other.m_priv.get() ? other.m_priv->copy() : 0));
		return *this;
	}
	
	TReturnType operator*() const {
		return m_priv->get();
	}
	
	AbstractArrayIterator & operator++() {
		m_priv->next();
		return *this;
	}
	
	AbstractArrayIterator operator++(int) {
		AbstractArrayIterator t(*this);
		++t;
		return t;
	}
	
	AbstractArrayIterator operator+(uint32_t amount) {
		AbstractArrayIterator t(*this);
		while (amount) {
			m_priv->next();
			--amount;
		}
		return t;
	}
	
	bool operator!=(const AbstractArrayIterator & other) {
		return m_priv->notEq(other.m_priv.get());
	}
};

template<typename TReturnType>
class AbstractArray {
public:
	typedef AbstractArrayIterator<TReturnType> const_iterator;
	typedef const_iterator iterator;
	typedef TReturnType const_reference;
	typedef std::shared_ptr< detail::AbstractArray<TReturnType> > ImpRefType;
private:
	ImpRefType m_priv;
public:
	AbstractArray() {}
	AbstractArray(detail::AbstractArray<TReturnType> * ptr) : m_priv(ptr) {}
	AbstractArray(const ImpRefType & d) : m_priv(d) {}
	virtual ~AbstractArray() {}
	const ImpRefType & priv() const { return m_priv; }
	template<typename TAbstractArrayType>
	TAbstractArrayType * get() const { return static_cast<TAbstractArrayType*>(m_priv.get()); }
	uint32_t size() const { return m_priv->size();}
	TReturnType at(uint32_t pos) const { return m_priv->at(pos); }
	TReturnType front() const { return m_priv->at(0); }
	TReturnType back() const { return m_priv->at(size()-1);}
	const_iterator cbegin() const { return const_iterator(m_priv->cbegin()); }
	const_iterator cend() const { return const_iterator(m_priv->cend()); }
};

}//end namespace


#endif