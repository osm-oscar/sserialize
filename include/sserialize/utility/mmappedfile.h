#ifndef SSERIALIZE_MMAPPEDFILE_H
#define SSERIALIZE_MMAPPEDFILE_H
#include <string>
#include <stdint.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/types.h>
#include <sys/stat.h>

namespace sserialize {

class MmappedFilePrivate: public RefCountObject {
private:
	std::string m_fileName;
	OffsetType m_exposedSize;
	OffsetType m_realSize;
	int m_fd;
	uint8_t * m_data;
	bool m_writable;
	bool m_deleteOnClose;
	bool m_syncOnClose;
public:
	MmappedFilePrivate(std::string filename);
	MmappedFilePrivate();
	~MmappedFilePrivate();
	bool do_open();
	bool do_close();
	bool do_sync();
	bool valid();
	bool resizeRounded(OffsetType size);
	bool resize(OffsetType size);
	inline OffsetType size() const { return m_exposedSize; }
	inline const std::string & fileName() const { return m_fileName;}
	inline uint8_t operator[](OffsetType pos) { return ( (pos < m_exposedSize) ? m_data[pos] : 0); }
	bool read(uint8_t * buffer, uint32_t len, OffsetType displacement);
	inline uint8_t * data() { return m_data; }
	void setFileName(std::string fileName) { m_fileName = fileName; }
	void setWriteableFlag(bool writable);
	void setDeleteOnClose(bool deleteOnClose);
	void setSyncOnClose(bool syncOnClose);
	
	///returns 0 on error and a instance of MmappedFilePrivate on success
	static MmappedFilePrivate* createTempFile(const std::string & fileNameBase, UByteArrayAdapter::OffsetType size);
};

class MmappedFile: public RCWrapper<MmappedFilePrivate> {
private:
	explicit MmappedFile(MmappedFilePrivate * priv) : RCWrapper<MmappedFilePrivate>(priv) {}
public:
	MmappedFile() : RCWrapper<MmappedFilePrivate>(new MmappedFilePrivate()) {
		if (priv()) {
			priv()->setWriteableFlag(false);
		}
		//TODO: Else: throw exception (but android does not support exception handling?)
	}
	
	explicit MmappedFile(bool writable) : RCWrapper<MmappedFilePrivate>(new MmappedFilePrivate()) {
		if (priv()) {
			priv()->setWriteableFlag(writable);
		}
		//TODO: Else: throw exception (but android does not support exception handling?)
	}
	
	
	MmappedFile(const MmappedFile & other) : RCWrapper<MmappedFilePrivate>(other) {}
	
	explicit MmappedFile(const char * fileName, bool writable=false) : RCWrapper<MmappedFilePrivate>(new MmappedFilePrivate(std::string(fileName))) {
		if (priv()) {
			priv()->setWriteableFlag(writable);
		}
		//TODO: Else: throw exception (but android does not support exception handling?)
	}
	
	MmappedFile(const std::string & fileName, bool writable=false) : RCWrapper<MmappedFilePrivate>(new MmappedFilePrivate(fileName)) {
		if (priv()) {
			priv()->setWriteableFlag(writable);
		}
		//TODO: Else: throw exception (but android does not support exception handling?)
	}
	
	MmappedFile & operator=(const MmappedFile & other) {
		RCWrapper<MmappedFilePrivate>::operator=(other);
		return *this;
	}

	~MmappedFile() {}
	inline bool open() { return priv()->do_open(); }
	inline bool close() { return priv()->do_close(); }
	inline bool sync() { return priv()->do_sync(); }
	inline bool valid() { return priv()->valid(); }
	inline OffsetType size() const { return priv()->size(); }
	inline const std::string & fileName() const { return priv()->fileName(); }
	inline uint8_t operator[](OffsetType pos) { return priv()->operator[](pos); }
	inline bool read(uint8_t * buffer, uint32_t len, OffsetType displacement=0) { return priv()->read(buffer, len, displacement); }
	inline uint8_t * data() { return priv()->data(); }
	inline UByteArrayAdapter dataAdapter() {
		return UByteArrayAdapter(*this);
	}
	inline void setFileName(std::string fileName) {
		if (privRc() == 1 && priv()->fileName().size() == 0) {
			priv()->setFileName(fileName);
		}
		else {
			setPrivate(new MmappedFilePrivate(fileName));
		}
	}
	inline void setDeleteOnClose(bool deleteOnClose) { priv()->setDeleteOnClose(deleteOnClose); }
	inline void setSyncOnClose(bool syncOnClose) { return priv()->setSyncOnClose(syncOnClose);}
	/** resizes the file to size bytes. All former data references are invalid after this */
	inline bool resize(OffsetType size) { return priv()->resizeRounded(size);}

public:
	/** creates a file with at least 1 byte */
	static bool createFile(const std::string & fileName, OffsetType size);
	///creates a cache file that gets automatically deleted
	static bool createCacheFile(OffsetType size, sserialize::MmappedFile & dest);
	static bool truncateFile(const std::string & fileName, OffsetType size);
	static bool fileExists(const std::string & fileName);
	static std::size_t fileSize(const std::string & fileName);
	/** Not thread-safe **/
	static std::string findLockFilePath(const std::string & fileNamePrefix, uint32_t maxTest);
	
	///Thread-safe
	static bool createTempFile(const std::string & fileNameBase, UByteArrayAdapter::OffsetType size, MmappedFile & dest);
	
	static bool unlinkFile(const std::string & fileName);
	static bool isDirectory(const std::string & fileName);
	///Tries to create a directory at fileName. Alsoreturns true if the dir already existed
	static bool createDirectory(const std::string & fileName, __mode_t mode = S_IRWXU);
	static bool createSymlink(const std::string & src, const std::string & destination);
	static std::string realPath(const std::string & path);
};

}//end namespace

#endif