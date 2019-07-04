#ifndef SSERIALIZE_VARIANT_STORE_H
#define SSERIALIZE_VARIANT_STORE_H
#include <forward_list>
#include <unordered_map>
#include <iostream>
#include <sserialize/mt/MultiReaderSingleWriterLock.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/Static/Array.h>

namespace sserialize {

/** This class is a storage for arbitrary data sets.
	You can enable deduplication on a per-insertion basis
	Current version flushes to a simple sserialize::Static::Array
	Future versions may implement compression, though this can already be accomplished by adding the compressed data.
*/

class VariantStore {
public:
	typedef sserialize::UByteArrayAdapter value_type;
	typedef uint64_t SizeType;
	typedef SizeType IdType;
	typedef enum {DDM_FORCE_OFF, DDM_FORCE_ON, DDM_DEFAULT} DeduplicationMode; 
public:
	///create the DataSetStore at dest.tellPutPtr()
	VariantStore(sserialize::UByteArrayAdapter dest, sserialize::MmappedMemoryType mmt);
	VariantStore(sserialize::MmappedMemoryType mmt = sserialize::MM_PROGRAM_MEMORY);
	VariantStore(VariantStore && other);
	VariantStore(const VariantStore &) = delete;
	~VariantStore();
	VariantStore & operator=(VariantStore && other);
	VariantStore & operator=(const VariantStore &) = delete;
	
	void setDDM(bool useDeduplication);

	SizeType size() const;
	SizeType hitCount() const;

	UByteArrayAdapter at(IdType id) const;

	///thread-safe, @param ddm user default or force on/off
	IdType insert(const sserialize::UByteArrayAdapter & data, DeduplicationMode ddm = DDM_DEFAULT);
	
	///Flushes the data, don't add data afterwards
	///@return number of bytes from the beginning of the dataStoreFile
	///Flushed to an Static::Array
	void flush();
	
	UByteArrayAdapter getFlushedData() const;
private:
	typedef uint64_t HashValue;
	struct HashListEntry {
		HashListEntry() : HashListEntry(std::numeric_limits<uint64_t>::max(), std::numeric_limits<IdType>::max()) {}
		HashListEntry(uint64_t prev, IdType id) : prev(prev), id(id) {}
		uint64_t prev;
		IdType id;
	} __attribute__ ((packed));
	typedef std::unordered_map<HashValue, uint64_t> DataHash;//maps from HashValue -> HashList position
	typedef sserialize::MMVector<HashListEntry> HashList;
	typedef sserialize::Static::detail::ArrayCreator::DefaultStreamingSerializer<value_type> Serializer;
	typedef sserialize::Static::ArrayCreator<UByteArrayAdapter, Serializer, sserialize::MMVector<uint64_t> > ArrayCreator;
	static constexpr IdType nid = std::numeric_limits<IdType>::max();
private:

	HashValue hashFunc(const UByteArrayAdapter::MemoryView & v);
	bool dataInStore(const UByteArrayAdapter::MemoryView & v, IdType id);
	
	///returns the id of the data or npos if none was found @thread-safety: yes
	IdType getStoreId(const UByteArrayAdapter::MemoryView & v, HashValue hv);

private:
	UByteArrayAdapter m_data;
	ArrayCreator m_ac;
	DataHash m_hash;
	HashList m_hashList;
	std::atomic<uint64_t> m_hitCount{0};
	MultiReaderSingleWriterLock m_hashLock;
	MultiReaderSingleWriterLock m_dataLock;
	DeduplicationMode m_ddm{DDM_FORCE_ON};
};

}//end namespace

#endif
