#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <sserialize/utility/utilfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <string.h>

#include "datacreationfuncs.h"

using namespace sserialize;

bool checkItemIndex(std::set<uint32_t> & real, std::deque<uint8_t> & coded) {
	uint8_t * arr = new uint8_t[coded.size()];
	for(size_t i = 0; i < coded.size(); i++) {
		arr[i] = coded.at(i);
	}
	UByteArrayAdapter adap(arr, 0, coded.size());
	{
		UByteArrayAdapter adap2(adap);
		std::cout <<adap2.size() << std::endl; 
	}
	ItemIndex index(adap);
	uint32_t i = 0;
	for(std::set<uint32_t>::iterator it = real.begin(); it != real.end(); it++) {
		uint32_t realVal = *it;
		uint32_t codedVal = index.at(i);
		if (codedVal != realVal) {
			index.at(i);
			return false;
		}
		i++;
	}
	if (index.getSizeInBytes() != coded.size()) {
		std::cout << "getSizeInBytes() is broken!" << std::endl;
		return false;
	}
	delete[] arr;
	return true;
}

bool checkItemIndex(std::deque<uint32_t> & real, ItemIndex & index) {
	for(size_t i = 0; i < real.size(); i++) {
		uint32_t realVal = real[i];
		uint32_t codedVal = index.at(i);
		if (codedVal != realVal) {
			index.at(i);
			return false;
		}
	}
	return true;
}

bool testIntersect(const std::set<uint32_t> & aset, const std::set<uint32_t> & bset, ItemIndex aindex, ItemIndex bindex) {
	std::vector<ItemIndex> indexList;
	indexList.push_back(aindex);
	indexList.push_back(bindex);
	ItemIndex intersectIndex = ItemIndex::intersect(indexList);
	
	std::set<uint32_t> intersectSet;
	for(std::set<uint32_t>::const_iterator ait = aset.begin(); ait != aset.end(); ait++) {
		if (bset.count(*ait) > 0)
			intersectSet.insert(*ait);
	}
	
	uint32_t count = 0;
	for(std::set<uint32_t>::iterator it = intersectSet.begin(); it != intersectSet.end(); it++) {
		if (intersectIndex.at(count) != *it) {
			return false;
		}
		count++;
	}
	return true;
}

bool testIntersect(uint32_t setCount, uint32_t minEqual, uint32_t sizeVariance) {
	std::deque< std::set<uint32_t> > sets;
	createOverLappingSets(setCount, minEqual, sizeVariance, sets);
	if (setCount % 2 == 1)
		sets.pop_back();

	std::set<uint32_t> intersectSet;
	for(std::set<uint32_t>::const_iterator it = sets[0].begin(); it != sets[0].end(); it++) {
		bool insert = true;
		for(size_t j = 1; j < sets.size(); j++) {
			if (sets[j].count(*it) == 0) {
				insert = false;
				break;
			}
		}
		if (insert) {
			intersectSet.insert(*it);
		}
	}

	//create the item indices
	std::vector< std::deque<uint8_t> > codedIndices(sets.size());
	std::vector<ItemIndex> itemIndices;
	for(size_t i = 0; i < sets.size(); i++) {
		ItemIndexPrivateRegLine::create(sets[i], codedIndices[i]);
		UByteArrayAdapter adap(&(codedIndices[i]));
		itemIndices.push_back( ItemIndex(adap) );
	}
	
	ItemIndex intersectedItemIndex = ItemIndex::intersect(itemIndices);
	std::deque<uint32_t> intersectDeque;
	insertSetIntoDeque(intersectSet, intersectDeque);
	
	return checkItemIndex(intersectDeque, intersectedItemIndex);
}

bool testUnite(const std::set<uint32_t> & aset, const std::set<uint32_t> & bset, ItemIndex aindex, ItemIndex bindex) {
	std::vector<ItemIndex> indexList;
	indexList.push_back(aindex);
	indexList.push_back(bindex);
	ItemIndex tmpIndex = ItemIndex::unite(indexList);
	
	std::set<uint32_t> tmpSet;
	for(std::set<uint32_t>::const_iterator ait = aset.begin(); ait != aset.end(); ait++) {
			tmpSet.insert(*ait);
	}
	for(std::set<uint32_t>::const_iterator bit = bset.begin(); bit != bset.end(); bit++) {
			tmpSet.insert(*bit);
	}

	uint32_t count = 0;
	for(std::set<uint32_t>::iterator it = tmpSet.begin(); it != tmpSet.end(); it++) {
		uint32_t realId = *it;
		uint32_t indexId = tmpIndex.at(count);
		if (indexId != realId) {
			return false;
		}
		count++;
	}
	return true;
}

bool testUnite(uint32_t setCount, uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand) {
	std::deque< std::set<uint32_t> > sets;
	std::set<uint32_t> tmpSetA, tmpSetB;
	for (size_t i = 0; i < setCount; i+=2) {
		createOverLappingSets(tmpSetA, tmpSetB, minEqual, maxUnEqual, secondAddRand);
		sets.push_back(std::set<uint32_t>());
		sets.back().swap(tmpSetA);
		sets.push_back(std::set<uint32_t>());
		sets.back().swap(tmpSetB);
	}
	if (setCount % 2 == 1)
		sets.pop_back();

	std::set<uint32_t> mergedSet;
	for(size_t i = 0; i < sets.size(); i++) {
		for(std::set<uint32_t>::const_iterator it = sets[i].begin(); it != sets[i].end(); it++) {
			mergedSet.insert(*it);
		}
	}

	//create the item indices
	std::vector< std::deque<uint8_t> > codedIndices(sets.size());
	std::vector<ItemIndex> itemIndices;
	for(size_t i = 0; i < sets.size(); i++) {
		ItemIndexPrivateRegLine::create(sets[i], codedIndices[i]);
		UByteArrayAdapter adap(&(codedIndices[i]));
		itemIndices.push_back( ItemIndex(adap) );
	}
	
	ItemIndex mergedItemIndex = ItemIndex::unite(itemIndices);
	std::deque<uint32_t> mergedDeque;
	insertSetIntoDeque(mergedSet, mergedDeque);
	
	return checkItemIndex(mergedDeque, mergedItemIndex);
}

bool testDifference(const std::set<uint32_t> & aset, const std::set<uint32_t> & bset, ItemIndex aindex, ItemIndex bindex) {
	ItemIndex tmpIndex = ItemIndex::difference(aindex, bindex);
	
	std::set<uint32_t> tmpSet;
	for(std::set<uint32_t>::const_iterator ait = aset.begin(); ait != aset.end(); ait++) {
		if (bset.count(*ait) == 0)
			tmpSet.insert(*ait);
	}

	uint32_t count = 0;
	for(std::set<uint32_t>::iterator it = tmpSet.begin(); it != tmpSet.end(); it++) {
		if (tmpIndex.at(count) != *it) {
			return false;
		}
		count++;
	}
	return true;
}

bool testSymmetricDifference(const std::set<uint32_t> & aset, const std::set<uint32_t> & bset, ItemIndex aindex, ItemIndex bindex) {
	ItemIndex tmpIndex = ItemIndex::symmetricDifference(aindex, bindex);
	
	std::set<uint32_t> tmpSet;
	for(std::set<uint32_t>::const_iterator ait = aset.begin(); ait != aset.end(); ait++) {
		if (bset.count(*ait) == 0)
			tmpSet.insert(*ait);
	}

	for(std::set<uint32_t>::const_iterator bit = bset.begin(); bit != bset.end(); bit++) {
		if (aset.count(*bit) == 0)
			tmpSet.insert(*bit);
	}

	uint32_t count = 0;
	for(std::set<uint32_t>::iterator it = tmpSet.begin(); it != tmpSet.end(); it++) {
		if (tmpIndex.at(count) != *it) {
			return false;
		}
		count++;
	}
	return true;
}


bool testSetFunctions(uint32_t minEqual, uint32_t maxUnEqual, uint32_t secondAddRand) {
	std::set<uint32_t> a;
	std::set<uint32_t> b;
	std::deque<uint8_t> aList;
	std::deque<uint8_t> bList;
	
	
	createOverLappingSets(a, b, minEqual, maxUnEqual, secondAddRand);
	
	ItemIndexPrivateRegLine::create(a, aList, -1, true);
	ItemIndexPrivateRegLine::create(b, bList, -1, true);

	UByteArrayAdapter aAdap(&aList, 0, aList.size());
	UByteArrayAdapter bAdap(&bList, 0, bList.size());

	ItemIndex aIndex(aAdap);
	ItemIndex bIndex(bAdap);

	bool allOk = true;
	
	if (testIntersect(a, b, aIndex, bIndex)) {
		std::cout <<  "Passed intersect test" << std::endl;
	}
	else {
		allOk = false;
		std::cout <<  "Failed intersect test" << std::endl;
	}

	if (testUnite(a, b, aIndex, bIndex)) {
		std::cout <<  "Passed unite test" << std::endl;
	}
	else {
		allOk = false;
		std::cout <<  "Failed merge test" << std::endl;
	}

	if (testDifference(a, b, aIndex, bIndex)) {
		std::cout <<  "Passed difference test" << std::endl;
	}
	else {
		allOk = false;
		std::cout <<  "Failed difference test" << std::endl;
	}


	if (testSymmetricDifference(a, b, aIndex, bIndex)) {
		std::cout <<  "Passed symmetricDifference test" << std::endl;
	}
	else {
		allOk = false;
		std::cout <<  "Failed symmetricDifference test" << std::endl;
	}

	for (size_t setCount=2; setCount < 31; setCount++) { 
		if (testUnite(setCount, minEqual, maxUnEqual, secondAddRand)) {
			std::cout <<  "Passed unite with " <<  setCount << " sets test" << std::endl;
		}
		else {
			allOk = false;
			std::cout <<  "Failed unite with " << setCount << " sets test" << std::endl;
		}
	}

	for (size_t setCount=2; setCount < 31; setCount++) { 
		if (testIntersect(setCount, minEqual, maxUnEqual+secondAddRand)) {
			std::cout <<  "Passed intersect with " <<  setCount << " sets test" << std::endl;
		}
		else {
			allOk = false;
			std::cout <<  "Failed intersect with " << setCount << " sets test" << std::endl;
		}
	}

	return allOk;
}

bool testHugeIndex(uint32_t start, uint32_t end, uint32_t minDiff, uint32_t maxAddDiff) {
	std::set<uint32_t> hugeSet = createNumbersSet(start, end, minDiff, maxAddDiff);
	std::deque<uint8_t> data;
	ItemIndexPrivateRegLine::create(hugeSet, data,  -1, true);
	UByteArrayAdapter dataAdap(&data);
	ItemIndex idx(dataAdap, ItemIndex::T_REGLINE);
	return hugeSet == idx;
}

int main() {

	uint32_t indexCount = 32;
	uint32_t testCount = 0xFFF;
	uint32_t indirectIndexDepth = 10;
	uint32_t indirectTestRuns = 5;
	
	
	std::vector< std::set<uint32_t> > srcNums(indexCount);
	std::vector< std::deque<uint8_t> > codedNums(indexCount);
	
	//Fill the first
	srand( 0 );
	uint32_t rndNum;
	for(uint32_t i=0; i < testCount; i++) srcNums[0].insert(i);
	
	for(uint32_t i=1; i < indexCount; i++) {
		for(std::set<uint32_t>::iterator jt = srcNums[0].begin(); jt != srcNums[0].end(); jt++) {
			rndNum = rand() & 0x1;
			if (rndNum) {
				srcNums[i].insert(*jt);
			}
		}
	}

	std::cout << "Testing direct index with a single Element" << std::endl;
	for(uint32_t i = 0; i < indexCount; i++) {
		std::cout << "Creating itemindex " << i << "...";
		std::set<uint32_t> singleSet;
		singleSet.insert(*(srcNums[i].begin()));
		std::deque<uint8_t> coded;
		std::deque<unsigned int> real;
		real.push_back(*(singleSet.begin()));
		if (!ItemIndexPrivateRegLine::create(singleSet, coded)) {
			std::cout << "Failed!" << std::endl;
		}
		else {
			std::cout << "Passed!" << std::endl;
		}
		std::cout << "Testing...";
		UByteArrayAdapter adap(&coded);
		ItemIndex idx(adap);
		if (checkItemIndex(real, idx) ) {
			std::cout <<  "Passed." << std::endl;
		}
		else {
			std::cout << "Failed." << std::endl;
		}

	}


	std::cout << "Testing direct index" << std::endl;
	for(uint32_t i = 0; i < indexCount; i++) {
		std::cout << "Creating itemindex " << i << "...";
		if (!ItemIndexPrivateRegLine::create(srcNums[i], codedNums[i], -1, true)) {
			std::cout << "Failed!" << std::endl;
		}
		else {
			std::cout << "Passed!" << std::endl;
		}
	}
	
	for(uint32_t i = 0; i < indexCount; i++) {
		std::cout << "Testing index: " << i << "...";
		if (checkItemIndex(srcNums[i], codedNums[i])) std::cout << "passed" << std::endl;
		else std::cout << "failed!" << std::endl;
	}

	//Indirect index
	std::cout << "-----Testing indirect index------" << std::endl;
	for(uint32_t run = 0; run < indirectTestRuns; run++) {
		std::cout << "Test run " << run << std::endl;
	
		//create sets:
		std::deque<uint8_t> indirectCodedIndex;
		uint32_t indirectCodedOffsets[indirectIndexDepth];
		std::vector< std::deque<uint32_t> > indirectRealNums(indirectIndexDepth);
		std::vector< std::set<uint32_t> > indirectSrcNums(indirectIndexDepth);
		
		indirectSrcNums[0] = srcNums[0]; //first will contain everything
		for(std::set<uint32_t>::iterator jt = srcNums[0].begin(); jt != srcNums[0].end(); jt++) {
			indirectRealNums[0].push_back(*jt);
		}

		for(uint32_t depth=1; depth < indirectIndexDepth; depth++) {
			for(size_t i = 0; i < indirectRealNums[depth-1].size(); i++) {
				rndNum = rand() & 0x1;
				if (rndNum) {
					indirectSrcNums[depth].insert(i); //relative to parent
					indirectRealNums[depth].push_back(indirectRealNums[depth-1].at(i)); //real id
				}
			}
		}
		
		std::cout << "Serializing index hierachy" << std::endl;
		for(uint32_t depth = 0; depth < indirectIndexDepth; depth++) {
			indirectCodedOffsets[depth] = indirectCodedIndex.size();
			std::cout << "Creating itemindex " << depth << "...";
			if (!ItemIndexPrivateRegLine::create(indirectSrcNums[depth], indirectCodedIndex)) {
				std::cout << "Failed!" << std::endl;
			}
			else {
				std::cout << "Passed!" << std::endl;
			}
		}

		//now Test the hierachy
		uint8_t * indirectCodedIndexBuffer = new uint8_t[indirectCodedIndex.size()];
		for(size_t i = 0; i < indirectCodedIndex.size(); i++) indirectCodedIndexBuffer[i] = indirectCodedIndex[i];
		UByteArrayAdapter indirectCodedIndexBufferAdapter(indirectCodedIndexBuffer, 0, indirectCodedIndex.size());
		
		std::cout << "Testing hierachy" << std::endl;
		for(uint32_t depth = 0; depth < indirectIndexDepth; depth++) {
			std::cout << "Creating hierachy for depth " << depth << ". ";
			std::deque<uint32_t> ofs;
			for(uint32_t cdepth = 0; cdepth <= depth; cdepth++) {
				ofs.push_back(indirectCodedOffsets[cdepth]);
			}
			ItemIndex idx = ItemIndex::fromIndexHierachy(ofs, indirectCodedIndexBufferAdapter);
			std::cout << "Testing hierachy with " << idx.size() << "(" << indirectRealNums[depth].size() << ")...";
			if (!checkItemIndex(indirectRealNums[depth], idx)) {
				std::cout << "Failed!" << std::endl;
			}
			else {
				std::cout << "Passed!" << std::endl;
			}
		}
		delete[] indirectCodedIndexBuffer;
	}

	//Test set functions:
	if (testSetFunctions(0xFF, 0xFF, 0)) {
		std::cout << "All set tests with equal sized indices passed!" << std::endl;
	}
	else {
		std::cout << "Some set tests with equal sized indices failed!" << std::endl;
	}


	//Test set functions:
	if (testSetFunctions(8, 8, 128)) {
		std::cout << "All set tests with second larger passed!" << std::endl;
	}
	else {
		std::cout << "Some set tests with second larger failed!" << std::endl;
	}

	for (size_t i = 1337; i < 0xFFFFFFFF; i = (i << 1) + 1337) {
		if (testHugeIndex(0, i, 20, 10*1000)) {
			std::cout << "Huge set " << i << " test passed!" << std::endl;
		}
		else {
			std::cout << "Huge set " << i << " test failed!" << std::endl;
		}
	}
	return 0;
}