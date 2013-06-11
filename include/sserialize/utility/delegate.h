#ifndef SSERIALIZE_DELEGATE_H
#define SSERIALIZE_DELEGATE_H
#include <algorithm>

namespace sserialize {

template<typename DelegateObj>
class DelegateWrapper {
public:
	DelegateWrapper() : m_Private(0) {};
	DelegateWrapper(DelegateObj * data) : m_Private(data) {}
	DelegateWrapper(const DelegateWrapper & other) : m_Private(other.m_Private) {}
	virtual ~DelegateWrapper() {
		delete m_Private;
	}

	DelegateWrapper & operator=(const DelegateWrapper & other) {
		delete m_Private;
		m_Private = other.m_Private;
		return *this;
	}
	
	bool operator==(const DelegateWrapper & other) { return m_Private == other.m_Private; }
	
	void swap(DelegateWrapper & other) {
		std::swap(other.m_Private, m_Private);
	}

protected:
	void setPrivate(DelegateObj * data) {
		delete m_Private;
		m_Private = data;
	}

	inline DelegateObj * priv() const { return m_Private; }
	
private:
		DelegateObj * m_Private;
};

}

namespace std {
	template<typename _T>
	void swap(sserialize::DelegateWrapper<_T> & a, sserialize::DelegateWrapper<_T> & b) {
		a.swap(b);
	}
}

#endif