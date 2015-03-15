#ifndef SSERIALIZE_DATA_SET_FACTORY_H
#define SSERIALIZE_DATA_SET_FACTORY_H
#include <forward_list>
#include <unordered_map>
#include <iostream>
#include <sserialize/utility/MultiReaderSingleWriterLock.h>
#include <sserialize/templated/MMVector.h>
#include <sserialize/Static/Array.h>

namespace sserialize {

/** This class is a storage for arbitrary data sets. If you add the same the multiple times, then it get's store only once.
	Compile-time debug options:
	DEBUG_CHECK_SERIALIZED_INDEX, DEBUG_CHECK_ALL
*/

class DataSetFactory {
public:
	typedef std::forward_list<uint32_t> DataOffsetContainer;
	typedef std::unordered_map< uint64_t, DataOffsetContainer > DataHashType;
private:
	UByteArrayAdapter m_data;
	Static::ArrayCreator<UByteArrayAdapter> m_ac;
	DataHashType m_hash;
	std::atomic<uint32_t> m_hitCount;
	MultiReaderSingleWriterLock m_mapLock;
	MultiReaderSingleWriterLock m_dataLock;
	
	uint64_t hashFunc(const UByteArrayAdapter & v);
	///returns the position of the data or -1 if none was found @thread-safety: yes
	int64_t getStoreId(const UByteArrayAdapter & v, uint64_t & hv);
	bool dataInStore(const UByteArrayAdapter & v, uint32_t id);

private://deleted functions
	DataSetFactory(const DataSetFactory & /*other*/);
	DataSetFactory & operator=(const DataSetFactory & /*other*/);
public:
	DataSetFactory(bool memoryBase = false);
	DataSetFactory(DataSetFactory && other);
	~DataSetFactory();
	DataSetFactory & operator=(DataSetFactory && other);

	///create the DataSetStore at the beginning of data
	void setDataStoreFile(UByteArrayAdapter data);


	uint32_t size() const { return m_ac.size();}
	inline uint32_t hitCount() const { return m_hitCount; }

	UByteArrayAdapter at(uint32_t id) const;

	uint32_t insert(const sserialize::UByteArrayAdapter & data);
	uint32_t insert(const std::vector<uint8_t> & data);
	
	///Flushes the data, don't add data afterwards
	///@return number of bytes from the beginning of the dataStoreFile
	///Flushed to an Static::Array
	OffsetType flush();
	
	UByteArrayAdapter getFlushedData() const { return m_ac.getFlushedData(); }
};

}//end namespace

#endif
