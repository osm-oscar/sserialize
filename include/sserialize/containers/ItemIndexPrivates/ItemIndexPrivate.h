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
	static constexpr uint32_t npos = 0xFFFFFFFF;
public:
	ItemIndexPrivate();
	virtual ~ItemIndexPrivate();
	virtual UByteArrayAdapter data() const;
	
	virtual ItemIndex::Types type() const = 0;
	
	virtual uint32_t find(uint32_t id) const;
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
	virtual ItemIndex::Types type() const override;

	virtual uint32_t find(uint32_t id) const override;

public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;

	virtual uint32_t getSizeInBytes() const override;

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