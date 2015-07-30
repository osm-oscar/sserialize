#ifndef ITEM_INDEX_PRIVATE_H
#define ITEM_INDEX_PRIVATE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/iterator/AtStlInputIterator.h>

namespace sserialize {

class ItemIndexPrivate: public RefCountObject {
public:
	typedef detail::AbstractArrayIterator<uint32_t> const_iterator_base_type;
	typedef detail::AbstractArrayIterator<uint32_t> * const_iterator;
	typedef const_iterator iterator;
public:
	ItemIndexPrivate();
	virtual ~ItemIndexPrivate();
	virtual UByteArrayAdapter data() const;
	
	virtual ItemIndex::Types type() const = 0;
	
	virtual int find(uint32_t id) const;
protected:
	///Convinience unite which uses a ItemIndexPrivateSimple as storage backend
	ItemIndexPrivate * doIntersect(const sserialize::ItemIndexPrivate * other) const;
	///Convinience unite which uses a ItemIndexPrivateSimple as storage backend
	ItemIndexPrivate * doUnite(const sserialize::ItemIndexPrivate * other) const;
	///Convinience difference which uses a ItemIndexPrivateSimple as storage backend
	ItemIndexPrivate * doDifference(const sserialize::ItemIndexPrivate * other) const;
	///Convinience symmetricDifference which uses a ItemIndexPrivateSimple as storage backend
	ItemIndexPrivate * doSymmetricDifference(const sserialize::ItemIndexPrivate * other) const;
	
	void doPutInto(DynamicBitSet & bitSet) const;
	void doPutInto(uint32_t* dest) const;
	
public:
	///load all data into memory (only usefull if the underlying storage is not contigous)
	virtual void loadIntoMemory();

	///uses at(pos) by default
	virtual uint32_t uncheckedAt(uint32_t pos) const;
	///checked at
	virtual uint32_t at(uint32_t pos) const = 0;
	virtual uint32_t first() const = 0;
	virtual uint32_t last() const = 0;
	
	virtual const_iterator cbegin() const;
	virtual const_iterator cend() const;

	virtual uint32_t size() const = 0;

	///return the mean bit per number including header
	virtual uint8_t bpn() const = 0;
	
	virtual uint32_t getSizeInBytes() const = 0;

	virtual bool is_random_access() const;

	virtual void putInto(DynamicBitSet & bitSet) const;
	virtual void putInto(uint32_t* dest) const;


	///Default uniteK uses unite
	virtual ItemIndexPrivate * uniteK(const sserialize::ItemIndexPrivate * other, uint32_t numItems) const;
	///Default intersect which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * intersect(const sserialize::ItemIndexPrivate * other) const;
	///Default unite which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * unite(const sserialize::ItemIndexPrivate * other) const;
	///Default difference which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * difference(const sserialize::ItemIndexPrivate * other) const;
	///Default symmetricDifference which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * symmetricDifference(const sserialize::ItemIndexPrivate * other) const;
};

class ItemIndexPrivateEmpty: public ItemIndexPrivate {
public:
	ItemIndexPrivateEmpty();
	virtual ~ItemIndexPrivateEmpty();
	virtual ItemIndex::Types type() const;

	virtual int find(uint32_t id) const;

public:
	virtual uint32_t at(uint32_t pos) const;
	virtual uint32_t first() const;
	virtual uint32_t last() const;
	
	virtual uint32_t size() const;

	virtual uint8_t bpn() const;
	virtual double entropy() const;

	virtual uint32_t getSizeInBytes() const;

};


///Wrapper for Indirect index, ItemIndexPrivate has to be an accessible base of T_BASE_CLASS
template<typename T_DATA, class T_BASE_CLASS>
class ItemIndexPrivateIndirectWrapper: public T_BASE_CLASS {
protected:
	ItemIndex m_realIdIndex;
	ItemIndex & realIdIndex() { return m_realIdIndex; }
public:
	ItemIndexPrivateIndirectWrapper() : T_BASE_CLASS() {}
	ItemIndexPrivateIndirectWrapper(const T_DATA & data, const ItemIndex & realIdIndex) : T_BASE_CLASS(data), m_realIdIndex(realIdIndex) {}
	virtual ~ItemIndexPrivateIndirectWrapper() {}
	
	///uses at(pos) by default
	virtual uint32_t uncheckedAt(uint32_t pos) const {
		uint32_t id = T_BASE_CLASS::uncheckedAt(pos);
		return m_realIdIndex.at(id);
	}
	
	virtual uint32_t at(uint32_t pos) const {
		uint32_t id = T_BASE_CLASS::at(pos);
		return m_realIdIndex.at(id);
	}

	virtual uint32_t first() const {
		uint32_t id = T_BASE_CLASS::first();
		return m_realIdIndex.at(id);
	}

	virtual uint32_t last() const {
		uint32_t id = T_BASE_CLASS::last();
		return m_realIdIndex.at(id);
	}
	
	using T_BASE_CLASS::putInto;
	
	virtual void putInto(DynamicBitSet & bitSet) const {
		ItemIndexPrivate::doPutInto(bitSet);
	}
	
	///Default intersect which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * doIntersect(const sserialize::ItemIndexPrivate * other) const {
		return ItemIndexPrivate::intersect(other);
	}
	
	///Default unite which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * doUnite(const sserialize::ItemIndexPrivate * other) const {
		return ItemIndexPrivate::unite(other);
	}
	
	///Default difference which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * doDifference(const sserialize::ItemIndexPrivate * other) const {
		return ItemIndexPrivate::difference(other);
	}
	
	///Default symmetricDifference which uses a ItemIndexPrivateSimple as storage backend
	virtual ItemIndexPrivate * doSymmetricDifference(const sserialize::ItemIndexPrivate * other) const {
		return ItemIndexPrivate::symmetricDifference(other);
	}
	
};

namespace detail {
namespace ItemIndexImpl {

struct IntersectOp {
	static constexpr bool pushFirstSmaller = false;
	static constexpr bool pushEqual = true;
	static constexpr bool pushSecondSmaller = false;
	static constexpr bool pushFirstRemainder = false;
	static constexpr bool pushSecondRemainder = false;
	static inline uint32_t maxSize(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		return std::min<uint32_t>(first->size(), second->size());
	}
};

struct UniteOp {
	static constexpr bool pushFirstSmaller = true;
	static constexpr bool pushEqual = true;
	static constexpr bool pushSecondSmaller = true;
	static constexpr bool pushFirstRemainder = true;
	static constexpr bool pushSecondRemainder = true;
	static inline uint32_t maxSize(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		return first->size() + second->size();
	}
};

struct DifferenceOp {
	static constexpr bool pushFirstSmaller = true;
	static constexpr bool pushEqual = false;
	static constexpr bool pushSecondSmaller = false;
	static constexpr bool pushFirstRemainder = true;
	static constexpr bool pushSecondRemainder = false;
	static inline uint32_t maxSize(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* /*second*/) {
		return first->size();
	}
};


struct SymmetricDifferenceOp {
	static constexpr bool pushFirstSmaller = true;
	static constexpr bool pushEqual = false;
	static constexpr bool pushSecondSmaller = true;
	static constexpr bool pushFirstRemainder = true;
	static constexpr bool pushSecondRemainder = true;
	static inline uint32_t maxSize(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		return first->size() + second->size();
	}
};

}}//end namespace detail::ItemIndex

}//end namespace

#endif