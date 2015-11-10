#ifndef SSERIALIZE_OOM_ARRAY_H
#define SSERIALIZE_OOM_ARRAY_H
#include <sserialize/storage/FileHandler.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/MmappedMemory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <type_traits>

#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

namespace sserialize {

template<typename TValue, typename TEnable = typename std::enable_if< IS_TRIVIALLY_COPYABLE(TValue) >::type >
class OOMArray {
public:
	typedef sserialize::SizeType SizeType;

	class ConstIterator {
	public:
		ConstIterator() : m_d(0), m_p(0) {}
		ConstIterator(const ConstIterator & other) : m_d(other.m_d), m_p(other.m_p) {}
		~ConstIterator() {}
		inline TValue operator*() { return m_d->get(m_p); }
		ConstIterator & operator++() { ++m_p; return *this; }
		bool operator!=(const ConstIterator & other) const { return m_d != other.m_d || m_p != other.m_p; }
	protected:
		friend class OOMArray;
		ConstIterator(OOMArray * d, SizeType s) : m_d(d), m_p(s) {}
		OOMArray * d() { return m_d; }
		const OOMArray * d() const { return m_d; }
		SizeType p() const { return m_p; }
	protected:
		OOMArray * m_d;
		SizeType m_p;
	};
	
	class Iterator: public ConstIterator {
	public:
		Iterator() : ConstIterator() {}
		Iterator(const Iterator & other) : ConstIterator(other) {}
		~Iterator() {}
		void set(const TValue & v) { ConstIterator::d()->set(ConstIterator::p(), v); }
		Iterator & operator++() { ConstIterator::operator++(); return *this; }
		bool operator!=(const Iterator & other) const { return ConstIterator::operator!=(other); }
	protected:
		friend class OOMArray;
		Iterator(OOMArray * d, SizeType s) : ConstIterator(d, s) {}
	};
	
	typedef ConstIterator const_iterator;
	typedef Iterator iterator;
	
public:
	OOMArray(MmappedMemoryType mmt);
	OOMArray(const OOMArray & other) = delete;
	OOMArray(OOMArray && other);
	~OOMArray();
	inline SizeType size() const { return m_size; }
	inline SizeType capacity() const { return m_capacity; }
	void reserve(SizeType reserveSize);
	void resize(SizeType newSize, const TValue & value = TValue());
	void clear();
	void shrink_to_fit();
	
	TValue get(SizeType pos) const;
	void set(SizeType pos, const TValue & v);
	
	inline TValue back() const { return get(m_size-1); }
	
	void push_back(const TValue & v);
	
	void emplace_back(TValue && v) { push_back(v); }
	void emplace_back(const TValue & v) { push_back(v); }
	
	template<typename... Args>
	inline void emplace_back(Args... args) {
		emplace_back(TValue(std::forward<Args>(args)...));
	}
	
	inline iterator begin() { return Iterator(this, 0); }
	inline iterator end() { return Iterator(this, size()); }
	inline const_iterator begin() const { return ConstIterator(this, 0); }
	inline const_iterator cbegin()  const { return begin(); }
	inline const_iterator end() const { return ConstIterator(this, size()); }
	inline const_iterator cend() const { return end(); }
private:
	int m_fd;
	std::string m_fn;
	sserialize::MmappedMemoryType m_mmt;
	//the number of elements that fit in, not the data size
	sserialize::SizeType m_capacity;
	sserialize::SizeType m_size;
};

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(MmappedMemoryType mmt) :
m_mmt(mmt),
m_capacity(16),
m_size(0)
{
	switch(m_mmt) {
	case sserialize::MM_FAST_FILEBASED:
	case sserialize::MM_SLOW_FILEBASED:
		m_fd = sserialize::FileHandler::createTmp(sizeof(TValue)*m_capacity, m_fn, mmt != MM_SLOW_FILEBASED);
		break;
	case sserialize::MM_PROGRAM_MEMORY:
	case sserialize::MM_SHARED_MEMORY:
	default:
		m_mmt = sserialize::MM_SHARED_MEMORY;
		m_fd = sserialize::FileHandler::shmCreate(m_fn);
		break;
	}
	
	if (m_fd < 0) {
		m_capacity = 0;
		throw sserialize::IOException("OOMArray could not create backend file");
	}
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(OOMArray<TValue, TEnable> && other) :
m_fd(other.m_fd),
m_fn(std::move(other.m_fn)),
m_capacity(other.m_capacity),
m_size(other.m_size)
{
	other.m_fd = -1;
	other.m_capacity = 0;
	other.m_size = 0;
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::~OOMArray() {
	if (m_fd >= 0) {
		::close(m_fd);
		switch(m_mmt) {
		case sserialize::MM_FAST_FILEBASED:
		case sserialize::MM_SLOW_FILEBASED:
			::unlink(m_fn.c_str());
			break;
		case sserialize::MM_SHARED_MEMORY:
			::shm_unlink(m_fn.c_str());
			break;
		default:
			break;
		}
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::reserve(SizeType reserveSize) {
	if (m_capacity >= reserveSize) {
		return;
	}
	if (::ftruncate(m_fd, reserveSize*sizeof(TValue)) < 0) {
		throw sserialize::IOException("OOMArray::reserve");
	}
	m_capacity = reserveSize;
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::resize(SizeType newSize, const TValue & v) {
	if (newSize <= m_size) {
		m_size = newSize;
	}
	else {
		reserve(newSize);
		for(SizeType i(m_size); i < newSize; ++i) {
			set(i, v);
		}
		m_size = newSize;
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::clear() {
	m_size = 0;
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::shrink_to_fit() {
	if (::ftruncate(m_fd, m_size*sizeof(TValue)) < 0) {
		throw sserialize::IOException("OOMArray::shrink_to_fit");
	}
}

template<typename TValue, typename TEnable>
TValue OOMArray<TValue, TEnable>::get(SizeType pos) const {
	TValue tmp;
	if (::pread64(m_fd, &tmp, sizeof(TValue), pos*sizeof(TValue)) != sizeof(TValue)) {
		throw sserialize::IOException("OOMArray::get");
	}
	return tmp;
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::set(SizeType pos, const TValue & v) {
	if (::pwrite64(m_fd, &v, sizeof(TValue), pos*sizeof(TValue)) != sizeof(TValue)) {
		throw IOException("OOMArray::set");
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::push_back(const TValue & v) {
	if (m_capacity <= m_size) {
		reserve(m_size*2);
	}
	set(m_size, v);
	++m_size;
}

}//end namespace
#endif