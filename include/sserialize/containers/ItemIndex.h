#ifndef SSERIALIZE_ITEM_INDEX_H
#define SSERIALIZE_ITEM_INDEX_H
#include <sserialize/utility/refcounting.h>
#include <vector>
#include <set>
#include <deque>
#include <ostream>

namespace sserialize {

class DynamicBitSet;
class UByteArrayAdapter;
class ItemIndexPrivate;

/** This class is an interface for an ItemIndex which is esential a set container of uint32_t,
  * but with different implentations which are all ref-counted, but not cowed
  * The constructors may throw an exception!
  */

class ItemIndex: public RCWrapper<ItemIndexPrivate>  {
	typedef RCWrapper<ItemIndexPrivate> MyBaseClass;
public:
	///Types have to be flags (so it's easier to check a list indices if they all have the same type
	enum Types {T_NULL=0, T_SIMPLE=1, T_REGLINE=2, T_WAH=4, T_DE=8, T_RLE_DE=16, T_EMPTY=32, T_INDIRECT=64, T_STL_DEQUE=128, T_STL_VECTOR=256};
private:
	void createPrivate(const UByteArrayAdapter & index, const ItemIndex::Types type);
	void createPrivate(const UByteArrayAdapter & index, const ItemIndex & realIdIndex, const ItemIndex::Types type);
private:
	static ItemIndex intersectWithTree(uint32_t start, uint32_t end, const std::vector< sserialize::ItemIndex >& set);
	static ItemIndex uniteWithTree(uint32_t start, uint32_t end, const std::vector< sserialize::ItemIndex >& set);
protected:
	ItemIndex(ItemIndexPrivate * data);
public:
	ItemIndex();
	ItemIndex(const ItemIndex & idx);
	ItemIndex(const UByteArrayAdapter & index, Types type = T_REGLINE);
	ItemIndex(const std::deque<uint32_t> & index);
	ItemIndex(const std::vector<uint32_t> & index);
	ItemIndex(const UByteArrayAdapter & index, const ItemIndex & realIdIndex, Types type = T_REGLINE);
	ItemIndex(const std::deque<uint32_t> & index, const ItemIndex & realIdIndex);
	ItemIndex(const std::vector<uint32_t> & index, const ItemIndex & realIdIndex);
	~ItemIndex();
	
	ItemIndex & operator=(const ItemIndex & idx);

	uint32_t size() const;
	uint32_t getSizeInBytes() const;
	Types type() const;

	int count(uint32_t id) const;
	int find(uint32_t id) const;

	void putInto(DynamicBitSet & bitSet) const;
	
	uint32_t at(uint32_t pos) const;
	uint32_t first() const;
	uint32_t last() const;

	
	
	void dump(const char * fileName);
	void dump(std::ostream & out);
	void dump();

	uint8_t bpn() const;
	

	ItemIndex operator+(const ItemIndex & idx) const;
	ItemIndex operator/(const ItemIndex & idx) const;
	ItemIndex operator-(const ItemIndex & idx) const;
	ItemIndex operator^(const ItemIndex & idx) const;
	
	std::set<uint32_t> toSet() const;
	///Store this index temporarily on disk
	void toDisk();

public:
	static ItemIndex uniteWithVectorBackend(const ItemIndex & a, const ItemIndex & b);

	///Creates am vector-based ItemIndex by absorbing. Doesn't copy. vec is empty afterwards
	static ItemIndex absorb(std::vector<uint32_t> & vec);

	static ItemIndex intersect(const std::vector< sserialize::ItemIndex >& set);
	static ItemIndex unite(const std::vector< sserialize::ItemIndex >& set);

	static ItemIndex difference(const ItemIndex & a, const ItemIndex & b);
	static ItemIndex symmetricDifference(const ItemIndex & a, const ItemIndex & b); //TODO: implement
	static ItemIndex unite(const ItemIndex & aindex, const ItemIndex & bindex);
	static ItemIndex intersect(const ItemIndex & aindex, const ItemIndex & bindex);

	static ItemIndex fromIndexHierachy(const std::deque<uint32_t> & offsets, const UByteArrayAdapter & indexFile, Types type = T_REGLINE);
	static ItemIndex fromFile(const std::string & fileName, bool deleteOnClose);
	template<typename TCONTAINER>
	static bool create(const TCONTAINER & src, UByteArrayAdapter & dest, Types type) {
		return false;
	}
	
	template<typename T_INDEX_TYPE, typename ... T_INDEX_ARGS>
	static ItemIndex createInstance(T_INDEX_ARGS ... args) {
		return ItemIndex( new T_INDEX_TYPE(args ...) );
	}
	
	template<typename T_INDEX_TYPE>
	static ItemIndex createInstance() {
		return ItemIndex( new T_INDEX_TYPE() );
	}

	static ItemIndex fromBitSet(const sserialize::DynamicBitSet & bitSet, sserialize::ItemIndex::Types type);
	
	static ItemIndex fusedIntersectDifference(const std::vector<ItemIndex> & intersect, const std::vector<ItemIndex> & substract, uint32_t count);
	static ItemIndex constrainedIntersect(const std::vector< ItemIndex > & intersect, uint32_t count);
};


}//end namespace

sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::ItemIndex & destination);

bool operator==(const sserialize::ItemIndex & set, const sserialize::ItemIndex & idx);

template<class TCONTAINER>
bool operator==(const sserialize::ItemIndex & idx, const TCONTAINER & set) {
	if (idx.size() != set.size())
		return false;
	uint32_t count = 0;
	for(typename TCONTAINER::const_iterator it = set.begin(); it != set.end(); ++it) {
		if (*it != idx.at(count)) {
			return false;
		}
		count++;
	}
	return true;
}

template<class TCONTAINER>
bool operator==(const TCONTAINER & set, const sserialize::ItemIndex & idx) {
	return idx == set;
}

template<class TCONTAINER>
bool operator!=(const sserialize::ItemIndex & idx, const TCONTAINER & set) {
	return !( idx == set);
}

template<class TCONTAINER>
bool operator!=(const TCONTAINER & set, const sserialize::ItemIndex & idx) {
	return !( idx == set);
}

std::ostream & operator<<(std::ostream & out, const sserialize::ItemIndex & idx);

#endif
