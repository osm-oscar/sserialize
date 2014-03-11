#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_SIMPLE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_SIMPLE_H
#include "ItemIndexPrivate.h"
#include <sserialize/utility/CompactUintArray.h>


/* File format:
 *---------------------------------------------------------------------------------
 *COUNTIDB|FIRSTID|CompactUintArray
 *---------------------------------------------------------------------------------
 *   4 B  |  4 B  |IDB*
 *
 * COUNT = number of Ids
 * IDB = 2 bits to select if bits are encoded in 8,16,24 or 32 bits each
 * CompactUintArray = id store
 */
#define ITEM_INDEX_PRIVATE_SIMPLE_HEADER_SIZE 8
 
 
namespace sserialize {

class ItemIndexPrivateSimple;

class ItemIndexPrivateSimpleCreator {
private:
	UByteArrayAdapter & m_destination;
	uint32_t m_beginning;
	uint32_t m_lowestId;
	CompactUintArray m_carr;
	uint32_t m_pos;
public:
	/** creates a new ItemIndexPrivateSimple at the beginning of destination */
	ItemIndexPrivateSimpleCreator(uint32_t lowestId, uint32_t highestId, uint32_t reserveCount, UByteArrayAdapter & destination);
	~ItemIndexPrivateSimpleCreator() {}
	void push_back(uint32_t id);
	
	inline uint32_t size() const { return m_pos; }
	
	/** This needs to be called before you can user the data */
	void flush();
	
	ItemIndexPrivate * getPrivateIndex();
	
	ItemIndex getIndex() const;
	
	ItemIndexPrivateSimple * privateIndex();
	
	static UByteArrayAdapter createCache(uint32_t lowestId, uint32_t highestId, uint32_t maxCount, bool forceFileBase);
};
 
class ItemIndexPrivateSimple: public ItemIndexPrivate {
protected:
	CompactUintArray m_idStore;
	uint32_t m_size;
	uint8_t m_bpn;
	int32_t m_yintercept; 

public:
	ItemIndexPrivateSimple();
	ItemIndexPrivateSimple(const UByteArrayAdapter & index);
	virtual ~ItemIndexPrivateSimple();
	virtual ItemIndex::Types type() const;
public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;

	virtual uint32_t size() const;

	virtual uint8_t bpn() const;
	virtual uint32_t slopenom() const;
	virtual int32_t yintercept() const;
	virtual uint32_t idOffSet() const;


	virtual uint32_t rawIdAt(const uint32_t pos) const;
	virtual uint32_t getSizeInBytes() const;
	virtual uint32_t getHeaderbytes() const;
	virtual uint32_t getRegressionLineBytes() const;
	virtual uint32_t getIdBytes() const;

	virtual void putInto(std::vector<uint32_t> & dest) const;

public:
	static bool addItemIndexFromIds(const std::set<uint32_t> & ids, UByteArrayAdapter & adap);
	/** adds the header to the beginning of adap */
	static void addHeader(uint32_t count, uint8_t bytesPerId, uint32_t lowestId, UByteArrayAdapter& adap);
	static uint32_t storageSize(uint32_t count, uint8_t bytesPerId);
	static uint32_t headerSize();
	static CompactUintArray initStorage(const sserialize::UByteArrayAdapter& destination, uint8_t bytesPerId);
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	
	template<typename TCONTAINER>
	static void create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		if (src.size()) {
			ItemIndexPrivateSimpleCreator creator(*src.begin(), *src.rbegin(), src.size(), dest);
			typename TCONTAINER::const_iterator srcIt = src.begin();
			typename TCONTAINER::const_iterator srcEnd = src.end();
			for(; srcIt != srcEnd; ++srcIt)
				creator.push_back(*srcIt);
			creator.flush();
		}
		else {
			dest.putUint32(0);
		}
	}
};

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateSimple> ItemIndexPrivateSimpleIndirect; 

}//end namespace

#endif