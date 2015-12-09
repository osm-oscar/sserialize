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
namespace detail {
namespace OOMArray {

template<typename TValue>
class ConstIterator;

template<typename TValue>
class Iterator;

}}//end namespace detail::OOMArray

template<typename TValue, typename TEnable = typename std::enable_if< sserialize::is_trivially_copyable<TValue>::value >::type >
class OOMArray {
private:
	friend class detail::OOMArray::ConstIterator<TValue>;
	friend class detail::OOMArray::Iterator<TValue>;
public:
	typedef sserialize::SizeType SizeType;
	typedef sserialize::DifferenceType DifferenceType;

	typedef detail::OOMArray::ConstIterator<TValue> const_iterator;
	typedef detail::OOMArray::Iterator<TValue> iterator;
	
public:
	OOMArray(MmappedMemoryType mmt);
	OOMArray(const OOMArray & other) = delete;
	OOMArray(OOMArray && other);
	~OOMArray();
	SizeType size() const { return m_backBufferBegin+m_backBuffer.size(); }
	SizeType capacity() const { return m_capacity; }
	void reserve(SizeType reserveSize);
	void resize(SizeType newSize, const TValue & value = TValue());
	void clear();
	void shrink_to_fit();
	
	///in Bytes
	void backBufferSize(sserialize::SizeType s);
	///in Bytes
	sserialize::SizeType backBufferSize() const { return m_backBufferSize*sizeof(TValue); }
	
	///standard read buffer size for iterators, in Bytes
	void readBufferSize(sserialize::SizeType s) { m_readBufferSize = s/sizeof(TValue); }
	///standard read buffer size for iterators, in Bytes
	sserialize::SizeType readBufferSize() const { return m_readBufferSize*sizeof(TValue); }
	
	void flush();
	
	TValue get(SizeType pos) const;
	void set(SizeType pos, const TValue & v);
	
	TValue back() const { return get(size()-1); }
	
	///buffered io
	void push_back(const TValue & v);
	
	///buffered io
	template<typename TIterator>
	void push_back(TIterator __begin, TIterator __end);
	
	///buffered io
	void emplace_back(TValue && v) { push_back(v); }
	///buffered io
	void emplace_back(const TValue & v) { push_back(v); }
	
	///invalidates iterators, unbuffered io
	template<typename TSourceIterator>
	iterator replace(const iterator & position, TSourceIterator srcBegin, const TSourceIterator & end);
	
	///buffered io
	template<typename... Args>
	void emplace_back(Args... args) {
		emplace_back(TValue(std::forward<Args>(args)...));
	}
	
	iterator begin() { return iterator(this, 0, readBufferSize()); }
	iterator end() { return iterator(this, size(), 0); }
	const_iterator begin() const { return const_iterator(this, 0, readBufferSize()); }
	const_iterator cbegin()  const { return begin(); }
	const_iterator end() const { return const_iterator(this, size(), 0); }
	const_iterator cend() const { return end(); }
	
	///invalidates iterators
	template<typename TSourceIterator>
	static iterator copy(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, const iterator & destBegin) {
		return destBegin.m_d->replace(destBegin, srcBegin, srcEnd);
	}
	
	///invalidates iterators
	template<typename TSourceIterator>
	static iterator move(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, const iterator & destBegin) {
		return destBegin.m_d->replace(destBegin, srcBegin, srcEnd);
	}
	
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

namespace detail {
namespace OOMArray {

///Buffered InputIterator, calls to set() or push_back/emplace_back invalidate this data
template<typename TValue>
class ConstIterator: public std::iterator<std::input_iterator_tag, TValue, sserialize::DifferenceType> {
public:
	ConstIterator() : m_d(0), m_p(0), m_bufferBegin(0), m_bufferSize(0) {}
	ConstIterator(ConstIterator && other) :
		m_d(other.m_d), m_p(other.m_p), m_bufferBegin(other.m_bufferBegin),
		m_bufferSize(other.m_bufferSize), m_buffer(std::move(other.m_buffer)) {}
	ConstIterator(const ConstIterator & other) :
		m_d(other.m_d), m_p(other.m_p), m_bufferBegin(other.m_bufferBegin),
		m_bufferSize(other.m_bufferSize), m_buffer(other.m_buffer) {}
	~ConstIterator() {}
	ConstIterator & operator=(const ConstIterator & other) {
		m_d = other.m_d;
		m_p = other.m_p;
		m_bufferBegin = other.m_bufferBegin;
		m_bufferSize = other.m_bufferSize;
		m_buffer = other.m_buffer;
		return *this;
	}
	ConstIterator & operator=(ConstIterator && other) {
		m_d = std::move(other.m_d);
		m_p = std::move(other.m_p);
		m_bufferBegin = std::move(other.m_bufferBegin);
		m_bufferSize = std::move(other.m_bufferSize);
		m_buffer = std::move(other.m_buffer);
		return *this;
	}
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator()
	const TValue & operator*() const { return m_buffer.at(m_p-m_bufferBegin); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator() and changes are not carried through
	TValue & operator*() { return m_buffer.at(m_p-m_bufferBegin); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator()
	const TValue * operator->() const { return &operator*(); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator() and changes are not carried through
	TValue * operator->() { return &operator*(); }
	ConstIterator & operator++() {
		++m_p;
		if (m_bufferBegin + m_buffer.size() <= m_p) {
			m_bufferBegin += m_buffer.size();
			m_buffer.clear();
			m_d->fill(m_buffer, m_bufferSize, m_p);
		}
		return *this;
	}
	ConstIterator operator+(uint32_t count) const { return ConstIterator(m_d, m_p+count, m_bufferSize); }
	sserialize::DifferenceType operator-(const ConstIterator & other) const { return (DifferenceType)(m_p) - (DifferenceType)(other.m_p); }
	bool operator<(const ConstIterator & other) const { return m_d == other.m_d && m_p < other.m_p; }
	bool operator!=(const ConstIterator & other) const { return m_d != other.m_d || m_p != other.m_p; }
	bool operator==(const ConstIterator & other) const { return m_d == other.m_d && m_p == other.m_p; }
	///s in bytes
	void bufferSize(SizeType s) { m_bufferSize = s/sizeof(TValue); }
	
	///in bytes
	SizeType bufferSize() const { return m_bufferSize*sizeof(TValue); }
protected:
	friend class sserialize::OOMArray<TValue>;
	ConstIterator(sserialize::OOMArray<TValue> * d, SizeType p, SizeType bs) : m_d(d), m_p(p), m_bufferBegin(m_p), m_bufferSize(bs) {
		m_d->fill(m_buffer, m_bufferSize, m_p);
	}
	sserialize::OOMArray<TValue> * d() { return m_d; }
	const sserialize::OOMArray<TValue> * d() const { return m_d; }
	SizeType p() const { return m_p; }
protected:
	sserialize::OOMArray<TValue> * m_d;
	SizeType m_p;
	SizeType m_bufferBegin;
	SizeType m_bufferSize;
	std::vector<TValue> m_buffer;
};

template<typename TValue>
class Iterator: public ConstIterator<TValue> {
public:
	typedef ConstIterator<TValue> MyBaseClass;
public:
	Iterator() : MyBaseClass() {}
	Iterator(Iterator && other) : MyBaseClass(std::move(other)) {}
	Iterator(const Iterator & other) : MyBaseClass(other) {}
	~Iterator() {}
	Iterator & operator=(const Iterator & other) {
		MyBaseClass::operator=(other);
		return *this;
	}
	Iterator & operator=(const Iterator && other) {
		MyBaseClass::operator=(std::move(other));
		return *this;
	}
	///potentially unbuffered write, invalidates other iterators!
	void set(const TValue & v) {
		MyBaseClass::d()->set(MyBaseClass::p(), v);
		MyBaseClass::operator*() = v;
	}
	Iterator & operator++() { MyBaseClass::operator++(); return *this; }
	Iterator operator+(uint32_t count) const { return Iterator( MyBaseClass::operator+(count) ); }
	bool operator<(const Iterator & other) const { return MyBaseClass::operator<(other); }
	sserialize::DifferenceType operator-(const Iterator & other) const { return MyBaseClass::operator-(other); }
	bool operator!=(const Iterator & other) const { return MyBaseClass::operator!=(other); }
	bool operator==(const Iterator & other) const { return MyBaseClass::operator==(other); }
protected:
	friend class sserialize::OOMArray<TValue>;
	Iterator(MyBaseClass && otherBase) : MyBaseClass(std::move(otherBase)) {}
	Iterator(sserialize::OOMArray<TValue> * d, SizeType p, SizeType bs) : MyBaseClass(d, p, bs) {}
};

}}//end namespace detail::OOMArray

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
		m_fd = sserialize::FileHandler::createTmp(sizeof(TValue)*m_capacity, m_fn, mmt == MM_FAST_FILEBASED, true);
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
	if (m_backBuffer.size() >= m_backBufferSize) {
		flush();
	}
}

template<typename TValue, typename TEnable>
template<typename TIterator>
void OOMArray<TValue, TEnable>::push_back(TIterator __begin, TIterator __end) {
	using std::distance;
	SizeType count = distance(__begin, __end);
	//check if count fits into buffer, if not, use replace
	if (count > m_backBufferSize-m_backBuffer.size()) {
		replace(begin(), __begin, __end);
	}
	else {
		m_backBuffer.insert(m_backBuffer.end(), __begin, __end);
		if (m_backBuffer.size() >= m_backBufferSize) {
			flush();
		}
	}
}

template<typename TValue, typename TEnable>
template<typename TSourceIterator>
typename OOMArray<TValue, TEnable>::iterator
OOMArray<TValue, TEnable>::replace(const iterator & position, TSourceIterator srcBegin, const TSourceIterator & srcEnd) {
	using std::distance;
	//first copy the stuff that is before the back-buffer
	SizeType offset = position.m_p;
	SizeType count = distance(srcBegin, srcEnd);
	
	if (offset+count > m_backBufferBegin) {
		//flush back buffer, makes everything easier
		flush();
		m_backBufferBegin = offset+count;
	}
	
	SizeType myBufferSize = std::min<SizeType>(m_backBufferSize, count);
	TValue * myBuffer = new TValue[myBufferSize];
	TValue * myBufferEnd = myBuffer+myBufferSize;
	while (srcBegin != srcEnd) {
		auto bufIt = myBuffer;
		for(; srcBegin != srcEnd && bufIt < myBufferEnd; ++srcBegin, ++bufIt) {
			*bufIt = *srcBegin;
		}
		::pwrite64(m_fd, myBuffer, sizeof(TValue)*(bufIt-myBuffer), offset*sizeof(TValue));
		offset += (bufIt-myBuffer);
	}
	delete myBuffer;
	assert(position.m_p+count == offset);
	return iterator(position.m_d, offset, position.bufferSize());
}

}//end namespace

namespace std {

///Copy into OOMArray, this invalidates other iterators that have the range within their cache
template<typename TSourceIterator, typename TValue>
sserialize::detail::OOMArray::Iterator<TValue>
copy(
	const TSourceIterator & srcBegin,
	const TSourceIterator & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::copy(srcBegin, srcEnd, targetBegin);
}

///Move into OOMArray, this invalidates other iterators that have the range within their cache
template<typename TSourceIterator, typename TValue>
sserialize::detail::OOMArray::Iterator<TValue>
move(
	const TSourceIterator & srcBegin,
	const TSourceIterator & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::move(srcBegin, srcEnd, targetBegin);
}

///Copy into OOMArray, this invalidates other iterators that have the range within their cache
template<typename TValue>
sserialize::detail::OOMArray::Iterator<TValue>
copy(
	const sserialize::detail::OOMArray::Iterator<TValue> & srcBegin,
	const sserialize::detail::OOMArray::Iterator<TValue> & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::copy(srcBegin, srcEnd, targetBegin);
}

///Move into OOMArray, this invalidates other iterators that have the range within their cache
template<typename TValue>
sserialize::detail::OOMArray::Iterator<TValue>
move(
	const sserialize::detail::OOMArray::Iterator<TValue> & srcBegin,
	const sserialize::detail::OOMArray::Iterator<TValue> & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::move(srcBegin, srcEnd, targetBegin);
}

template<typename TValue>
sserialize::DifferenceType
distance(const sserialize::detail::OOMArray::ConstIterator<TValue> & a, const sserialize::detail::OOMArray::ConstIterator<TValue> & b) {
	return b-a;
}

template<typename TValue>
sserialize::DifferenceType
distance(const sserialize::detail::OOMArray::Iterator<TValue> & a, const sserialize::detail::OOMArray::Iterator<TValue> & b) {
	return b-a;
}

template<typename TValue, typename TComp>
bool
is_sorted(sserialize::detail::OOMArray::ConstIterator<TValue> begin, const sserialize::detail::OOMArray::ConstIterator<TValue> & end, TComp comp) {
	if (begin == end) {
		return true;
	}
	auto prev(begin);
	for(++begin; begin != end; ++begin, ++prev) {
		if (comp(*begin, *prev)) {
			return false;
		}
	}
	return true;
}

template<typename TValue, typename TComp>
bool
is_sorted(const sserialize::detail::OOMArray::Iterator<TValue> & a, const sserialize::detail::OOMArray::Iterator<TValue> & b, const TComp & comp) {
	return is_sorted(
		static_cast< const sserialize::detail::OOMArray::ConstIterator<TValue> &>(a),
		static_cast< const sserialize::detail::OOMArray::ConstIterator<TValue> &>(b),
		comp
	);
}

template<typename TValue>
bool is_sorted(const sserialize::detail::OOMArray::ConstIterator<TValue> & a, const sserialize::detail::OOMArray::ConstIterator<TValue> & b) {
	return is_sorted(a, b, std::less<TValue>());
}

template<typename TValue>
bool is_sorted(const sserialize::detail::OOMArray::Iterator<TValue> & a, const sserialize::detail::OOMArray::Iterator<TValue> & b) {
	return is_sorted(a, b, std::less<TValue>());
}

}//end namespace std

#endif