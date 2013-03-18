#ifndef SSERIALIZE_STATIC_ITEM_GEO_GRID_H
#define SSERIALIZE_STATIC_ITEM_GEO_GRID_H
#include <sserialize/completers/GeoCompleterPrivateProxy.h>
#include <sserialize/containers/DynamicBitSet.h>
#include "RGeoGrid.h"
#include "ItemIndexStore.h"
#include <iomanip>
#include <iostream>

namespace sserialize {
namespace Static {
namespace spatial {

template<typename DataBaseType>
class ItemGeoGrid: public sserialize::Static::spatial::RGeoGrid<uint32_t> {
protected:
	typedef sserialize::Static::spatial::RGeoGrid<uint32_t> MyParentClass;
private:
	DataBaseType m_db;
	Static::ItemIndexStore m_indexStore;
private:
	ItemIndex mergeBinIndices(const std::vector<uint32_t> & definiteBins, const uint32_t start, const uint32_t end) const {
		if (start == end) {
			return indexFromBin( definiteBins.at(start) );
		}
		else {
			return mergeBinIndices(definiteBins, start, start + (end-start)/2) + mergeBinIndices(definiteBins, start + (end-start)/2 + 1, end);
		}
	}
	
	void putBinIndicesInto(const std::vector<uint32_t> & src, DynamicBitSet & dest) const {
		std::vector<uint32_t>::const_iterator end(src.end());
		for(std::vector<uint32_t>::const_iterator it(src.begin()); it != end; ++it) {
			indexFromBin(*it).putInto(dest);
		}
	}
public:
	ItemGeoGrid() : MyParentClass(), m_indexStore() {}
	ItemGeoGrid(const UByteArrayAdapter & data, const DataBaseType & db, const Static::ItemIndexStore & indexStore) : MyParentClass(data), m_db(db), m_indexStore(indexStore) {}
	virtual ~ItemGeoGrid() {}
	const DataBaseType & db()  const { return m_db; }
	DataBaseType & db() { return m_db; }
	
	ItemIndex indexFromId(uint32_t id) const {
		return m_indexStore.at(id);
	}
	
	ItemIndex indexFromTile(uint32_t tile) const {
		return m_indexStore.at( storage().at(tile) );
	}
	
	ItemIndex indexFromBin(uint32_t binId) const {
		return indexFromId( binAt(binId) );
	}
	
	uint32_t indexIdAt(uint32_t x, uint32_t y) const {
		if (x >= latCount() || y >= lonCount())
			return 0;
		uint32_t tile = selectBin(x,y);
		return indexIdAt(tile);
	}
	
	uint32_t indexIdAt(uint32_t tile) const {
		return storage().at(tile);
	}
	
	ItemIndex indexAt(uint32_t x, uint32_t y) const {
		return indexFromId(indexIdAt(x, y));
	}
	
	ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool approximate) const {
		std::vector<uint32_t> definiteBins, possibleBins;
		select(rect, definiteBins, possibleBins);
		if (!definiteBins.size() && !possibleBins.size())
			return ItemIndex();
			
		if (approximate) {
			uint32_t binsToMerge = definiteBins.size() + possibleBins.size();
			if (binsToMerge < 1024) {
				if (definiteBins.size())
					if (possibleBins.size())
						return mergeBinIndices(definiteBins, 0, definiteBins.size()-1) + mergeBinIndices(possibleBins, 0, possibleBins.size()-1);
					else
						return mergeBinIndices(definiteBins, 0, definiteBins.size()-1);
				else if (possibleBins.size())
					return mergeBinIndices(possibleBins, 0, possibleBins.size()-1);
			}
			else {
				DynamicBitSet bitSet;
				if (definiteBins.size())
					putBinIndicesInto(definiteBins, bitSet);
				if (possibleBins.size())
					putBinIndicesInto(possibleBins, bitSet);
				return  ItemIndex::fromBitSet(bitSet, m_indexStore.indexType());
			}
		}
		else {
			//let's try to use a set to merge the bins
			DynamicBitSet bitSet;
			for(size_t i = 0; i < possibleBins.size(); ++i) {
				ItemIndex idx = indexFromBin( possibleBins.at(i) );
				uint32_t idxSize = idx.size();
				for(uint32_t idxIt = 0; idxIt < idxSize; ++idxIt) {
					uint32_t itemId = idx.at(idxIt);
					if (m_db.match( itemId, rect ))
						bitSet.set(itemId);
				}
			}
			
			if (definiteBins.size())
				if (bitSet.size()) {
					putBinIndicesInto(definiteBins, bitSet);
					return  ItemIndex::fromBitSet(bitSet, m_indexStore.indexType());
				}
				else
					return mergeBinIndices(definiteBins, 0, definiteBins.size()-1);
			else if (bitSet.size())
				return ItemIndex::fromBitSet(bitSet, m_indexStore.indexType());
		}
		return ItemIndex();
	}
	
	ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool approximate) const {
		return ItemIndexIterator( complete(rect, approximate) );
	}
	
	ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndex & partner) const {
		return complete(rect, approximate) / partner;
	}
	
	ItemIndexIterator filter(const sserialize::spatial::GeoRect & rect, bool approximate, const ItemIndexIterator & partner) const {
		return db().filter(rect, approximate, partner);
	}
	
	std::ostream& printStats(std::ostream & out) const {
		out << "ItemGeoGrid Stats --BEGIN" << std::endl;
		out << "LatCount: " << latCount() <<  std::endl;
		out << "LonCount: " << lonCount() <<  std::endl;
		out << "LatStep: " << latStep() << std::endl;
		out << "LonStep: " << lonStep() << std::endl;
		out << "Boundary: " << rect() << std::endl;
		out << "Storage Size:" << getSizeInBytes() << std::endl;
		uint32_t emptyIdx = 0;
		uint32_t largestIdx = 0;
		uint32_t smallestsIdx = 0xFFFFFFFF;
		std::unordered_set<uint32_t> indexIds;
		for(size_t i = 0; i < storage().size(); i++) {
			ItemIndex idx( indexFromBin(i) );
			if (!idx.size())
				++emptyIdx;
			largestIdx = std::max(largestIdx, idx.size());
			smallestsIdx = std::min(smallestsIdx, idx.size());
			indexIds.insert(indexIdAt(i));
		}
		out << "Empty indices: " << emptyIdx <<  std::endl;
		out << "Smallest index: " << smallestsIdx << std::endl;
		out << "Largest index: " << largestIdx << std::endl;
		out << "IndexStore Information: " << std::endl;
		m_indexStore.printStats(out, indexIds);
		out << "ItemGeoGrid Stats --END" << std::endl;
		return out;
	}
	
	std::ostream& dump(std::ostream & out) const {
		if (latCount() == 0 || lonCount() == 0)
			return out << "ItemGeoGrid is empty";
		for(int y = lonCount()-1; y >= 0; --y) {
			out << "#" << std::setw(2) << std::setfill<char>('0') << y << "#";
			for(size_t x = 0; x < latCount(); ++x) {
				uint32_t idxId = indexIdAt(x,y);
				out << std::setw(2) << idxId << "|";
			}
			out << "#" << std::endl;
		}
		return out;
	}
	void dump() const {
		dump(std::cout);
	}
	
	std::string getName() const { return "sserialize::Static::spatial::ItemGeoGrid"; }
};

}}}//end namespace
#endif