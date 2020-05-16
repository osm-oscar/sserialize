#ifndef SSERIALIZE_OOM_ARRAY_H
#define SSERIALIZE_OOM_ARRAY_H
#include <sserialize/storage/FileHandler.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/utility/type_traits.h>
#include <sserialize/utility/constants.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/utility/assert.h>
#include <sserialize/utility/types.h>
#include <vector>
#include <sys/mman.h>
#include <errno.h>

namespace sserialize {
namespace detail {
namespace OOMArray {

template<typename TValue>
class IteratorBuffer;

template<typename TValue>
class ConstIterator;

template<typename TValue>
class Iterator;

}}//end namespace detail::OOMArray

template<typename TValue, typename TEnable = typename std::enable_if< sserialize::is_trivially_copyable<TValue>::value >::type >
class OOMArray {
private:
	friend class detail::OOMArray::IteratorBuffer<TValue>;
	friend class detail::OOMArray::ConstIterator<TValue>;
	friend class detail::OOMArray::Iterator<TValue>;
public:
	typedef sserialize::SizeType SizeType;
	typedef sserialize::DifferenceType DifferenceType;
	
	typedef TValue value_type;

	typedef detail::OOMArray::ConstIterator<value_type> const_iterator;
	typedef detail::OOMArray::Iterator<value_type> iterator;
	
public:
	OOMArray(const std::string & fileName);
	OOMArray(MmappedMemoryType mmt);
	OOMArray(const OOMArray & other) = delete;
	OOMArray(OOMArray && other);
	~OOMArray();
	///Swap data but don't swap config (like buffer sizes, deletion etc)
	///invalidates iterators of this and other
	void swap_data(OOMArray & other);
	MmappedMemoryType mmt() const { return m_mmt; }
	const std::string & storageFileName() const { return m_fn; }
	SizeType size() const { return m_backBufferBegin+m_backBuffer.size(); }
	void reserve(SizeType reserveSize);
	void resize(SizeType newSize, const value_type & value = value_type());
	void clear();
	void shrink_to_fit();
	void sync();
	
	bool syncOnFlush() const { return m_syncOnFlush; }
	///if you disable syncOnFlush then read operations may return wrong data (call sync() before)
	void syncOnFlush(bool v) { m_syncOnFlush = v; }
	
	///in Bytes
	void backBufferSize(sserialize::SizeType s);
	///in Bytes
	sserialize::SizeType backBufferSize() const { return m_backBufferSize*sizeof(value_type); }
	
	///standard read buffer size for iterators, in Bytes
	void readBufferSize(sserialize::SizeType s) { m_readBufferSize = s/sizeof(value_type); }
	///standard read buffer size for iterators, in Bytes
	sserialize::SizeType readBufferSize() const { return m_readBufferSize*sizeof(value_type); }
	
	void flush();
	
	inline value_type at(SizeType pos) const { return get(pos); }
	value_type get(SizeType pos) const;
	void set(SizeType pos, const value_type & v);
	
	value_type back() const { return get(size()-1); }
	
	///buffered io
	void push_back(const value_type & v);
	
	///buffered io
	template<typename TIterator>
	void push_back(TIterator __begin, TIterator __end);
	
	///buffered io
	void emplace_back(value_type && v) { push_back(v); }
	///buffered io
	void emplace_back(const value_type & v) { push_back(v); }
	
	///invalidates iterators, unbuffered io
	template<typename TSourceIterator>
	iterator replace(const iterator & position, TSourceIterator srcBegin, const TSourceIterator & end);
	
	///invalidates iterators, unbuffered io
	template<typename TSourceIterator>
	SizeType replace(SizeType position, TSourceIterator srcBegin, const TSourceIterator & end);
	
	///buffered io
	template<typename... Args>
	void emplace_back(Args... args) {
		emplace_back(TValue(std::forward<Args>(args)...));
	}
	
	iterator begin() { return iterator(this, 0, readBufferSize()); }
	const_iterator begin() const { return const_iterator(this, 0, readBufferSize()); }
	const_iterator cbegin()  const { return begin(); }

	iterator end() { return iterator(this, size(), 0); }
	const_iterator end() const { return const_iterator(this, size(), 0); }
	const_iterator cend() const { return end(); }
	
	///invalidates iterators
	template<typename TSourceIterator>
	static iterator copy(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, iterator destBegin) {
		return destBegin.d()->replace(destBegin, srcBegin, srcEnd);
	}
	
	///invalidates iterators
	template<typename TSourceIterator>
	static iterator move(const TSourceIterator & srcBegin, const TSourceIterator & srcEnd, iterator destBegin) {
		return destBegin.d()->replace(destBegin, srcBegin, srcEnd);
	}

	///invalidates iterators
	static iterator copy(iterator srcBegin, const iterator & srcEnd, iterator destBegin) {
		return destBegin.d()->replace(destBegin, std::move(srcBegin), srcEnd);
	}
	///invalidates iterators
	static iterator copy(const_iterator srcBegin, const const_iterator & srcEnd, iterator destBegin) {
		return destBegin.d()->replace(destBegin, std::move(srcBegin), srcEnd);
	}
	
	///invalidates iterators
	static iterator move(iterator srcBegin, const iterator & srcEnd, iterator destBegin);
	
	///invalidates iterators
	static iterator move(const_iterator srcBegin, const const_iterator & srcEnd, iterator destBegin) {
		return destBegin.d()->replace(destBegin, std::move(srcBegin), srcEnd);
	}
	
	static OOMArray* backend(iterator it) {
		return it.d();
	}
	
	static const OOMArray * backend(const_iterator it) {
		return it.d();
	}
	
private:
	void fill(std::vector<value_type> & buffer, SizeType bufferSize, SizeType p);
	///invalidates iterators, unbuffered io
	template<typename TSourceIterator>
	SizeType replaceWithoutFlush(SizeType position, TSourceIterator srcBegin, TSourceIterator const & srcEnd);
	SizeType replaceWithoutFlush(SizeType position, typename std::vector<TValue>::iterator srcBegin, typename std::vector<TValue>::iterator srcEnd) {
		return replaceWithoutFlush(position, &(*srcBegin), &(*srcEnd));
	}
	SizeType replaceWithoutFlush(SizeType position, typename std::vector<TValue>::const_iterator srcBegin, typename std::vector<TValue>::const_iterator srcEnd) {
		return replaceWithoutFlush(position, &(*srcBegin), &(*srcEnd));
	}
	SizeType replaceWithoutFlush(SizeType position, TValue * srcBegin, TValue * srcEnd) {
		return replaceWithoutFlush(position, (TValue const*)(srcBegin), (TValue const*)(srcEnd)); 
	}
	SizeType replaceWithoutFlush(SizeType position, TValue const * srcBegin, TValue const *srcEnd);
private:
	int m_fd;
	std::string m_fn;
	bool m_delete;
	bool m_syncOnFlush;
	sserialize::MmappedMemoryType m_mmt;
	sserialize::SizeType m_backBufferBegin; //number of entries before back buffer
	//maximum number of entries in back buffer
	sserialize::SizeType m_backBufferSize;
	//write buffer for push_back operations
	std::vector<value_type> m_backBuffer;
	//number of entries in read buffer
	sserialize::SizeType m_readBufferSize;
};

namespace detail {
namespace OOMArray {

template<typename TValue>
class IteratorBuffer: public RefCountObject {
public:
	typedef std::vector<TValue> BufferType;
	typedef sserialize::OOMArray<TValue> BaseContainerType;
public:
	///@param bs in Bytes
	IteratorBuffer(BaseContainerType * d, SizeType bb, SizeType bs) : m_d(d), m_bufferBegin(bb), m_bufferSize(bs/sizeof(TValue)) {
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(m_d->size(), bb);
		m_d->fill(m_buffer, m_bufferSize, m_bufferBegin);
	}
	IteratorBuffer(IteratorBuffer && other) = delete;
	IteratorBuffer(const IteratorBuffer & other) = delete;
	IteratorBuffer & operator=(IteratorBuffer && other) = delete;
	IteratorBuffer & operator=(const IteratorBuffer & other) = delete;
	virtual ~IteratorBuffer() {
		m_d = 0;
	}
	///sync buffer from backend
	void sync() {
		m_buffer.clear();
		m_d->fill(m_buffer, m_bufferSize, m_bufferBegin);
	}
	
	BufferType & buffer() { return m_buffer; }
	const BufferType & buffer() const { return m_buffer; }
	SizeType bufferBegin() const { return m_bufferBegin; }

	///@param s in Bytes
	///@return 0 if single parent, new IteratorBuffer otherwise
	IteratorBuffer * bufferSize(SizeType s) {
		if (rc() == 1) {
			m_bufferSize = s/sizeof(TValue);
			return 0;
		}
		else {
			return new IteratorBuffer(d(), m_bufferBegin, s);
		}
	}
	///in Bytes
	SizeType bufferSize() const { return m_bufferSize*sizeof(TValue); }

	BaseContainerType * d() { return m_d; }
	const BaseContainerType * d() const { return m_d; }

	const TValue & get(SizeType p) const {
		SSERIALIZE_CHEAP_ASSERT_LARGER(d()->size(), p);
		return buffer().at(p-bufferBegin());
	}
	TValue & get(SizeType p) {
		SSERIALIZE_CHEAP_ASSERT_LARGER(d()->size(), p);
		return m_buffer.at(p-bufferBegin());
	}
	
	///returns either a new buffer or this buffer if position is within this buffer
	IteratorBuffer * getRepositioned(SizeType position) {
		if (position < m_bufferBegin || position-m_bufferBegin >= m_buffer.size()) {
			return new IteratorBuffer(m_d, position, bufferSize());
		}
		return this;
	}
	
	IteratorBuffer * copy() const {
		return new IteratorBuffer(m_d, m_bufferBegin, m_bufferSize, m_buffer);
	}
	
	///increment this buffer, if position is within the buffer, nothing happens and nullptr is returned
	///if it is outside a buffer fill is done if only one parent is present
	///if there are multiple present, then a new buffer is created
	IteratorBuffer * incTo(SizeType position) {
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(position, m_bufferBegin);
		if (position-m_bufferBegin >= m_buffer.size()) {
			if (rc() == 1) {
				m_bufferBegin = position;
				m_buffer.clear();
				m_d->fill(m_buffer, m_bufferSize, m_bufferBegin);
			}
			else {
				return new IteratorBuffer(m_d, position, bufferSize());
			}
		}
		return 0;
	}
private:
	IteratorBuffer(BaseContainerType * d, SizeType bb, SizeType bs, const BufferType & b) :
	m_d(d),
	m_bufferBegin(bb),
	m_bufferSize(bs),
	m_buffer(b)
	{}
private:
	BaseContainerType * m_d;
	SizeType m_bufferBegin; //in entries
	SizeType m_bufferSize; //in entries
	BufferType m_buffer;

};

///Buffered InputIterator, calls to set() or push_back/emplace_back invalidate this data
template<typename TValue>
class ConstIterator: public std::iterator<std::input_iterator_tag, TValue, sserialize::DifferenceType> {
protected:
	typedef IteratorBuffer<TValue> MyIteratorBuffer;
	typedef sserialize::RCPtrWrapper<MyIteratorBuffer> MyIteratorBufferPtr;
	typedef std::vector<TValue> BufferType;
	typedef sserialize::OOMArray<TValue> BaseContainerType;
public:
	ConstIterator() : m_b(0), m_p(0) {}
	ConstIterator(ConstIterator && other) : m_b(std::move(other.m_b)), m_p(other.m_p) {
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
	}
	ConstIterator(const ConstIterator & other) : m_b(other.m_b), m_p(other.m_p)
	{
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
	}
	~ConstIterator() {}
	ConstIterator & operator=(const ConstIterator & other) {
		m_b = other.m_b;
		m_p = other.m_p;
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		return *this;
	}
	ConstIterator & operator=(ConstIterator && other) {
		m_b = std::move(other.m_b);
		m_p = std::move(other.m_p);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		return *this;
	}
	ConstIterator copy() {
		return ConstIterator(MyIteratorBufferPtr(m_b->copy()), m_p);
	}
	///sync the buffer of this iterator with the backend
	void sync() { m_b->sync(); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator()
	const TValue & operator*() const { return m_b->get(m_p); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator() and changes are not carried through
	TValue & operator*() { return m_b->get(m_p); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator()
	const TValue * operator->() const { return &operator*(); }
	///The returned reference is invalidated by a call to operator++() or ~ConstIterator() and changes are not carried through
	TValue * operator->() { return &operator*(); }
	ConstIterator & operator++() {
		return operator+=(1);
	}
	ConstIterator operator+(SizeType count) const {
		return ConstIterator(m_b, m_p+count);
	}
	ConstIterator & operator+=(SizeType count) {
		m_p += count;
		MyIteratorBuffer * tmp = m_b->incTo(m_p);
		if (tmp) {
			m_b.reset(tmp);
		}
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		return *this;
	}
	sserialize::DifferenceType operator-(const ConstIterator & other) const { return (DifferenceType)(m_p) - (DifferenceType)(other.m_p); }
	bool operator<(const ConstIterator & other) const {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(d(), other.d());
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		return d() == other.d() && m_p < other.m_p;
	}
	bool operator!=(const ConstIterator & other) const {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(d(), other.d());
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), other.m_p);
		return d() != other.d() || m_p != other.m_p;
	}
	bool operator==(const ConstIterator & other) const {
		SSERIALIZE_CHEAP_ASSERT_EQUAL(d(), other.d());
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), other.m_p);
		return d() == other.d() && m_p == other.m_p;
	}
	///s in Bytes
	void bufferSize(SizeType s) {
		MyIteratorBuffer * tmp = m_b->bufferSize(s);
		if (UNLIKELY_BRANCH(tmp)) {
			m_b.reset(tmp);
		}
	}
	
	///in Bytes
	SizeType bufferSize() const { return m_b->bufferSize(); }
protected:
	friend class sserialize::OOMArray<TValue>;
	///@bs in Bytes
	ConstIterator(sserialize::OOMArray<TValue> * d, SizeType p, SizeType bs) : m_b(new MyIteratorBuffer(d, p, bs)), m_p(p) {}
	ConstIterator(MyIteratorBufferPtr b, SizeType p) : m_b(b->getRepositioned(p)), m_p(p) {
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(d()->size(), m_p);
	}

	BufferType & buffer() { return m_b->buffer(); }
	const BufferType & buffer() const { return m_b->buffer(); }
	SizeType bufferBegin() const { return m_b->bufferBegin(); }

	sserialize::OOMArray<TValue> * d() { return m_b->d(); }
	const sserialize::OOMArray<TValue> * d() const { return m_b->d(); }
	SizeType p() const { return m_p; }

protected:
	MyIteratorBufferPtr m_b;
	SizeType m_p;
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
	Iterator & operator+=(SizeType count) { MyBaseClass::operator+=(count); return *this; }
	Iterator operator+(SizeType count) const { return Iterator( MyBaseClass::operator+(count) ); }
	bool operator<(const Iterator & other) const { return MyBaseClass::operator<(other); }
	sserialize::DifferenceType operator-(const Iterator & other) const { return MyBaseClass::operator-(other); }
	bool operator!=(const Iterator & other) const { return MyBaseClass::operator!=(other); }
	bool operator==(const Iterator & other) const { return MyBaseClass::operator==(other); }
protected:
	friend class sserialize::OOMArray<TValue>;
	Iterator(MyBaseClass && otherBase) : MyBaseClass(std::move(otherBase)) {}
	///@param bs in Bytes
	Iterator(sserialize::OOMArray<TValue> * d, SizeType p, SizeType bs) : MyBaseClass(d, p, bs) {}
};

}}//end namespace detail::OOMArray

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::fill(std::vector<TValue> & buffer, SizeType bufferSize, SizeType p) {
	#ifdef SSERIALIZE_NORMAL_ASSERT_ENABLED
	if (m_syncOnFlush) {
		SSERIALIZE_NORMAL_ASSERT_EQUAL(sserialize::MmappedFile::fileSize(m_fd), m_backBufferBegin*sizeof(value_type));
	}
	#endif
	buffer.clear();
	if (!bufferSize || p >= size()) {
		buffer.shrink_to_fit();
		return;
	}
	if (p < m_backBufferBegin) {
		SizeType fileCopyCount = std::min<SizeType>(bufferSize, m_backBufferBegin-p);
		buffer.resize(fileCopyCount);
		
		SizeType readSize = sizeof(TValue)*fileCopyCount;
		FileHandler::pread(m_fd, &(buffer[0]), readSize, sizeof(TValue)*p);
		
		bufferSize -= fileCopyCount;
		p = 0;
	}
	else {
		p -= m_backBufferBegin;
	}
	if (bufferSize) {
		SizeType pe = p+bufferSize;
		pe = std::min<SizeType>(pe, m_backBuffer.size());
		buffer.insert(buffer.end(), m_backBuffer.begin()+p, m_backBuffer.begin()+pe);
	}
	buffer.shrink_to_fit();
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(const std::string & fileName) :
m_fd(-1),
m_fn(fileName),
m_delete(false),
m_syncOnFlush(true),
m_mmt(sserialize::MM_INVALID),
m_backBufferBegin(0),
m_backBufferSize((1024*1024)/sizeof(TValue)),
m_readBufferSize(m_backBufferSize/16)
{
	if (!MmappedFile::fileExists(fileName)) {
		throw sserialize::IOException("sserialize::OOMArray: input file " + fileName + " does not exist");
	}
	m_backBufferBegin = sserialize::MmappedFile::fileSize(fileName);

	if (m_backBufferBegin % sizeof(value_type)) {
		throw sserialize::IOException("sserialize::OOMArray: input file " + fileName + " is not a multiple of sizeof(value_type)");
	}
	m_backBufferBegin /= sizeof(value_type);
	
	m_fd = FileHandler::open(fileName);
	if (m_fd < 0) {
		throw sserialize::IOException("OOMArray could not open backend file " + std::string(::strerror(errno)));
	}

	m_backBuffer.reserve(m_backBufferSize);
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(MmappedMemoryType mmt) :
m_fd(-1),
m_delete(true),
m_syncOnFlush(true),
m_mmt(mmt),
m_backBufferBegin(0),
m_backBufferSize((1024*1024)/sizeof(TValue)),
m_readBufferSize(m_backBufferSize/16)
{
	m_backBuffer.reserve(m_backBufferSize);

	switch(m_mmt) {
	case sserialize::MM_FAST_FILEBASED:
	case sserialize::MM_SLOW_FILEBASED:
		m_fd = sserialize::FileHandler::createTmp(0, m_fn, mmt == MM_FAST_FILEBASED, false);
		break;
	case sserialize::MM_PROGRAM_MEMORY:
	case sserialize::MM_SHARED_MEMORY:
	default:
		m_mmt = sserialize::MM_SHARED_MEMORY;
		m_fd = sserialize::FileHandler::shmCreate(m_fn);
		if (m_fd < 0) {
			throw sserialize::IOException("OOMArray could not create backend file: " + std::string(::strerror(errno)));
		}
			
		if (::ftruncate(m_fd, sizeof(TValue)) < 0) {
			std::string errmsg(::strerror(errno));
			::close(m_fd);
			::shm_unlink(m_fn.c_str());
			throw sserialize::IOException("OOMArray could not create backend file: " + errmsg);
		}
		break;
	}
	
	if (m_fd < 0) {
		throw sserialize::IOException("OOMArray could not create backend file: " + std::string(::strerror(errno)));
	}
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::OOMArray(OOMArray<TValue, TEnable> && other) :
m_fd(other.m_fd),
m_fn(std::move(other.m_fn)),
m_delete(other.m_delete),
m_syncOnFlush(true),
m_backBufferBegin(other.m_size),
m_backBufferSize(other.m_backBufferSize),
m_backBuffer(std::move(other.m_backBuffer)),
m_readBufferSize(other.m_readBufferSize)
{
	other.m_fd = -1;
	other.m_size = 0;
	other.m_delete = false;
}

template<typename TValue, typename TEnable>
OOMArray<TValue, TEnable>::~OOMArray() {
	if (m_delete && m_fd >= 0) {
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
	else if (m_syncOnFlush) {
		sync();
	}
}
template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::swap_data(OOMArray & other) {
	//make sure that backbuffer sizes match what is defined
	if (m_backBufferSize != other.m_backBufferSize) {
		SizeType myBackBufferSize = m_backBufferSize;
		this->backBufferSize( other.m_backBufferSize*sizeof(value_type) );
		other.backBufferSize( myBackBufferSize*sizeof(value_type) );
	}

	using std::swap;

	swap(m_fd, other.m_fd);
	swap(m_fn, other.m_fn);
	swap(m_mmt, other.m_mmt);
	swap(m_backBufferSize, other.m_backBufferSize);
	swap(m_backBufferBegin, other.m_backBufferBegin);
	swap(m_backBuffer, other.m_backBuffer);
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::backBufferSize(SizeType s) {
	m_backBufferSize = s/sizeof(TValue);
	if (m_backBuffer.size()) {
		flush();
	}
	m_backBuffer.shrink_to_fit();
	m_backBuffer.reserve(m_backBufferSize);
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::reserve(SizeType /*reserveSize*/) {
	//nothing to reserve here
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::resize(SizeType newSize, const TValue & v) {
	if (newSize <= size()) {
		flush();
		if (::ftruncate(m_fd, newSize*sizeof(value_type)) < 0) {
			throw sserialize::IOException("OOMArray::resize: " + std::string(::strerror(errno)));
		}
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
	if (::ftruncate(m_fd, 0) < 0) {
		throw sserialize::IOException("OOMArray::clear: " + std::string(::strerror(errno)));
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::shrink_to_fit() {
	flush();

	if (::ftruncate(m_fd, size()*sizeof(TValue)) < 0) {
		throw sserialize::IOException("OOMArray::shrink_to_fit: " + std::string(::strerror(errno)));
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::sync() {
	::fdatasync(m_fd);
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::flush() {
	SizeType writeSize = sizeof(TValue)*m_backBuffer.size();
	if (writeSize) {
		#ifdef SSERIALIZE_NORMAL_ASSERT_ENABLED
		if (m_syncOnFlush) {
			SSERIALIZE_NORMAL_ASSERT_EQUAL(sserialize::MmappedFile::fileSize(m_fd), m_backBufferBegin*sizeof(value_type));
		}
		#endif
		FileHandler::pwrite(m_fd, &(m_backBuffer[0]), writeSize, m_backBufferBegin*sizeof(TValue));
		m_backBufferBegin += m_backBuffer.size();
		m_backBuffer.clear();
		if (m_syncOnFlush) {
			::fdatasync(m_fd);
			SSERIALIZE_NORMAL_ASSERT_EQUAL(sserialize::MmappedFile::fileSize(m_fd), m_backBufferBegin*sizeof(value_type));
		}
	}
}

template<typename TValue, typename TEnable>
TValue OOMArray<TValue, TEnable>::get(SizeType pos) const {
	if (pos < m_backBufferBegin) {
		TValue tmp;
		FileHandler::pread(m_fd, &tmp, sizeof(TValue), pos*sizeof(TValue));
		return tmp;
	}
	else {
		return m_backBuffer[pos - m_backBufferBegin];
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::set(SizeType pos, const TValue & v) {
	if (pos < m_backBufferBegin) {
		FileHandler::pwrite(m_fd, &v, sizeof(TValue), pos*sizeof(TValue));
	}
	else {
		m_backBuffer[pos - m_backBufferBegin] = v;
	}
}

template<typename TValue, typename TEnable>
void OOMArray<TValue, TEnable>::push_back(const TValue & v) {
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
		replace(end(), __begin, __end);
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
	auto offset = replace(position.p(), srcBegin, srcEnd);
	return iterator(this, offset, position.bufferSize());
}

template<typename TValue, typename TEnable>
template<typename TSourceIterator>
typename OOMArray<TValue, TEnable>::SizeType
OOMArray<TValue, TEnable>::replace(SizeType position, TSourceIterator srcBegin, const TSourceIterator & srcEnd) {
	SizeType offset = position;
	SizeType count = distance(srcBegin, srcEnd);
	
	if (offset+count > m_backBufferBegin) {
		//flush back buffer, makes everything easier
		flush();
		m_backBufferBegin = std::max<uint64_t>(offset+count, m_backBufferBegin);
	}
	
	offset = replaceWithoutFlush(position, srcBegin, srcEnd);
	
	if (m_syncOnFlush) {
		::fdatasync(m_fd);
		SSERIALIZE_NORMAL_ASSERT_EQUAL(sserialize::MmappedFile::fileSize(m_fd), m_backBufferBegin*sizeof(value_type));
	}
	return offset;
}

template<typename TValue, typename TEnable>
template<typename TSourceIterator>
typename OOMArray<TValue, TEnable>::SizeType
OOMArray<TValue, TEnable>::replaceWithoutFlush(SizeType position, TSourceIterator srcBegin, const TSourceIterator & srcEnd) {
	SizeType offset = position;
	SizeType count = distance(srcBegin, srcEnd);

	//set buffer size to be between sizeof(value_type) and 1 GiB
	SizeType myBufferSize = std::max<SizeType>(1, std::min<SizeType>(std::min<SizeType>(m_backBufferSize, (1024*1024*1024)/sizeof(value_type)), count));
	TValue * myBuffer = new TValue[myBufferSize];
	
	if (!myBuffer) {
		throw sserialize::IOException("OOMArray::replace: could not allocate memory for buffer");
	}
	
	TValue * myBufferEnd = myBuffer+myBufferSize;
	while (srcBegin != srcEnd) {
		auto bufIt = myBuffer;
		for(; srcBegin != srcEnd && bufIt < myBufferEnd; ++srcBegin, ++bufIt) {
			*bufIt = *srcBegin;
		}
		SizeType writeSize = sizeof(TValue)*(bufIt-myBuffer);
		FileHandler::pwrite(m_fd, myBuffer, writeSize, offset*sizeof(TValue));
		offset += (bufIt-myBuffer);
	}
	delete[] myBuffer;
	SSERIALIZE_CHEAP_ASSERT_EQUAL(position+count, offset);
	
	return offset;
}

template<typename TValue, typename TEnable>
typename OOMArray<TValue, TEnable>::SizeType
OOMArray<TValue, TEnable>::replaceWithoutFlush(SizeType position, TValue const * srcBegin, TValue const *srcEnd) {
	std::size_t writeSize = sizeof(TValue)*std::distance(srcBegin, srcEnd);
	FileHandler::pwrite(m_fd, srcBegin, writeSize, position*sizeof(TValue));
	return position+writeSize;
}

template<typename TValue, typename TEnable>
typename OOMArray<TValue, TEnable>::iterator
OOMArray<TValue, TEnable>::move(iterator srcBegin, const iterator & srcEnd, iterator destBegin) {
	OOMArray * src = srcBegin.d();
	OOMArray * dest = destBegin.d();
	SSERIALIZE_CHEAP_ASSERT(src);
	SSERIALIZE_CHEAP_ASSERT(dest);
	
	//move optimization for files with src >= dest and same mmt
	if (src->size() >= dest->size() &&
		srcBegin.p() == 0 && srcEnd.p() == src->size() &&
		destBegin.p() == 0 &&
		src->m_mmt == dest->m_mmt &&
		(
			src->m_mmt != sserialize::MM_FILEBASED ||
			sserialize::MmappedFile::deviceId(src->m_fd) == sserialize::MmappedFile::deviceId(dest->m_fd) ||
			sserialize::MmappedFile::dirName(src->m_fn) == sserialize::MmappedFile::dirName(src->m_fn)
		)
	)
	{
		src->swap_data(*dest);
		return dest->end();
	}
	else {
		return dest->replace(destBegin, std::move(srcBegin), srcEnd);
	}
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
	sserialize::detail::OOMArray::Iterator<TValue> srcBegin,
	const sserialize::detail::OOMArray::Iterator<TValue> & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::copy(std::move(srcBegin), srcEnd, targetBegin);
}

///Move into OOMArray, this invalidates other iterators that have the range within their cache
template<typename TValue>
sserialize::detail::OOMArray::Iterator<TValue>
move(
	sserialize::detail::OOMArray::Iterator<TValue> srcBegin,
	const sserialize::detail::OOMArray::Iterator<TValue> & srcEnd,
	const sserialize::detail::OOMArray::Iterator<TValue> & targetBegin)
{
	return sserialize::OOMArray<TValue>::move(std::move(srcBegin), srcEnd, targetBegin);
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
