#ifndef SSERIALIZE_ITEM_INDEX_H
#define SSERIALIZE_ITEM_INDEX_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/containers/CompactUintArray.h>
#include <sserialize/iterator/RangeGenerator.h>
#include <sserialize/utility/type_traits.h>
#include <vector>
#include <set>
#include <deque>
#include <ostream>
#include <type_traits>

namespace sserialize {

class DynamicBitSet;
class ItemIndexPrivate;

/** This class is an interface for an ItemIndex which is esential a set container of uint32_t,
  * but with different implentations which are all ref-counted, but not cowed
  * The constructors may throw an exception!
  */

class ItemIndex final: public RCWrapper<ItemIndexPrivate>  {
	typedef RCWrapper<ItemIndexPrivate> MyBaseClass;
public:
	enum Types {
		T_NULL=0,
		T_SIMPLE=0x1,
		T_REGLINE=0x2,
		T_WAH=0x4,
		T_DE=0x8,
		T_RLE_DE=0x10,
		T_NATIVE=0x20,
		T_ELIAS_FANO=0x40,
		T_PFOR=0x80,
		T_FOR=0x100,
		//more or less a wrapper
		T_BOUNDED_COMPACT_UINT_ARRAY=0x200,
		//the following are all in-memory
		T_EMPTY=0x400,
		T_INDIRECT=0x800,
		T_STL_DEQUE=0x1000,
		T_STL_VECTOR=0x2000,
		T_RANGE_GENERATOR=0x4000,
		__T_LAST_ENTRY=T_RANGE_GENERATOR,
		//The following indicates that the type has to be encoded in the data or somewhere else
		T_MULTIPLE=0x800000 
	};
	
	///You can check if an index supports fast random access using these masks
	enum RandomAccess {
		RANDOM_ACCESS_NO=T_WAH|T_DE|T_RLE_DE|T_ELIAS_FANO|T_PFOR|T_FOR|T_INDIRECT,
		RANDOM_ACCESS_YES=T_SIMPLE|T_REGLINE|T_NATIVE|T_EMPTY|T_STL_DEQUE|T_STL_VECTOR
	};
	
	enum CompressionLevel {
		CL_NONE, CL_LOW, CL_MID, CL_HIGH, CL_DEFAULT=CL_HIGH
	};

	
	struct ItemFilter {
		virtual bool operator()(uint32_t id) const = 0;
	};
	
	struct ItemFilterIdentity: ItemFilter {
		virtual bool operator()(uint32_t /*id*/) const { return true; }
	};
	
	typedef sserialize::AbstractArrayIterator<uint32_t> const_iterator;
	typedef const_iterator iterator;
	
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
	
private:
	void createPrivate(const UByteArrayAdapter & index, const ItemIndex::Types type);
private:
	static ItemIndex intersectWithTree(uint32_t start, uint32_t end, const std::vector< sserialize::ItemIndex >& set);
	static ItemIndex uniteWithTree(uint32_t start, uint32_t end, const std::vector< sserialize::ItemIndex >& set);
protected:
	ItemIndex(ItemIndexPrivate * data);
public:
	ItemIndex();
	ItemIndex(const ItemIndex & idx);
	ItemIndex(ItemIndex && idx);
	explicit ItemIndex(std::initializer_list<uint32_t> l);
	explicit ItemIndex(const UByteArrayAdapter & index, Types type = T_REGLINE);
	explicit ItemIndex(UByteArrayAdapter & index, Types type, UByteArrayAdapter::ConsumeTag);
	explicit ItemIndex(UByteArrayAdapter const & index, Types type, UByteArrayAdapter::NoConsumeTag);
	explicit ItemIndex(const std::deque<uint32_t> & index);
	explicit ItemIndex(const std::vector<uint32_t> & index);
	explicit ItemIndex(std::vector<uint32_t> && index);
	explicit ItemIndex(const sserialize::BoundedCompactUintArray & index);
	explicit ItemIndex(RangeGenerator<uint32_t> const & index);
	template<typename T_ITERATOR>
	explicit ItemIndex(const T_ITERATOR & begin, const T_ITERATOR & end) : ItemIndex(std::vector<uint32_t>(begin, end)) {}
	~ItemIndex() override;
	void loadIntoMemory() const;
	
	UByteArrayAdapter data() const;
	
	ItemIndex & operator=(const ItemIndex & idx);
	ItemIndex & operator=(ItemIndex && idx);

	uint32_t size() const;
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
	Types type() const;

	int count(uint32_t id) const;
	uint32_t find(uint32_t id) const;

	void putInto(DynamicBitSet & bitSet) const;
	void putInto(std::vector<uint32_t> & dest) const;
	void putInto(uint32_t * dest) const;
	
	uint32_t at(uint32_t pos) const;
	uint32_t front() const;
	uint32_t back() const;

	
	const_iterator cbegin() const;
	const_iterator cend() const;
	
	inline iterator begin() const { return cbegin(); }
	inline iterator end() const { return cend(); }
	
	void dump(const char * fileName) const;
	void dump(std::ostream & out) const;
	void dump() const;

	uint8_t bpn() const;
	

	ItemIndex operator+(const ItemIndex & idx) const;
	ItemIndex operator/(const ItemIndex & idx) const;
	ItemIndex operator-(const ItemIndex & idx) const;
	ItemIndex operator^(const ItemIndex & idx) const;
	
	ItemIndex & operator+=(const ItemIndex & idx);
	ItemIndex & operator/=(const ItemIndex & idx);
	ItemIndex & operator-=(const ItemIndex & idx);
	ItemIndex & operator^=(const ItemIndex & idx);
	
	std::set<uint32_t> toSet() const;
	std::vector<uint32_t> toVector() const;
	template<typename T_BACK_INSERTER>
	void insertInto(T_BACK_INSERTER inserter) {
		uint32_t s = size();
		for(uint32_t i = 0; i < s; ++i) {
			*inserter = at(i);
			++inserter;
		}
	}
	
	///Store this index temporarily on disk
	void toDisk();

public:
	static ItemIndex uniteWithVectorBackend(const ItemIndex & a, const ItemIndex & b);

	static ItemIndex intersect(const std::vector< sserialize::ItemIndex >& set);
	static ItemIndex unite(const std::vector< sserialize::ItemIndex >& set);

	static ItemIndex difference(const ItemIndex & a, const ItemIndex & b);
	static ItemIndex symmetricDifference(const ItemIndex & a, const ItemIndex & b);
	static ItemIndex unite(const ItemIndex & aindex, const ItemIndex & bindex);
	static ItemIndex intersect(const ItemIndex & aindex, const ItemIndex & bindex);

	static ItemIndex fromFile(const std::string & fileName, bool deleteOnClose);
	
	template<typename T_INDEX_TYPE, typename ... T_INDEX_ARGS>
	static ItemIndex createInstance(T_INDEX_ARGS ... args) {
		return ItemIndex( new T_INDEX_TYPE(args ...) );
	}
	
	template<typename T_INDEX_TYPE>
	static ItemIndex createInstance() {
		return ItemIndex( new T_INDEX_TYPE() );
	}

	static ItemIndex fromBitSet(const sserialize::DynamicBitSet & bitSet, int type, CompressionLevel ol = CL_DEFAULT);
	
	static ItemIndex fusedIntersectDifference(const std::vector<ItemIndex> & intersect, const std::vector<ItemIndex> & substract, uint32_t count, ItemFilter * filter = 0);
	static ItemIndex constrainedIntersect(const std::vector< ItemIndex > & intersect, uint32_t count, ItemFilter * filter = 0);
	
	static ItemIndex uniteK(const sserialize::ItemIndex& a, const sserialize::ItemIndex& b, uint32_t numItems);
};

template<>
struct is_trivially_relocatable<sserialize::ItemIndex> {
	static constexpr bool value = true;
};

sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & source, sserialize::ItemIndex & destination);

template<class TCONTAINER>
bool operator==(const sserialize::ItemIndex & idx, const TCONTAINER & set) {
	if (idx.size() != set.size()) {
		return false;
	}
	sserialize::ItemIndex::const_iterator idxIt(idx.cbegin());
	for(typename TCONTAINER::const_iterator it(set.cbegin()), end(set.cend()); it != end; ++it, ++idxIt) {
		if (*it != *idxIt) {
			return false;
		}
	}
	return true;
}

template<class TCONTAINER, typename TDummy = typename std::enable_if<! std::is_same<TCONTAINER, sserialize::ItemIndex>::value>::type >
bool operator==(const TCONTAINER & set, const sserialize::ItemIndex & idx) {
	return idx == set;
}

template<class TCONTAINER, typename TDummy = typename std::enable_if<! std::is_same<TCONTAINER, sserialize::ItemIndex>::value>::type >
bool operator!=(const sserialize::ItemIndex & idx, const TCONTAINER & set) {
	return !( idx == set);
}

template<class TCONTAINER>
bool operator!=(const TCONTAINER & set, const sserialize::ItemIndex & idx) {
	return !( idx == set);
}


std::ostream & operator<<(std::ostream & out, const sserialize::ItemIndex & idx);

std::string to_string(sserialize::ItemIndex::Types t);

bool from_string(const std::string & str, sserialize::ItemIndex::Types & type);

}//end namespace

namespace std {

std::string to_string(sserialize::ItemIndex::Types t);

}

#endif
