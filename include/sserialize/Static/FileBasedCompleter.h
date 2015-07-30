#ifndef SSERIALIZE_STATIC_FILE_BASED_COMPLETER_H 
#define SSERIALIZE_STATIC_FILE_BASED_COMPLETER_H
#include <sserialize/storage/mmappedfile.h>
#include <sserialize/Static/Completer.h>
#include <sserialize/utility/types.h>

namespace sserialize {
namespace Static {

template<class ItemType, class DataBaseType, class CompleterBase = sserialize::Static::Completer<ItemType, DataBaseType> >
class FileBasedCompleter: public CompleterBase {
private:
	std::vector<std::string> m_strCompleterFileNames;
	std::string m_dataFileName;
	std::string m_stringTableFileName;
	std::string m_indexFileName;
	
	std::vector<bool> m_strCompleterFileCompression;
	bool m_dataFileCompression;
	bool m_stringTableFileCompression;
	bool m_indexFileCompression;

	std::string m_tempDirPath;

protected:
	bool openAll();
	void closeAll();

public:
	FileBasedCompleter();
	virtual ~FileBasedCompleter();
	/** @return true, if file exisists */
	bool addStringCompleterFile(const std::string & filename, bool compressed);
	inline void setIndexFile(const std::string & filename, bool compressed) {
		m_indexFileName = filename;
		m_indexFileCompression = compressed;
	}
	inline void setDataFile(const std::string & filename, bool compressed) {
		m_dataFileName = filename;
		m_dataFileCompression = compressed;
	}
	inline void setStringTableFile(const std::string & filename, bool compressed) {
		m_stringTableFileName = filename;
		m_stringTableFileCompression = compressed;
	}
	inline void setTempDirPath(const std::string & dirPath) { m_tempDirPath = dirPath;}

public:
	inline const std::string & getIndexFileName() const { return m_indexFileName; }
	inline const std::string & getDataFileName() const { return m_dataFileName;}
	inline const std::string & getStringTableFileName() const { return m_stringTableFileName;}
	bool energize();
	std::ostream& printStats(std::ostream & out) const {
		return CompleterBase::printStats(out);
	}
};

template<class ItemType, class DataBaseType, class CompleterBase>
bool FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::addStringCompleterFile(const std::string & filename, bool compressed) {
	if (MmappedFile::fileExists(filename)) {
		m_strCompleterFileNames.push_back(filename);
		m_strCompleterFileCompression.push_back(compressed);
		return true;
	}
	else {
		sserialize::err("FileBasedCompleter::addStringCompleterFile", "File " + filename + " is not available"); 
	}
	return false;
}

template<class ItemType, class DataBaseType, class CompleterBase>
bool FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::openAll() {
	
	UByteArrayAdapter dataAdapter(UByteArrayAdapter::openRo(m_dataFileName, m_dataFileCompression, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT));
	
	if (! dataAdapter.size() ) {
		sserialize::err("sserialize::FileBasedCompleter::openAll", "Failed to open data file: " + m_dataFileName);
		return false;
	}
	
	UByteArrayAdapter stringTableAdapter(UByteArrayAdapter::openRo(m_stringTableFileName, m_stringTableFileCompression, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT));
	
	if (! stringTableAdapter.size() ) {
		dataAdapter = UByteArrayAdapter();
		sserialize::err("sserialize::FileBasedCompleter::openAll", "Failed to open string table file: " + m_stringTableFileName);
		return false;
	}
	
	UByteArrayAdapter indexAdapter( UByteArrayAdapter::openRo(m_indexFileName, m_indexFileCompression, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT) );

	if (! indexAdapter.size()) {
		dataAdapter = UByteArrayAdapter();
		stringTableAdapter = UByteArrayAdapter();
		sserialize::err("sserialize::FileBasedCompleter::openAll", "Failed to open index file: " + m_indexFileName);
		return false;
	}
		


	CompleterBase::setIndexData(indexAdapter);
	CompleterBase::setDBData(dataAdapter);
	CompleterBase::setStringTableData(stringTableAdapter);

	for(size_t i = 0; i < m_strCompleterFileNames.size(); i++)  {
		UByteArrayAdapter strCompleterAdapter( UByteArrayAdapter::openRo(m_strCompleterFileNames[i], m_strCompleterFileCompression[i], MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT) );
		if (strCompleterAdapter.size()) {
			CompleterBase::addStringCompleterData(strCompleterAdapter);
		}
		else {
			sserialize::err("sserialize::FileBasedCompleter::openAll", "Failed to open completer file: " + m_strCompleterFileNames[i]);
		}
	}

	return true;
}

template<class ItemType, class DataBaseType, class CompleterBase>
void FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::closeAll() {
	CompleterBase::clear();
}

template<class ItemType, class DataBaseType, class CompleterBase>
FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::FileBasedCompleter() :
CompleterBase()
{}

template<class ItemType, class DataBaseType, class CompleterBase>
FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::~FileBasedCompleter() {
	closeAll();
}

template<class ItemType, class DataBaseType, class CompleterBase>
bool FileBasedCompleter<ItemType, DataBaseType, CompleterBase>::energize() {
	if (!openAll())
		return false;
	
	return CompleterBase::energize();
}

}}//end namespace

#endif