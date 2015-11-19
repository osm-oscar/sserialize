#ifndef SSERIALIZE_OOM_ARRAY_H
#define SSERIALIZE_OOM_ARRAY_H
#include <sserialize/storage/FileHandler.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/MmappedMemory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sserialize/utility/type_traits.h>
#include <vector>

namespace sserialize {

template<typename TValue, typename TEnable = typename std::enable_if< sserialize::is_trivially_copyable<TValue>::value >::type >
class OOMArray {
public:
	typedef sserialize::SizeType SizeType;

	///Buffered InputIterator, calls to set() or push_back/emplace_back invalidate this data
	class ConstIterator {
	public:
		ConstIterator() : m_d(0), m_p(0), m_bufferBegin(0), m_bufferSize(0) {}
		ConstIterator(ConstIterator && other) :
			m_d(other.m_d), m_p(other.m_p), m_bufferBegin(other.m_bufferBegin),
			m_bufferSize(other.m_bufferSize), m_buffer(std::move(other.m_buffer)) {}
		ConstIterator(const ConstIterator & other) :
			m_d(other.m_d), m_p(other.m_p), m_bufferBegin(other.m_bufferBegin),
			m_bufferSize(other.m_bufferSize), m_buffer(other.m_buffer) {}
		~ConstIterator() {}
		///The returned reference is invalidated by a call to operator++() or ~ConstIterator()
		inline const TValue & operator*() const { return m_buffer.at(m_p-m_bufferBegin); }
		///The returned reference is invalidated by a call to operator++() or ~ConstIterator() and changes are not carried through
		inline TValue & operator*() { return m_buffer.at(m_p-m_bufferBegin); }
		ConstIterator & operator++() {
			++m_p;
			if (m_bufferBegin + m_buffer.size() <= m_p) {
				m_bufferBegin += m_buffer.size();
				m_buffer.clear();
				m_d->fill(m_buffer, m_bufferSize, m_p);
			}
			return *this;
		}
		inline bool operator!=(const ConstIterator & other) const { return m_d != other.m_d || m_p != other.m_p; }
		///s in bytes
		inline void bufferSize(SizeType s) { m_bufferSize = s/sizeof(TValue); }
		
		///in bytes
		inline SizeType bufferSize() const { return m_bufferSize*sizeof(TValue); }
	protected:
		friend class OOMArray;
		ConstIterator(OOMArray * d, SizeType p, SizeType bs) : m_d(d), m_p(p), m_bufferBegin(m_p), m_bufferSize(bs) {
			m_d->fill(m_buffer, m_bufferSize, m_p);
		}
		OOMArray * d() { return m_d; }
		const OOMArray * d() const { return m_d; }
		SizeType p() const { return m_p; }
	protected:
		OOMArray * m_d;
		SizeType m_p;
		SizeType m_bufferBegin;
		SizeType m_bufferSize;
		std::vector<TValue> m_buffer;
	};
	
	class Iterator: public ConstIterator {
	public:
		Iterator() : ConstIterator() {}
		Iterator(Iterator && other) : ConstIterator(std::move(other)) {}
		Iterator(const Iterator & other) : ConstIterator(other) {}
		~Iterator() {}
		///potentially unbuffered write, invalidates other iterators!
		void set(const TValue & v) {
			ConstIterator::d()->set(ConstIterator::p(), v);
			ConstIterator::operator*() = v;
		}
		Iterator & operator++() { ConstIterator::operator++(); return *this; }
		bool operator!=(const Iterator & other) const { return ConstIterator::operator!=(other); }
	protected:
		friend class OOMArray;
		Iterator(OOMArray * d, SizeType p, SizeType bs) : ConstIterator(d, p, bs) {}
	};
	
	typedef ConstIterator const_iterator;
	typedef Iterator iterator;
	
public:
	OOMArray(MmappedMemoryType mmt);
	OOMArray(const OOMArray & other) = delete;
	OOMArray(OOMArray && other);
	~OOMArray();
	inline SizeType size() const { return m_backBufferBegin+m_backBuffer.size(); }
	inline SizeType capacity() const { return m_capacity; }
	void reserve(SizeType reserveSize);
	void resize(SizeType newSize, const TValue & value = TValue());
	void clear();
	void shrink_to_fit();
	
	///in Bytes
	void backBufferSize(sserialize::SizeType s);
	///in Bytes
	inline sserialize::SizeType backBufferSize() const { return m_backBufferSize*sizeof(TValue); }
	
	///standard read buffer size for iterators, in Bytes
	inline void readBufferSize(sserialize::SizeType s) { m_readBufferSize = s/sizeof(TValue); }
	///standard read buffer size for iterators, in Bytes
	inline sserialize::SizeType readBufferSize() const { return m_readBufferSize*sizeof(TValue); }
	
	void flush();
	
	TValue get(SizeType pos) const;
	void set(SizeType pos, const TValue & v);
	
	inline TValue back() const { return get(size()-1); }
	
	///buffered io
	void push_back(const TValue & v);
	///buffered io
	void emplace_back(TValue && v) { push_back(v); }
	///buffered io
	void emplace_back(const TValue & v) { push_back(v); }
	
	///buffered io
	template<typename... Args>
	inline void emplace_back(Args... args) {
		emplace_back(TValue(std::forward<Args>(args)...));
	}
	
	inline iterator begin() { return Iterator(this, 0, readBufferSize()); }
	inline iterator end() { return Iterator(this, size(), 0); }
	inline const_iterator begin() const { return ConstIterator(this, 0, readBufferSize()); }
	inline const_iterator cbegin()  const { return begin(); }
	inline const_iterator end() const { return ConstIterator(this, size(), 0); }
	inline const_iterator cend() const { return end(); }
private:
	void fill(std::vector<TValue> & buffer, SizeType bufferSize, SizeType p);
private:
	int m_fd;
	std::string m_fn;
	sserialize::MmappedMemoryType m_mmt;
	//the number of elements that fit in, not the data size
	sserialize::SizeType m_capacity;
	sserialize::SizeType m_backBufferBegin;
	//number of entries in back buffer
	sserialize::SizeType m_backBufferSize;
	//write buffer for push_back operations
	std::vector<TValue> m_backBuffer;
	//number of entries in read buffer
	sserialize::SizeType m_readBufferSize;
};

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::fill(std::vector<TValue> & buffer, SizeType bufferSize, SizeType p) {
	buffer.clear();
	if (p >= size()) {
		return;
	}
	if (p < m_backBufferBegin) {
		SizeType fileCopyCount = std::min<SizeType>(bufferSize, m_backBufferBegin-p);
		buffer.resize(fileCopyCount);
		
		::pread64(m_fd, &(buffer[0]), sizeof(TValue)*fileCopyCount, sizeof(TValue)*p);
		
		bufferSize -= fileCopyCount;
		p = 0;
	}
	else {
		p -= m_backBufferBegin;
	}
	if (bufferSize) {
		SizeType pe = p+bufferSize;
		pe = std::min(pe, m_backBuffer.size());
		buffer.insert(buffer.end(), m_backBuffer.begin()+p, m_backBuffer.begin()+pe);
	}
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(MmappedMemoryType mmt) :
m_mmt(mmt),
m_capacity(1024*1024/sizeof(TValue)),
m_backBufferBegin(0),
m_backBufferSize(m_capacity),
m_readBufferSize(m_capacity/16)
{
	switch(m_mmt) {
	case sserialize::MM_FAST_FILEBASED:
	case sserialize::MM_SLOW_FILEBASED:
		m_fd = sserialize::FileHandler::createTmp(sizeof(TValue)*m_capacity, m_fn, mmt == MM_FAST_FILEBASED);
		break;
	case sserialize::MM_PROGRAM_MEMORY:
	case sserialize::MM_SHARED_MEMORY:
	default:
		m_mmt = sserialize::MM_SHARED_MEMORY;
		m_fd = sserialize::FileHandler::shmCreate(m_fn);
		::ftruncate(m_fd, sizeof(TValue)*m_capacity);
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
m_backBufferBegin(other.m_size),
m_backBufferSize(other.m_backBufferSize),
m_backBuffer(std::move(other.m_backBuffer)),
m_readBufferSize(other.m_readBufferSize)
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
void OOMArray<TValue, TEnable>::backBufferSize(SizeType s) {
	if (s < m_backBuffer.size()) {
		flush();
	}
	m_backBufferSize = s;
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::reserve(SizeType reserveSize) {
	flush();
	
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
	if (newSize <= size()) {
		flush();
		m_backBufferBegin = newSize;
	}
	else {
		reserve(newSize);
		for(SizeType i(size()); i < newSize; ++i) {
			push_back(v);
		}
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::clear() {
	m_backBufferBegin = 0;
	m_backBuffer.clear();
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::shrink_to_fit() {
	flush();

	if (::ftruncate(m_fd, size()*sizeof(TValue)) < 0) {
		throw sserialize::IOException("OOMArray::shrink_to_fit");
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::flush() {
	::pwrite64(m_fd, &(m_backBuffer[0]), sizeof(TValue)*m_backBuffer.size(), m_backBufferBegin*sizeof(TValue));
	m_backBufferBegin += m_backBuffer.size();
	m_backBuffer.clear();
}

template<typename TValue, typename TEnable>
TValue OOMArray<TValue, TEnable>::get(SizeType pos) const {
	if (pos < m_backBufferBegin) {
		TValue tmp;
		if (::pread64(m_fd, &tmp, sizeof(TValue), pos*sizeof(TValue)) != sizeof(TValue)) {
			throw sserialize::IOException("OOMArray::get");
		}
		return tmp;
	}
	else {
		return m_backBuffer[pos - m_backBufferBegin];
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::set(SizeType pos, const TValue & v) {
	if (pos < m_backBufferBegin) {
		if (::pwrite64(m_fd, &v, sizeof(TValue), pos*sizeof(TValue)) != sizeof(TValue)) {
			throw IOException("OOMArray::set");
		}
	}
	else {
		m_backBuffer[pos - m_backBufferBegin] = v;
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::push_back(const TValue & v) {
	if (m_capacity <= size()) {
		reserve(size()*2);
	}
	m_backBuffer.push_back(v);
	if (m_backBuffer.size() >= (1024*1024)/sizeof(TValue)) {
		flush();
	}
}

}//end namespace
#endif