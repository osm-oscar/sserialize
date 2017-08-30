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
	
	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const = 0;

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

	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;

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

template<typename TPositionIterator>
struct GenericSetOpExecuterAccessors {
	typedef TPositionIterator PositionIterator;
	static PositionIterator begin(const sserialize::ItemIndexPrivate * idx);
	static PositionIterator end(const sserialize::ItemIndexPrivate * idx);
	static void next(PositionIterator & it);
	static bool unequal(const PositionIterator & first, const PositionIterator & second);
	static uint32_t get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it);
};

template<typename TCreator, typename TFunc>
struct GenericSetOpExecuterInit {
	using Creator = TCreator;
	using SetOpTraits = TFunc;
	static Creator init(const sserialize::ItemIndexPrivate* first, const sserialize::ItemIndexPrivate* second) {
		return Creator( SetOpTraits::maxSize(first, second) );
	}
};

///TFunc is one of the above and
///TCreator needs to have functions:
///
///push_back(uint32_t)
///flush()
///getPrivateIndex() -> ItemIndexPrivate*
template<typename TFunc, typename TCreator, typename TPositionIterator>
class GenericSetOpExecuter{
public:
	typedef TFunc SetOpTraits;
	typedef TPositionIterator PositionIterator;
	typedef GenericSetOpExecuterAccessors<PositionIterator> Accessors;
	typedef GenericSetOpExecuterInit<TCreator, SetOpTraits> Init;
public:
	inline static PositionIterator begin(const sserialize::ItemIndexPrivate * idx) { return Accessors::begin(idx); }
	inline static PositionIterator end(const sserialize::ItemIndexPrivate * idx) { return Accessors::end(idx); }
	inline static void next(PositionIterator & it) { Accessors::next(it); }
	inline static bool unequal(const PositionIterator & first, const PositionIterator & second) { return Accessors::unequal(first, second); }
	inline static uint32_t get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it) { return Accessors::get(idx, it); }
	
	inline static TCreator init(const sserialize::ItemIndexPrivate * first, const sserialize::ItemIndexPrivate * second) {
		return Init::init(first, second);
	}

	static sserialize::ItemIndexPrivate* execute(const sserialize::ItemIndexPrivate * first, const sserialize::ItemIndexPrivate * second) {
		TCreator creator( init(first, second) );
		
		PositionIterator fIt( begin(first) );
		PositionIterator fEnd( end(first) );
		PositionIterator sIt( begin(second) );
		PositionIterator sEnd( end(second) );
		
		for( ;unequal(fIt, fEnd) && unequal(sIt, sEnd); ) {
			uint32_t fId = get(first, fIt);
			uint32_t sId = get(second, sIt);
			if (fId < sId) {
				if (TFunc::pushFirstSmaller) {
					creator.push_back(fId);
				}
				next(fIt);
			}
			else if (sId < fId) {
				if (TFunc::pushSecondSmaller) {
					creator.push_back(sId);
				}
				next(sIt);
			}
			else {
				if (TFunc::pushEqual) {
					creator.push_back(fId);
				}
				next(fIt);
				next(sIt);
			}
		}
		if (TFunc::pushFirstRemainder) {
			for(; unequal(fIt, fEnd); next(fIt)) {
				creator.push_back(get(first, fIt));
			}
		}
		if (TFunc::pushSecondRemainder) {
			for(; unequal(sIt, sEnd); next(sIt)) {
				creator.push_back(get(second, sIt));
			}
		}
		return creator.getPrivateIndex();
	}
};

template<>
struct GenericSetOpExecuterAccessors<uint32_t> {
	typedef uint32_t PositionIterator;
	static PositionIterator begin(const sserialize::ItemIndexPrivate * idx);
	static PositionIterator end(const sserialize::ItemIndexPrivate * idx);
	static void next(PositionIterator & it);
	static bool unequal(const PositionIterator & first, const PositionIterator & second);
	static uint32_t get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it);
};

template<>
struct GenericSetOpExecuterAccessors<sserialize::ItemIndex::const_iterator> {
	typedef sserialize::ItemIndex::const_iterator PositionIterator;
	static PositionIterator begin(const sserialize::ItemIndexPrivate * idx);
	static PositionIterator end(const sserialize::ItemIndexPrivate * idx);
	static void next(PositionIterator & it);
	static bool unequal(const PositionIterator & first, const PositionIterator & second);
	static uint32_t get(const sserialize::ItemIndexPrivate * idx, const PositionIterator & it);
};

}}//end namespace detail::ItemIndex

}//end namespace

#endif