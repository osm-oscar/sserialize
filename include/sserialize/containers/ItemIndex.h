#ifndef SSERIALIZE_ITEM_INDEX_H
#define SSERIALIZE_ITEM_INDEX_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/storage/UByteArrayAdapter.h>
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

class ItemIndex: public RCWrapper<ItemIndexPrivate>  {
	typedef RCWrapper<ItemIndexPrivate> MyBaseClass;
public:
	///Types have to be flags (so it's easier to check a list indices if they all have the same type
	enum Types {
		T_NULL=0,
		T_SIMPLE=1,
		T_REGLINE=2,
		T_WAH=4,
		T_DE=8,
		T_RLE_DE=16,
		T_NATIVE=32,
		T_ELIAS_FANO=64,
		//the following are all in-memory
		T_EMPTY=128,
		T_INDIRECT=2*T_EMPTY,
		T_STL_DEQUE=4*T_EMPTY,
		T_STL_VECTOR=8*T_EMPTY,
		__T_LAST_ENTRY=T_STL_VECTOR
	};
	
	struct ItemFilter {
		virtual bool operator()(uint32_t id) const = 0;
	};
	
	struct ItemFilterIdentity: ItemFilter {
		virtual bool operator()(uint32_t /*id*/) const { return true; }
	};
	
	typedef sserialize::AbstractArrayIterator<uint32_t> const_iterator;
	typedef const_iterator iterator;
	
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
	explicit ItemIndex(std::initializer_list<uint32_t> l);
	explicit ItemIndex(const UByteArrayAdapter & index, Types type = T_REGLINE);
	explicit ItemIndex(const std::deque<uint32_t> & index);
	explicit ItemIndex(const std::vector<uint32_t> & index);
	explicit ItemIndex(std::vector<uint32_t> && index);
	template<typename T_ITERATOR>
	explicit ItemIndex(const T_ITERATOR & begin, const T_ITERATOR & end) : ItemIndex(std::vector<uint32_t>(begin, end)) {}
	~ItemIndex();
	void loadIntoMemory() const;
	
	UByteArrayAdapter data() const;
	
	ItemIndex & operator=(const ItemIndex & idx);

	uint32_t size() const;
	sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const;
	Types type() const;

	int count(uint32_t id) const;
	int find(uint32_t id) const;

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

	static ItemIndex fromBitSet(const sserialize::DynamicBitSet & bitSet, sserialize::ItemIndex::Types type);
	
	static ItemIndex fusedIntersectDifference(const std::vector<ItemIndex> & intersect, const std::vector<ItemIndex> & substract, uint32_t count, ItemFilter * filter = 0);
	static ItemIndex constrainedIntersect(const std::vector< ItemIndex > & intersect, uint32_t count, ItemFilter * filter = 0);
	
	static ItemIndex uniteK(const sserialize::ItemIndex& a, const sserialize::ItemIndex& b, uint32_t numItems);
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

}//end namespace

namespace std {

std::string to_string(sserialize::ItemIndex::Types t);

}

#endif
