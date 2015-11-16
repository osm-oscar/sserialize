#include <sserialize/containers/ItemIndex.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {

//ItemIndex

void ItemIndex::createPrivate(const UByteArrayAdapter & index, const ItemIndex::Types type) {
	switch (type) {
		case(ItemIndex::T_SIMPLE):
			setPrivate( new ItemIndexPrivateSimple(index) );
			break;
		case(ItemIndex::T_REGLINE):
			setPrivate( new ItemIndexPrivateRegLine(index) );
			break;
		case (ItemIndex::T_WAH):
			setPrivate( new ItemIndexPrivateWAH(index) );
			break;
		case (ItemIndex::T_DE):
			setPrivate( new ItemIndexPrivateDE(index) );
			break;
		case (ItemIndex::T_RLE_DE):
			setPrivate( new ItemIndexPrivateRleDE(index) );
			break;
		case (ItemIndex::T_NATIVE):
			setPrivate( new detail::ItemIndexPrivate::ItemIndexPrivateNative(index) );
			break;
		default:
			setPrivate( new ItemIndexPrivateEmpty() );
			break;
	}
}

void ItemIndex::createPrivate(const UByteArrayAdapter& index, const ItemIndex& realIdIndex, const ItemIndex::Types type) {
	switch (type) {
		case(ItemIndex::T_SIMPLE):
			setPrivate( new ItemIndexPrivateSimpleIndirect(index, realIdIndex) );
			break;
		case(ItemIndex::T_REGLINE):
			setPrivate( new ItemIndexPrivateRegLineIndirect(index, realIdIndex) );
			break;
		case(ItemIndex::T_WAH):
			setPrivate( new ItemIndexPrivateWAHIndirect(index, realIdIndex) );
			break;
		case(ItemIndex::T_DE):
			setPrivate( new ItemIndexPrivateDEIndirect(index, realIdIndex) );
			break;
		case(ItemIndex::T_RLE_DE):
			setPrivate( new ItemIndexPrivateRleDEIndirect(index, realIdIndex) );
			break;
		case (ItemIndex::T_NATIVE):
			setPrivate( new detail::ItemIndexPrivate::ItemIndexPrivateNativeIndirect(index, realIdIndex) );
			break;
		default:
			setPrivate( new ItemIndexPrivateEmpty() );
			break;
	}
}

ItemIndex ItemIndex::intersectWithTree(uint32_t start, uint32_t end, const std::vector<ItemIndex> & set) {
	if (start == end)
		return set.at(start);
	uint32_t mid = start + (end-start)/2;
	return ItemIndex::intersectWithTree(start, mid, set) / ItemIndex::intersectWithTree(mid+1, end, set);
}

ItemIndex ItemIndex::uniteWithTree(uint32_t start, uint32_t end, const std::vector<ItemIndex> & set) {
	if (start == end)
		return set.at(start);
	uint32_t mid = start + (end-start)/2;
	return ItemIndex::uniteWithTree(start, mid, set) + ItemIndex::uniteWithTree(mid+1, end, set);
}

ItemIndex::ItemIndex() : RCWrapper< sserialize::ItemIndexPrivate > ( new ItemIndexPrivateEmpty()) {}
ItemIndex::ItemIndex(ItemIndexPrivate * data): RCWrapper< sserialize::ItemIndexPrivate >(data) {}


ItemIndex::ItemIndex(const ItemIndex & other) : RCWrapper< sserialize::ItemIndexPrivate >(other) {}

ItemIndex::ItemIndex(const UByteArrayAdapter & index, ItemIndex::Types type) : RCWrapper< sserialize::ItemIndexPrivate >(0) {
	createPrivate(index, type);
}

ItemIndex::ItemIndex(const std::deque<uint32_t> & index) : RCWrapper< sserialize::ItemIndexPrivate >(new sserialize::ItemIndexPrivateStlDeque( index ) ) {}
ItemIndex::ItemIndex(const std::vector< uint32_t >& index) : RCWrapper< sserialize::ItemIndexPrivate >(new sserialize::ItemIndexPrivateStlVector(index)) {}
ItemIndex::ItemIndex(std::vector<uint32_t> && index) {
	sserialize::ItemIndexPrivateStlVector * myPriv = new sserialize::ItemIndexPrivateStlVector(std::move(index));
	MyBaseClass::setPrivate(myPriv);
}


ItemIndex::ItemIndex(const UByteArrayAdapter& index, const ItemIndex& realIdIndex, Types type) : RCWrapper< sserialize::ItemIndexPrivate >(0) {
	createPrivate(index, realIdIndex, type);
}

ItemIndex::ItemIndex(const std::deque< uint32_t >& index, const ItemIndex& realIdIndex) :
	RCWrapper< sserialize::ItemIndexPrivate >(new ItemIndexPrivateStlDequeIndirect(index, realIdIndex) )
{}

ItemIndex::ItemIndex(const std::vector< uint32_t >& index, const ItemIndex& realIdIndex) :
	RCWrapper< sserialize::ItemIndexPrivate >(new ItemIndexPrivateStlVectorIndirect(index, realIdIndex) )
{}


ItemIndex::~ItemIndex() {}

ItemIndex & ItemIndex::operator=(const ItemIndex & idx) {
	RCWrapper< sserialize::ItemIndexPrivate >::operator=(idx);
	return *this;
}

void ItemIndex::loadIntoMemory() const {
	priv()->loadIntoMemory();
}

UByteArrayAdapter ItemIndex::data() const {
	return priv()->data();
}

uint32_t ItemIndex::at(uint32_t pos) const {
	return priv()->at(pos);
}

uint32_t ItemIndex::front() const {
	return priv()->first();
}

uint32_t ItemIndex::back() const {
	return priv()->last();
}

ItemIndex::const_iterator ItemIndex::cbegin() const {
	return const_iterator(priv()->cbegin());
}

ItemIndex::const_iterator ItemIndex::cend() const {
	return const_iterator(priv()->cend());
}

int ItemIndex::find(uint32_t id) const {
	return priv()->find(id);
}

int ItemIndex::count(uint32_t id) const {
	return (find(id) < 0 ? 0 : 1);
}

void ItemIndex::putInto(DynamicBitSet & bitSet) const {
	priv()->putInto(bitSet);
}

void ItemIndex::putInto(std::vector<uint32_t> & dest) const {
	dest.resize(size());
	priv()->putInto(&dest[0]);
}

void ItemIndex::putInto(uint32_t * dest) const {
	priv()->putInto(dest);
}

uint32_t ItemIndex::size() const {
	return priv()->size();
}

uint8_t ItemIndex::bpn() const {
	return priv()->bpn();
}

uint32_t ItemIndex::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

ItemIndex::Types ItemIndex::type() const {
	return priv()->type();
}

ItemIndex ItemIndex::operator+(const ItemIndex& idx) const {
	return ItemIndex::unite(*this, idx);
}

ItemIndex ItemIndex::operator/(const ItemIndex& idx) const {
	return ItemIndex::intersect(*this, idx);
}

ItemIndex ItemIndex::operator-(const ItemIndex& idx) const {
	return ItemIndex::difference(*this, idx);
}

ItemIndex ItemIndex::operator^(const ItemIndex& idx) const {
	return ItemIndex::symmetricDifference(*this, idx);
}

std::set<uint32_t> ItemIndex::toSet() const {
	std::set<uint32_t> s;
	uint32_t msize = size();
	std::set<uint32_t>::iterator sit = s.begin();
	for(uint32_t i = 0; i < msize; i++) {
		sit = s.insert(sit, at(i));
	}
	return s;
}

void ItemIndex::toDisk() {
	std::size_t s = size();
	UByteArrayAdapter dest( ItemIndexPrivateSimpleCreator::createCache(front(), back(), s, true) );
	ItemIndexPrivateSimpleCreator creator(front(), back(), s, dest);
	for(uint32_t i = 0; i < s; ++i)
		creator.push_back(at(i));
	creator.flush();
	this->operator=(creator.getIndex());
}

void ItemIndex::dump(const char* fileName) const {
	if (fileName) {
		std::fstream fout;
		fout.open(fileName);
		if (fout.is_open())
			dump(fout);
		fout.close();
	}
}

void ItemIndex::dump(std::ostream & out) const {
	out << "ItemIndex<size=" << size() << ", bpn=" << static_cast<uint32_t>(bpn());
	out << ">[";
	for(uint32_t i = 0; i < size(); i++) {
		out << at(i) << ", ";
	}
	out << "]" << std::endl;
}

void ItemIndex::dump() const {
	dump(std::cout);
}


ItemIndex ItemIndex::uniteWithVectorBackend(const ItemIndex & aindex, const ItemIndex & bindex) {
	std::vector<uint32_t> res;
	uint32_t aSize = aindex.size();
	uint32_t bSize = bindex.size();
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	res.reserve(std::max(aSize, bSize));
	uint32_t aItemId = aindex.at(aIndexIt);
	uint32_t bItemId = bindex.at(bIndexIt);
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aItemId == bItemId) {
			res.push_back(aItemId);
			aIndexIt++;
			bIndexIt++;
			aItemId = aindex.at(aIndexIt);
			bItemId = bindex.at(bIndexIt);
		}
		else if (aItemId < bItemId) {
			res.push_back(aItemId);
			aIndexIt++;
			aItemId = aindex.at(aIndexIt);
		}
		else { //bItemId is smaller
			res.push_back(bItemId);
			bIndexIt++;
			bItemId = bindex.at(bIndexIt);
		}
	}

	while (aIndexIt < aSize) { //if there are still some elements left in aindex
		res.push_back(aindex.at(aIndexIt));
		aIndexIt++;
	}

	while (bIndexIt < bSize) { //if there are still some elements left in bindex
		res.push_back(bindex.at(bIndexIt));
		bIndexIt++;
	}
	
	return ItemIndex(std::move(res));
}


ItemIndex ItemIndex::intersect(const std::vector<ItemIndex> & set) {
	if (set.size() == 0)
		return ItemIndex();
	else if (set.size() == 1)
		return set.front();
	else if (set.size() == 2)
		return ItemIndex::intersect(set.front(), set.back());
	else {
		return intersectWithTree(0, set.size()-1, set);
	}
}

ItemIndex ItemIndex::unite(const std::vector<ItemIndex> & set) {
	if (set.size() == 0)
		return ItemIndex();
	else if (set.size() == 1)
		return set.front();
	else if (set.size() == 2)
		return *(set.begin()) + *(set.rbegin());
	else {
		return uniteWithTree(0, set.size()-1, set);
	}
}

ItemIndex ItemIndex::intersect(const ItemIndex & aindex, const ItemIndex & bindex) {
	if (aindex.size() == 0 || bindex.size() == 0)
		return ItemIndex();
	return ItemIndex( aindex.priv()->intersect( bindex.priv() ) );
}

ItemIndex ItemIndex::unite(const ItemIndex & aindex, const ItemIndex & bindex) {
	if (aindex.size() == 0)
		return bindex;
	else if (bindex.size() == 0)
		return aindex;
	else
		return ItemIndex( aindex.priv()->unite( bindex.priv() ) );
}

ItemIndex ItemIndex::difference(const ItemIndex & a, const ItemIndex & b) {
	if (a.size() == 0 || b.size() == 0)
		return a;
	else
		return ItemIndex( a.priv()->difference( b.priv() ) );
}

ItemIndex ItemIndex::symmetricDifference(const ItemIndex & a, const ItemIndex & b) {
	if (a.size() == 0)
		return b;
	else if (b.size() == 0)
		return a;
	else
		return ItemIndex( a.priv()->symmetricDifference( b.priv() ) );
}


ItemIndex ItemIndex::fromIndexHierachy(const std::deque< uint32_t >& offsets, const UByteArrayAdapter & indexFile, Types type) {
	if (offsets.size() == 0) return ItemIndex();
	ItemIndex idx(indexFile+offsets.at(0), type);
	for(size_t i = 1; i < offsets.size(); i++) {
		idx = ItemIndex(indexFile+offsets.at(i), idx, type);
	}
	return idx;
}


ItemIndex ItemIndex::fromFile(const std::string & fileName, bool deleteOnClose){
	MmappedFile tempIndexFile = MmappedFile(fileName); //re-open read-only
	tempIndexFile.setDeleteOnClose(deleteOnClose);
	return ItemIndex(UByteArrayAdapter(tempIndexFile));
}


ItemIndex ItemIndex::fromBitSet(const DynamicBitSet & bitSet, Types type) {
	if (!bitSet.data().size())
		return ItemIndex();
	switch (type) {
	case (ItemIndex::T_DE):
		return ItemIndex( ItemIndexPrivateDE::fromBitSet(bitSet) );
	case (ItemIndex::T_RLE_DE):
		return ItemIndex( ItemIndexPrivateRleDE::fromBitSet(bitSet) );
	case (ItemIndex::T_WAH):
		return ItemIndex( ItemIndexPrivateWAH::fromBitSet(bitSet) );
	case (ItemIndex::T_REGLINE):
	case (ItemIndex::T_SIMPLE):
		return ItemIndex( ItemIndexPrivateSimple::fromBitSet(bitSet) );
	case (ItemIndex::T_NATIVE):
	case (ItemIndex::T_STL_DEQUE):
	case (ItemIndex::T_STL_VECTOR):
		return ItemIndex( ItemIndexPrivateStlVector::fromBitSet(bitSet) );
	case (ItemIndex::T_EMPTY):
		return ItemIndex();
	default:
		throw UnimplementedFunctionException("ItemIndex::fromBitSet type not supported");
	}
}


ItemIndex ItemIndex::fusedIntersectDifference(const std::vector< ItemIndex > & intersect, const std::vector< ItemIndex >& subtract, uint32_t count, ItemFilter * filter) {
	if (! intersect.size())
		return ItemIndex();

	int type = ItemIndex::T_NULL;
	
	std::vector< ItemIndex >::const_iterator endInt(intersect.end()), endDiff(subtract.end());
	
	for(std::vector< ItemIndex >::const_iterator it = intersect.begin(); it != endInt; ++it)
		type |= it->type();

	for(std::vector< ItemIndex >::const_iterator it(subtract.begin()); it != endDiff; ++it)
		type |= it->type();

		
	//TODO: store result in std::vector if result size is smaller than 1k
	//(vector should occupy less than one page then)
	if (count > 1024)
		type = T_EMPTY | T_INDIRECT;
		
	switch (type) {
	case ItemIndex::T_WAH:
	{
		std::vector<ItemIndexPrivateWAH*> intersectPrivates, substractPrivates;
		intersectPrivates.reserve(intersect.size());
		substractPrivates.reserve(subtract.size());
		for(std::vector< ItemIndex >::const_iterator it = intersect.begin(); it != endInt; ++it) {
			if (it->size())
				intersectPrivates.push_back( static_cast<ItemIndexPrivateWAH*>( it->priv() ) );
			else
				return ItemIndex();
		}
		
		for(std::vector< ItemIndex >::const_iterator it(subtract.begin()); it != endDiff; ++it) {
			if (it->size()) {
				substractPrivates.push_back( static_cast<ItemIndexPrivateWAH*>( it->priv() ) );
			}
		}
		ItemIndexPrivateWAH::fusedIntersectDifference(intersectPrivates, substractPrivates, count, filter);
	}
	default:
		ItemIndex idx = ItemIndex::intersect( intersect ) - ItemIndex::unite( subtract );
		if (filter) {
			std::vector<uint32_t> ids;
			ids.reserve(count);
			uint32_t size = idx.size();
			for(uint32_t i = 0; i < size && ids.size() < count; ++i) {
				uint32_t id = idx.at(i);
				if ((*filter)(id))
					ids.push_back(id);
			}
			idx = ItemIndex(std::move(ids));
		}
		return idx;
	}
}

ItemIndex ItemIndex::constrainedIntersect(const std::vector< ItemIndex >& intersect, uint32_t count, ItemIndex::ItemFilter * filter) {
	if (! intersect.size())
		return ItemIndex();

	int type = ItemIndex::T_NULL;
	
	std::vector< ItemIndex >::const_iterator endInt(intersect.end());
	
	for(std::vector< ItemIndex >::const_iterator it = intersect.begin(); it != endInt; ++it)
		type |= it->type();

		
	//TODO: store result in std::vector if result size is smaller than 1k
	//(vector should occupy less than one page then)
	if (count > 1024)
		type = T_EMPTY | T_INDIRECT;
		
	switch (type) {
	case ItemIndex::T_WAH:
	{
		std::vector<ItemIndexPrivateWAH*> intersectPrivates;
		intersectPrivates.reserve(intersect.size());
		for(std::vector< ItemIndex >::const_iterator it = intersect.begin(); it != endInt; ++it) {
			if (it->size())
				intersectPrivates.push_back( static_cast<ItemIndexPrivateWAH*>( it->priv() ) );
			else
				return ItemIndex();
		}
		
		return ItemIndexPrivateWAH::constrainedIntersect(intersectPrivates, count, filter);
	}
	case ItemIndex::T_RLE_DE:
	{
		if (!filter) {
			std::vector<ItemIndexPrivateRleDE*> intersectPrivates;
			intersectPrivates.reserve(intersect.size());
			for(std::vector< ItemIndex >::const_iterator it = intersect.begin(); it != endInt; ++it) {
				if (it->size())
					intersectPrivates.push_back( static_cast<ItemIndexPrivateRleDE*>( it->priv() ) );
				else
					return ItemIndex();
			}
			return ItemIndexPrivateRleDE::constrainedIntersect(intersectPrivates, count);
		}
		//else fall through and use full intersect 
	}
	default:
		ItemIndex idx = ItemIndex::intersect( intersect );
		if (filter) {
			std::vector<uint32_t> ids;
			ids.reserve(count);
			uint32_t size = idx.size();
			for(uint32_t i = 0; i < size && ids.size() < count; ++i) {
				uint32_t id = idx.at(i);
				if ((*filter)(id))
					ids.push_back(id);
			}
			idx = ItemIndex(std::move(ids));
		}
		return idx;
	}
}

ItemIndex ItemIndex::uniteK(const ItemIndex& a, const ItemIndex& b, uint32_t numItems) {
	return a.priv()->uniteK(b.priv(), numItems);
}

sserialize::UByteArrayAdapter& operator>>(sserialize::UByteArrayAdapter & source, sserialize::ItemIndex & destination) {
	sserialize::UByteArrayAdapter tmpAdap(source);
	tmpAdap.shrinkToGetPtr();
	destination = ItemIndex(tmpAdap);
	source.incGetPtr(destination.getSizeInBytes());
	return source;
}

std::ostream & operator<<(std::ostream & out, const sserialize::ItemIndex & s) {
	if (!s.size())
		return out << "sserialize::ItemIndex<0>[]";
	uint32_t end = s.size();
	uint32_t it = 0;
	
	out << "sserialize::ItemIndex<" << s.size() << ">[";
	while (true) {
		out << s.at(it);
		++it;
		if (it != end)
			out << ", ";
		else
			return out << "]";
	}
}

}//end namespace