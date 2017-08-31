#ifndef SSERIALIZE_ITEM_INDEX_PRIVATE_ELIAS_FANO_H
#define SSERIALIZE_ITEM_INDEX_PRIVATE_ELIAS_FANO_H
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/iterator/UnaryCodeIterator.h>

namespace sserialize {

class ItemIndexPrivateEliasFano;

namespace detail {
namespace ItemIndexImpl {
	class EliasFanoCreator;

	class EliasFanoIterator: public detail::AbstractArrayIterator<uint32_t> {
	public:
		using MyBaseClass = detail::AbstractArrayIterator<uint32_t>;
	public:
		virtual ~EliasFanoIterator() override;
	public:
		virtual value_type get() const override;
		virtual void next() override;
		virtual bool notEq(const MyBaseClass * other) const override;
		virtual bool eq(const MyBaseClass * other) const override;
		virtual MyBaseClass * copy() const override;
	private:
		friend class EliasFanoCreator;
		friend class sserialize::ItemIndexPrivateEliasFano;
	private:
		EliasFanoIterator(const CompactUintArray::const_iterator & lb, const UnaryCodeIterator & ub, uint32_t lastUb, uint8_t numLowerBits);
		EliasFanoIterator(const CompactUintArray::const_iterator & lb, const UnaryCodeIterator & ub);
		EliasFanoIterator(const CompactUintArray::const_iterator & lb);
	private:
		const CompactUintArray::const_iterator & lowerBits() const;
		const UnaryCodeIterator & upperBits() const;
	private:
		CompactUintArray::const_iterator m_lb;
		UnaryCodeIterator m_ub;
		uint32_t m_lastUb;
		uint8_t m_numLowerBits;
	};

class EliasFanoCreator final {
public:
	EliasFanoCreator(const EliasFanoCreator& other) = delete;
	EliasFanoCreator & operator=(const EliasFanoCreator & other) = delete;
public:
	EliasFanoCreator(uint32_t maxId);
	EliasFanoCreator(UByteArrayAdapter & data, uint32_t maxId);
	EliasFanoCreator(EliasFanoCreator && other);
	~EliasFanoCreator();
	uint32_t size() const;
	void push_back(uint32_t id);
	void flush();
	
	UByteArrayAdapter flushedData() const;
	///flush needs to be called before
	ItemIndex getIndex();
	///flush needs to be called before
	sserialize::ItemIndexPrivate * getPrivateIndex();
private:
	UByteArrayAdapter & data();
	const UByteArrayAdapter & data() const;
private:
	std::vector<uint32_t> m_values;
	UByteArrayAdapter * m_data;
	sserialize::UByteArrayAdapter::OffsetType m_putPtr;
	bool m_delete;
};


}} //end namespace detail::ItemIndexImpl

/** Default format is:
  *
  * -------------------------------------------------------------
  * COUNT|MAX |DATA SIZE|LOWER BITS      |UPPER BITS
  * -------------------------------------------------------------
  * vu32 |vu32|vu32     |CompactUintArray|UnaryCodeStream
  * -------------------------------------------------------------
  * 
  * where
  * COUNT is the number of entries
  * MAX is the maximum element
  * DATA SIZE is the size of the UnaryCodeStream
  * 
  **/

class ItemIndexPrivateEliasFano: public ItemIndexPrivate {
public:
	ItemIndexPrivateEliasFano(const UByteArrayAdapter & d);
	ItemIndexPrivateEliasFano(uint32_t size, const CompactUintArray & lb, const UnaryCodeIterator & ub);
	virtual ~ItemIndexPrivateEliasFano();
	
	virtual ItemIndex::Types type() const override;
	
	virtual UByteArrayAdapter data() const;
public:
	uint32_t maxId() const;
public:
	///load all data into memory (only usefull if the underlying storage is not contigous)
	virtual void loadIntoMemory() override;

	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual const_iterator cbegin() const;
	virtual const_iterator cend() const;

	virtual uint32_t size() const override;
	
	virtual uint8_t bpn() const override;
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;

	virtual bool is_random_access() const override;

	virtual void putInto(DynamicBitSet & bitSet) const override;
	virtual void putInto(uint32_t* dest) const override;

	///Default uniteK uses unite
	virtual ItemIndexPrivate * uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const override;
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const override;
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const override;
public:
	static ItemIndexPrivate * fromBitSet(const DynamicBitSet & bitSet);
	template<typename T_ITERATOR>
	static void create(T_ITERATOR begin, T_ITERATOR end, UByteArrayAdapter & dest);
	static void create(const std::vector<uint32_t> & src, UByteArrayAdapter & dest);
	static uint8_t numLowerBits(uint32_t count, uint32_t max);
private:
	uint32_t dataSize() const;
	CompactUintArray lowerBits() const;
	UnaryCodeIterator upperBits() const;
	uint8_t numLowerBits() const;
private:
	UByteArrayAdapter m_d;
	uint32_t m_size;
	uint32_t m_maxIdBegin:10;
	uint32_t m_dataSizeBegin:10;
	uint32_t m_lowerBitsBegin:10;
	mutable AbstractArrayIterator<uint32_t> m_it;
	mutable std::vector<uint32_t> m_cache; //TODO: get rid of this using skip pointers?
};

}//end namespace

namespace sserialize {


template<typename T_ITERATOR>
void ItemIndexPrivateEliasFano::create(T_ITERATOR begin, T_ITERATOR end, UByteArrayAdapter & dest) {
	create(std::vector<uint32_t>(begin, end), dest);
}

}

#endif