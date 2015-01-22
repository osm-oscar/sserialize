#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_RLE_DE_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_RLE_DE_H
#include "ItemIndexPrivate.h"
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/UDWConstrainedIterator.h>

namespace sserialize {

/** This implements a difference run-length encoded index
  *
  *--------------------------------------------------------------------------------------
  * SIZE |COUNT |DATA
  *--------------------------------------------------------------------------------------
  *uint32|uint32|*
  *--------------------------------------------------------------------------------------
  * SIZE is the size of the DATA section
  * COUNT is the number of elements
  * Data is delta encoded with rle. Type of word is selected by the least significant bit.
  * If data[i] & 0x1 then data is a rle and the next varuint32 tells the difference between the (data[i] >> 1) elements
  *
  *
  */
  
class ItemIndexPrivateRleDE;
  
class ItemIndexPrivateRleDECreator {
public:
	UByteArrayAdapter & m_data;
	uint32_t m_beginning;
	uint32_t m_rle;
	uint32_t m_lastDiff;
	uint32_t m_count;
	uint32_t m_prev;
private:
	void flushRle() {
		if (m_rle) {
			if (m_rle == 1) { //no rle
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
			else {
				m_data.putVlPackedUint32((m_rle << 1) | 0x1);
				m_data.putVlPackedUint32(m_lastDiff << 1);
			}
		}
		m_rle = 0;
	}
	void flushHeader() {
		m_data.putUint32(m_beginning, m_data.tellPutPtr() - (m_beginning + 8));
		m_data.putUint32(m_beginning+4, m_count);
	}
public:
	ItemIndexPrivateRleDECreator(UByteArrayAdapter & data) :
	m_data(data), m_beginning(data.tellPutPtr()), m_rle(0), m_lastDiff(0), m_count(0), m_prev(0) {
		m_data.putUint32(0);
		m_data.putUint32(0);
	}
	virtual ~ItemIndexPrivateRleDECreator() {}
	///push only in ascending order (id need to be unique and larger than the one before! otherwise this will eat your kitten!
	void push_back(uint32_t id) {
		uint32_t diff = id - m_prev;
		if (diff == m_lastDiff) {
			++m_rle;
		}
		else {
			if (m_rle) {
				if (m_rle == 1) { //no rle
					m_data.putVlPackedUint32(m_lastDiff << 1);
				}
				else {
					m_data.putVlPackedUint32((m_rle << 1) | 0x1);
					m_data.putVlPackedUint32(m_lastDiff << 1);
				}
			}
			m_rle = 1;
			m_lastDiff = diff;
		}
		m_prev = id;
		++m_count;
	}
	
	///Does a flush and appends the data which has countInData elements (mainly used by set functions)
	void flushWithData(const UByteArrayAdapter & appendData, uint32_t countInData) {
		flushRle();
		m_data.put(appendData);
		m_count += countInData;
		flushHeader();
	}
	///you need to call this prior to using toIndex() or using the data
	///you should not push after calling ths function
	void flush() {
		flushRle();
		flushHeader();
	}

	ItemIndex getIndex() {
		return ItemIndex(UByteArrayAdapter(m_data, m_beginning), ItemIndex::T_RLE_DE);
	}
	
	ItemIndexPrivate * getPrivateIndex();
};

class ItemIndexPrivateRleDE: public ItemIndexPrivate {
public:
	typedef ItemIndexPrivate MyBaseClass;
private:
	class MyIterator: public ItemIndexPrivate::const_iterator_base_type {
	private:
		const ItemIndexPrivateRleDE * m_parent;
		UByteArrayAdapter::OffsetType m_dataOffset;
		uint32_t m_curRleCount;
		uint32_t m_curRleDiff;
		uint32_t m_curId;
	private:
		MyIterator(const MyIterator & other);
		void fetchNext();
	public:
		MyIterator(const ItemIndexPrivateRleDE * parent, UByteArrayAdapter::OffsetType dataOffset);
		virtual ~MyIterator();
		virtual uint32_t get() const override;
		virtual void next() override;
		virtual bool notEq(const ItemIndexPrivate::const_iterator_base_type * other) const override;
		virtual ItemIndexPrivate::const_iterator_base_type * copy() const override;
	};
private:
	UByteArrayAdapter m_data;
	uint32_t m_size;
	mutable uint32_t m_dataOffset;
	mutable uint32_t m_curId;
	mutable std::vector<uint32_t> m_cache;
public:
	ItemIndexPrivateRleDE();
	ItemIndexPrivateRleDE(const UByteArrayAdapter & data);
	ItemIndexPrivateRleDE(const UDWIterator & data);
	virtual ~ItemIndexPrivateRleDE();
	virtual ItemIndex::Types type() const;
	
	virtual int find(uint32_t id) const;

public:
	virtual UByteArrayAdapter data() const;

	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;

	virtual MyBaseClass::const_iterator cbegin() const override;
	virtual MyBaseClass::const_iterator cend() const override;

	virtual uint32_t size() const;

	virtual uint8_t bpn() const;


	virtual uint32_t getSizeInBytes() const;
	
	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t * dest) const override;
	
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);



	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const;

	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex fusedIntersectDifference(const std::vector< ItemIndexPrivateRleDE* > & intersect, const std::vector< ItemIndexPrivateRleDE* >& subtract, uint32_t count);


	///The result of this operations uses a memory-based backend
	///none of the indices has to be empty!
	static ItemIndex constrainedIntersect(const std::vector< ItemIndexPrivateRleDE* > & intersect, uint32_t count);


public:
	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest) {
		ItemIndexPrivateRleDECreator creator(dest);
		typename TCONTAINER::const_iterator srcIt = src.begin();
		typename TCONTAINER::const_iterator srcEnd = src.end();
		for(; srcIt != srcEnd; ++srcIt) {
			creator.push_back(*srcIt);
		}
		creator.flush();
		return true;
	}
};

typedef ItemIndexPrivateIndirectWrapper<UByteArrayAdapter, ItemIndexPrivateRleDE> ItemIndexPrivateRleDEIndirect;

}//end namespace
#endif