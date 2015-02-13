#ifndef SSERIALIZE_DATA_SET_FACTORY_H
#define SSERIALIZE_DATA_SET_FACTORY_H
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/utilcontainerfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/types.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/MultiReaderSingleWriterLock.h>

namespace sserialize {

/** This class is a storage for arbitrary data sets. If you add the same the multiple times, then it get's store only once.
	Compile-time debug options:
	DEBUG_CHECK_SERIALIZED_INDEX, DEBUG_CHECK_ALL
*/

class DataSetFactory {
public:
	typedef std::forward_list<OffsetType> DataOffsetContainer;
	typedef std::unordered_map< uint64_t, std::forward_list<OffsetType> > DataHashType;
	typedef std::unordered_map< uint64_t, uint32_t > OffsetToIdHashType;
	typedef std::vector<uint64_t> IdToOffsetsType;
private:
	UByteArrayAdapter m_header;
	UByteArrayAdapter m_dataStore;
	DataHashType m_hash;
	OffsetToIdHashType m_offsetsToId;
	IdToOffsetsType m_idToOffsets;
	std::atomic<uint32_t> m_hitCount;
	MultiReaderSingleWriterLock m_mapLock;
	MultiReaderSingleWriterLock m_dataLock;
	
	uint64_t hashFunc(const UByteArrayAdapter & v);
	///returns the position of the data or -1 if none was found @thread-safety: yes
	int64_t getStoreOffset(const UByteArrayAdapter & v, uint64_t & hv);
	bool dataInStore(const UByteArrayAdapter & v, uint64_t offset);

private://deleted functions
	DataSetFactory(const DataSetFactory & /*other*/) {}
	DataSetFactory & operator=(const DataSetFactory & /*other*/) { return *this;}
public:
	DataSetFactory(bool memoryBase = false);
	DataSetFactory(DataSetFactory && other);
	~DataSetFactory();
	DataSetFactory & operator=(DataSetFactory && other);

	///create the DataSetStore at the beginning of data
	void setDataStoreFile(UByteArrayAdapter data);


	uint32_t size() const { return m_idToOffsets.size();}
	inline uint32_t hitCount() const { return m_hitCount; }

	UByteArrayAdapter at(uint32_t id) const;

	uint32_t insert(const sserialize::UByteArrayAdapter & data);
	uint32_t insert(const std::vector<uint8_t> & data);
	
	UByteArrayAdapter getFlushedData();
	
	///Flushes the data, don't add data afterwards
	///@return number of bytes from the beginning of the dataStoreFile
	///Flushed to an Static::Array
	OffsetType flush();
};

}//end namespace

#endif
