#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <cstdint>
#include <assert.h>
#include <memory>

namespace sserialize {

template<typename RCObj>
class RCWrapper {
public:
	RCWrapper() {};
	///@param data shall NOT be owned by another instance of RCWrapper
	RCWrapper(RCObj * data)  __attribute__((warning("RCWrapper possibly initialized with undefined owner structure")));
	explicit RCWrapper(const std::shared_ptr<RCObj> & data) : m_Private(data) {}
	RCWrapper(const RCWrapper & other) : m_Private(other.m_Private) {}
	virtual ~RCWrapper() {}

	RCWrapper & operator=(const RCWrapper & other) {
		m_Private = other.m_Private;
		return *this;
	}
	
	bool operator==(const RCWrapper & other) { return m_Private.get() == other.m_Private.get(); }
	
	inline int privRc() const { return m_Private.use_count();}

protected:
	inline void setPrivate(RCObj * data) {
		m_Private.reset(data);
	}

	inline RCObj * priv() const { return m_Private.get(); }
	
private:
	std::shared_ptr<RCObj> m_Private;
};

template<typename RCObj>
RCWrapper<RCObj>::RCWrapper(RCObj * data) : m_Private(data) {}

}
#endif