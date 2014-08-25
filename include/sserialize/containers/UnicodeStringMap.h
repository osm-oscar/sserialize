#ifndef SSERIALIZE_UNICODE_STRING_MAP_H
#define SSERIALIZE_UNICODE_STRING_MAP_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace detail {

template<typename TValue>
class UnicodeStringMap: public RefCountObject {
public:
	UnicodeStringMap() {}
	virtual ~UnicodeStringMap() {}
	virtual TValue at(const std::string & str, bool prefixMatch) const = 0;
	virtual bool count(const std::string & str, bool prefixMatch) const = 0;
};

template<typename TValue>
class UnicodeStringMapEmpty: public UnicodeStringMap<TValue> {
public:
	UnicodeStringMapEmpty() {}
	virtual ~UnicodeStringMapEmpty() {}
	virtual TValue at(const std::string & /*str*/, bool /*prefixMatch*/) const override {
		throw sserialize::OutOfBoundsException("sserialize::detai::UnicodeStringMapEmpty");
		return TValue();
	}
	virtual bool count(const std::string & /*str*/, bool /*prefixMatch*/) const override {
		return false;
	}
};

}//end namespace detail

template<typename TValue>
class UnicodeStringMap {
public:
	typedef TValue value_type;
	typedef RCPtrWrapper< detail::UnicodeStringMap<TValue> > PrivPtrType;
private:
	PrivPtrType m_priv;
protected:
	PrivPtrType & priv() { return m_priv; }
	const PrivPtrType & priv() const { return m_priv; }
public:
	UnicodeStringMap() : m_priv(new detail::UnicodeStringMapEmpty<TValue>()) {}
	UnicodeStringMap(const PrivPtrType & priv) : m_priv(priv) {}
	virtual ~UnicodeStringMap() {}
	TValue at(const std::string & str, bool prefixMatch) const { return priv()->at(str, prefixMatch); }
	bool count(const std::string & str, bool prefixMatch) const { return priv()->count(str, prefixMatch); }
};

}//end namespace


#endif