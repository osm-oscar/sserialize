#ifndef SSERIALIZE_ABSTRACT_ARRAY_H
#define SSERIALIZE_ABSTRACT_ARRAY_H
#include <sserialize/utility/types.h>
#include <vector>
#include <memory>

namespace sserialize {

namespace detail {

///The base class of the AbstractArrayIterator implementation part
template<typename TReturnType>
class AbstractArrayIterator {
public:
	using value_type = TReturnType;
	using size_type = sserialize::SizeType;
public:
	AbstractArrayIterator() {}
	virtual ~AbstractArrayIterator() {}
	virtual value_type get() const = 0;
	virtual void next() = 0;
	virtual void ffwd(size_type amount) {
		while (amount > 0) {
			next();
			--amount;
		}
	}
	virtual bool notEq(const AbstractArrayIterator * other) const = 0;
	virtual bool eq(const AbstractArrayIterator * other) const = 0;
	virtual AbstractArrayIterator * copy() const = 0;
};

template<typename TReturnType>
class AbstractArray {
public:
	typedef AbstractArrayIterator<TReturnType> * const_iterator;
	typedef sserialize::SizeType SizeType;
public:
	AbstractArray() {}
	virtual ~AbstractArray() {}
	virtual SizeType size() const = 0;
	virtual TReturnType at(SizeType pos) const = 0;
	///function should return a pointer to a AbstractArrayIterator which is then owned by a shared_ptr
	virtual const_iterator cbegin() const = 0;
	virtual const_iterator cend() const = 0;
};

template<typename TMyIteratorType, typename TReturnType = typename std::iterator_traits<TMyIteratorType>::value_type>
class AbstractArrayIteratorDefaultImp: public AbstractArrayIterator<TReturnType> {
	TMyIteratorType m_it;
public:
	AbstractArrayIteratorDefaultImp(const TMyIteratorType & it) : m_it(it) {}
	AbstractArrayIteratorDefaultImp(const AbstractArrayIteratorDefaultImp & other) : m_it(other.m_it) {}
	virtual ~AbstractArrayIteratorDefaultImp() {}
	virtual TReturnType get() const override { return *m_it;}
	virtual void next() override { ++m_it;}
	virtual bool notEq(const AbstractArrayIterator<TReturnType> * other) const override {
		const AbstractArrayIteratorDefaultImp * oIt = dynamic_cast<const AbstractArrayIteratorDefaultImp* >(other);
		return !oIt || oIt->m_it != m_it;
	}
	virtual bool eq(const AbstractArrayIterator<TReturnType> * other) const override {
		const AbstractArrayIteratorDefaultImp * oIt = dynamic_cast<const AbstractArrayIteratorDefaultImp* >(other);
		return oIt && !(oIt->m_it != m_it);
	}
	
	virtual AbstractArrayIterator<TReturnType> * copy() const override {
		return new AbstractArrayIteratorDefaultImp(*this);
	}
};

template<typename TContainerType, typename TReturnType, typename TAbstractArrayIteratorType = AbstractArrayIteratorDefaultImp<typename TContainerType::const_iterator, TReturnType> >
class AbstractArrayDefaultImp: public AbstractArray<TReturnType> {
public:
	typedef AbstractArray<TReturnType> MyBaseClass;
	typedef TAbstractArrayIteratorType const_iterator;
	typedef TContainerType ContainerType;
private:
	TContainerType m_data;
public:
	AbstractArrayDefaultImp() {}
	AbstractArrayDefaultImp(const TContainerType & d) : m_data(d) {}
	virtual ~AbstractArrayDefaultImp() {}
	const TContainerType & container() const { return m_data; }
	virtual SizeType size() const { return m_data.size(); }
	virtual TReturnType at(SizeType pos) const { return m_data.at(pos); }
	///function should return a pointer to a AbstractArrayIterator which is then owned by a shared_ptr
	virtual typename MyBaseClass::const_iterator cbegin() const { return new const_iterator(m_data.cbegin());}
	virtual typename MyBaseClass::const_iterator cend() const { return new const_iterator(m_data.cend());}
};

}//end namespace detail

template<typename TReturnType>
class AbstractArrayIterator: public std::iterator<std::input_iterator_tag, typename std::remove_reference<TReturnType>::type, typename detail::AbstractArrayIterator<TReturnType>::size_type > {
public:
	typedef typename detail::AbstractArrayIterator<TReturnType>::size_type size_type;
private:
	std::unique_ptr< detail::AbstractArrayIterator<TReturnType> > m_priv;
public:
	AbstractArrayIterator() {}
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
	
	AbstractArrayIterator operator+=(size_type amount) {
		m_priv->ffwd(amount);
		return *this;
	}
	
	AbstractArrayIterator operator+(size_type amount) {
		AbstractArrayIterator t(*this);
		t.m_priv->ffwd(amount);
		return t;
	}
	
	bool operator!=(const AbstractArrayIterator & other) const {
		return m_priv->notEq(other.m_priv.get());
	}
	
	bool operator==(const AbstractArrayIterator & other) const {
		return m_priv->eq(other.m_priv.get());
	}
	
	template<typename T_ITERATOR>
	static AbstractArrayIterator fromIterator(T_ITERATOR it) {
		return AbstractArrayIterator( new detail::AbstractArrayIteratorDefaultImp<T_ITERATOR, TReturnType>(it) );
	}
};

template<typename TReturnType, typename TReference = TReturnType, typename TConstReference = TReturnType>
class AbstractArray {
public:
	typedef AbstractArrayIterator<TReturnType> const_iterator;
	typedef const_iterator iterator;
	typedef TReference reference;
	typedef TConstReference const_reference;
	typedef std::shared_ptr< detail::AbstractArray<TReturnType> > ImpRefType;
	typedef typename detail::AbstractArray<TReturnType>::SizeType SizeType;
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
	SizeType size() const { return m_priv->size();}
	const_reference at(SizeType pos) const { return m_priv->at(pos); }
	const_reference front() const { return m_priv->at(0); }
	const_reference back() const { return m_priv->at(size()-1);}
	const_iterator cbegin() const { return const_iterator(m_priv->cbegin()); }
	const_iterator cend() const { return const_iterator(m_priv->cend()); }
};

}//end namespace


#endif
