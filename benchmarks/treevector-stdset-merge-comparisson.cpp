#include <iostream>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h>
#include <vector>
#include <set>

template<typename T>
void appendOrInsert(const T & src, std::vector<T> & dest) {
	dest.push_back(src);
}

template<typename T_CONTAINER_DEST, typename T_CONTAINER_A, typename T_CONTAINER_B>
void mergeSortedContainer(T_CONTAINER_DEST & out, const T_CONTAINER_A & a, const T_CONTAINER_B & b, uint64_t & mergeComparisonCount) {
	if (a.size() == 0) {
		out = T_CONTAINER_DEST(b.begin(), b.end());
		return;
	}
	if (b.size() == 0) {
		out = T_CONTAINER_DEST(a.begin(), a.end());
		return;
	}
	T_CONTAINER_DEST result;
	typename T_CONTAINER_A::const_iterator aIndexIt(a.begin());
	typename T_CONTAINER_A::const_iterator aEnd(a.end());
	typename T_CONTAINER_B::const_iterator bIndexIt(b.begin());
	typename T_CONTAINER_B::const_iterator bEnd(b.end());
	while (aIndexIt != aEnd && bIndexIt != bEnd) {
		uint32_t aItemId = *aIndexIt;
		uint32_t bItemId = *bIndexIt;

		mergeComparisonCount += 2; //access penalty
		
		if (aItemId == bItemId) {
			appendOrInsert(aItemId, result);
			mergeComparisonCount += 1;
			++aIndexIt;
			++bIndexIt;
		}
		else if (aItemId < bItemId) {
			appendOrInsert(aItemId, result);
			mergeComparisonCount+=2;
			++aIndexIt;
		}
		else { //bItemId is smaller
			appendOrInsert(bItemId, result);
			mergeComparisonCount += 3;
			++bIndexIt;
		}
	}

	while (aIndexIt != aEnd) { //if there are still some elements left in aindex
		appendOrInsert(*aIndexIt, result);
		++aIndexIt;
		++mergeComparisonCount;
	}

	while (bIndexIt != bEnd) { //if there are still some elements left in bindex
		appendOrInsert(*bIndexIt, result);
		++bIndexIt;
		++mergeComparisonCount;
	}
	using std::swap;
	swap(result, out);
}

std::vector<uint32_t> createNumbersSet(uint32_t count) {
	std::set<uint32_t> ret;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	while( ret.size() < count) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		uint32_t key = rndNum & mask;
		ret.insert(key);
	}
	return std::vector<uint32_t>(ret.begin(), ret.end());
}

std::vector<uint32_t> mergeVecs(const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
	if (std::min(a.size(), b.size()) == 0)
		return std::vector<uint32_t>();
	std::vector<uint32_t> ret; ret.reserve( std::min(a.size(), b.size()) );
	std::vector<uint32_t>::const_iterator aIt = a.begin();
	std::vector<uint32_t>::const_iterator aEnd = a.end();
	std::vector<uint32_t>::const_iterator bIt = b.begin();
	std::vector<uint32_t>::const_iterator bEnd = b.end();
	
	uint32_t aId;
	uint32_t bId;
	while(aIt != aEnd && bIt != bEnd) {
		aId = *aIt;
		bId = *bIt;
		if (aId == bId) {
			ret.push_back(aId);
			++aIt;
			++bIt;
		}
		else if (aId <  bId) {
			ret.push_back(aId);
			++aIt;
		}
		else {
			ret.push_back(bId);
			++bIt;
		}
	}

	for(; aIt != aEnd; ++aIt)
		ret.push_back(*aIt);

	for(; bIt != bEnd; ++bIt)
		ret.push_back(*bIt);

	return ret;
}


std::vector<uint32_t> treeMerge(const std::vector< std::vector<uint32_t> > & v, uint32_t begin, uint32_t end) {
	if (begin ==  end)
		return v[begin];
	else if (end-begin == 1) {
		return mergeVecs(v[begin], v[end]);
	}
	else {
		return mergeVecs(treeMerge(v, begin, begin+(end-begin)/2), treeMerge(v, begin+(end-begin)/2+1, end));
	}
}

sserialize::ItemIndex treeMergeItemIndexVectorBackend(const std::vector< sserialize::ItemIndex > & v, uint32_t begin, uint32_t end) {
	if (begin ==  end)
		return v[begin];
	else if (end-begin == 1) {
		return sserialize::ItemIndex::uniteWithVectorBackend(v[begin], v[end]);
	}
	else {
		return sserialize::ItemIndex::uniteWithVectorBackend(treeMergeItemIndexVectorBackend(v, begin, begin+(end-begin)/2), treeMergeItemIndexVectorBackend(v, begin+(end-begin)/2+1, end));
	}
}

std::vector<uint32_t> treeMerge(const std::vector< std::vector<uint32_t> > & v, uint32_t begin, uint32_t end, uint64_t & comparisonCount) {
	if (begin ==  end)
		return v[begin];
	else if (end-begin == 1) {
		std::vector<uint32_t> ret;
		mergeSortedContainer(ret, v[begin], v[end], comparisonCount);
		return ret;
	}
	else {
		std::vector<uint32_t> ret;
		mergeSortedContainer(ret, treeMerge(v, begin, begin+(end-begin)/2, comparisonCount), treeMerge(v, begin+(end-begin)/2+1, end, comparisonCount), comparisonCount);
		return ret;
	}
}

struct TestResult {
	uint32_t bucketFill;
	uint64_t comparisonCount;
	uint32_t setSize;
	long int setTime;
	long int vecTime;
	long int indexTime;
	long int indexVecTime;
	long int indexWahTime;
	long int indexReglineTime;
	long int indexDETime;
	long int indexRleDETime;
	void print(std::ostream & out) {
		out << "bucketFill: " << bucketFill << std::endl;
		out << "setSize: " << setSize << std::endl;
		out << "comparisonCount: " << comparisonCount << std::endl;
		out << "setTime: " << setTime << std::endl;
		out << "vecTime: " << vecTime << std::endl;
		out << "indexTime: " << indexTime << std::endl;
		out << "indexVecTime: " << indexVecTime << std::endl;
		out << "indexWahTime: " << indexWahTime << std::endl;
		out << "indexRegLineTime: " << indexWahTime << std::endl;
		out << "indexDETime: " << indexWahTime << std::endl;
		out << "vecTime/setTime: " << (double)vecTime/setTime << std::endl;
		out << "indexTime/setTime: " << (double)indexTime/setTime << std::endl;
		out << "indexVecTime/setTime: " << (double)indexVecTime/setTime << std::endl;
		out << "indexWahTime/setTime: " << (double)indexWahTime/setTime << std::endl;
		out << "indexRegLineTime/setTime: " << (double)indexReglineTime/setTime << std::endl;
		out << "indexDETime/setTime: " << (double)indexDETime/setTime << std::endl;
		out << "indexRLeDETime/setTime: " << (double)indexRleDETime/setTime << std::endl;
	}
	void printPlotFriendly(std::ostream & out) {
		out << bucketFill << ";"; //0
		out << setSize << ";"; //1
		out << comparisonCount << ";"; //2
		out << setTime << ";"; //3
		out << vecTime << ";"; //4 (vector => vector)
		out << indexTime << ";"; //5 (vector => simple with ItemIndex)
		out << indexVecTime << ";"; //6 (vector => vector with ItemIndex)
		out << indexWahTime << ";"; //7
		out << indexReglineTime << ";"; //8
		out << indexDETime << ";"; //9
		out << indexRleDETime << ";"; // 10
	}
};

TestResult bench(uint32_t bucketCount, uint32_t bucketFillCount, uint32_t testCount, uint32_t testSelect) {
	if (!testCount)
		return TestResult();
	std::cout << "Creating " << bucketCount << " buckets with a fill of " << bucketFillCount << " and testing " << testCount << std::endl;

	std::vector< std::vector<uint32_t> > buckets;
	buckets.reserve(bucketCount);
	for(size_t i = 0; i < bucketCount; i++) {
		buckets.push_back( createNumbersSet(bucketFillCount) );
	}

	
	std::vector< sserialize::ItemIndex > itemIndexBuckets;
	std::vector< sserialize::ItemIndex > wahItemIndexBuckets;
	std::vector< sserialize::ItemIndex > reglineItemIndexBuckets;
	std::vector< sserialize::ItemIndex > deItemIndexBuckets;
	std::vector< sserialize::ItemIndex > rledeItemIndexBuckets;
	
	if (testSelect & 0x4) {
		itemIndexBuckets.reserve(bucketCount);
		for(size_t i = 0; i < bucketCount; i++) {
			itemIndexBuckets.push_back( sserialize::ItemIndex(buckets[i]) );
		}
	}
	
	if (testSelect & 0x10) {
		wahItemIndexBuckets.reserve(bucketCount);
		for(size_t i = 0; i < bucketCount; i++) {
			sserialize::UByteArrayAdapter adap(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexPrivateWAH::create(buckets[i], adap);
			wahItemIndexBuckets.push_back( sserialize::ItemIndex(adap, sserialize::ItemIndex::T_WAH) );
		}
	}

	if (testSelect & 0x20) {
		reglineItemIndexBuckets.reserve(bucketCount);
		for(size_t i = 0; i < bucketCount; i++) {
			std::deque<uint8_t> * dest = new std::deque<uint8_t>();
			sserialize::ItemIndexPrivateRegLine::create(buckets[i], *dest, -1, true);
			reglineItemIndexBuckets.push_back( sserialize::ItemIndex(sserialize::UByteArrayAdapter(dest, true), sserialize::ItemIndex::T_REGLINE) );
		}
	}

	if (testSelect & 0x40) {
		deItemIndexBuckets.reserve(bucketCount);
		for(size_t i = 0; i < bucketCount; i++) {
			sserialize::UByteArrayAdapter adap(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexPrivateDE::create(buckets[i], adap);
			deItemIndexBuckets.push_back( sserialize::ItemIndex(adap, sserialize::ItemIndex::T_DE) );
		}
	}
	
	if (testSelect & 0x80) {
		rledeItemIndexBuckets.reserve(bucketCount);
		for(size_t i = 0; i < bucketCount; i++) {
			sserialize::UByteArrayAdapter adap(new std::vector<uint8_t>(), true);
			sserialize::ItemIndexPrivateRleDE::create(buckets[i], adap);
			rledeItemIndexBuckets.push_back( sserialize::ItemIndex(adap, sserialize::ItemIndex::T_RLE_DE) );
		}
	}

	uint64_t comparisonCount = 0;
	treeMerge(buckets, 0, bucketCount-1, comparisonCount);
	std::cout << "Tree-Based comparisson penalty is " << comparisonCount << std::endl; 

	std::size_t setSize = 0;
	long int setTime = 0;
	long int vecTime = 0;
	long int indexTime = 0;
	long int indexVecTime = 0;
	long int indexWahTime = 0;
	long int indexReglineTime = 0;
	long int indexDETime = 0;
	long int indexRleDETime = 0;
	
	for(size_t i = 0; i < testCount; i++) {
		if (testSelect & 0x1) {
			std::cout << "Testing std::set merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			std::set<uint32_t> dest;
			for(size_t i = 0; i < bucketCount; i++) {
				dest.insert(buckets[i].begin(), buckets[i].end());
			}
			std::vector<uint32_t> result(dest.begin(), dest.end());
			tm.end();
			setTime += tm.elapsedTime();
			setSize = std::max(setSize, result.size());
			std::cout << " took " << setTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x2) {
			std::cout << "Testing tree merge with std::vector..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			std::vector<uint32_t> result = treeMerge(buckets, 0, bucketCount-1);
			tm.end();
			vecTime += tm.elapsedTime();
			setSize = std::max(setSize, result.size());
			std::cout << " took " << vecTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x4) {
			std::cout << "Testing ItemIndex merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = sserialize::ItemIndex::unite(itemIndexBuckets);
			tm.end();
			indexTime += tm.elapsedTime();
			setSize = std::max<std::size_t>(setSize, result.size());
			std::cout << " took " << indexTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x8) {
			std::cout << "Testing ItemIndex with vector-Backend merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = treeMergeItemIndexVectorBackend(itemIndexBuckets, 0, bucketCount-1);
			tm.end();
			indexVecTime += tm.elapsedTime();
			setSize = std::max<std::size_t>(setSize, result.size());
			std::cout << " took " << indexVecTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x10) {
			std::cout << "Testing ItemIndex with wah-Backend merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = sserialize::ItemIndex::unite(wahItemIndexBuckets);
			tm.end();
			indexWahTime += tm.elapsedTime();
			std::cout << " took " << indexWahTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x20) {
			std::cout << "Testing ItemIndex with regline-Backend merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = sserialize::ItemIndex::unite(reglineItemIndexBuckets);
			tm.end();
			indexReglineTime += tm.elapsedTime();
			std::cout << " took " << indexReglineTime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x40) {
			std::cout << "Testing ItemIndex with delta encoding-Backend merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = sserialize::ItemIndex::unite(deItemIndexBuckets);
			tm.end();
			indexDETime += tm.elapsedTime();
			std::cout << " took " << indexDETime << " useconds. Resultsize=" << result.size() << std::endl;
		}
		if (testSelect & 0x80) {
			std::cout << "Testing ItemIndex with run-length delta encoding-Backend merge..." << std::flush;
			sserialize::TimeMeasurer tm; tm.begin();
			sserialize::ItemIndex result = sserialize::ItemIndex::unite(rledeItemIndexBuckets);
			tm.end();
			indexRleDETime += tm.elapsedTime();
			std::cout << " took " << indexRleDETime << " useconds. Resultsize=" << result.size() << std::endl;
		}
	}
	if (setTime)
		setTime /= testCount;
	else
		setTime = 1;
	vecTime /= testCount;
	indexTime /= testCount;
	indexVecTime /= testCount;
	indexWahTime /= testCount;
	indexReglineTime /= testCount;
	indexDETime /= testCount;
	indexRleDETime /= testCount;
	
	TestResult result;
	result.bucketFill = bucketFillCount;
	result.comparisonCount = comparisonCount;
	result.setSize = setSize;
	result.setTime = setTime;
	result.vecTime = vecTime;
	result.indexTime = indexTime;
	result.indexVecTime = indexVecTime;
	result.indexWahTime = indexWahTime;
	result.indexReglineTime = indexReglineTime;
	result.indexDETime = indexDETime;
	result.indexRleDETime = indexRleDETime;
	return result;
}

void printHelp() {
	std::cout << std::endl << "Need testCount bucketCount bucketFillCountStart [ bucketFillCountMultIncrement bucketFillCountEnd [testSelect]]" << std::endl;
}


int main(int argc, char ** argv) {
	uint32_t bucketCount = 0;
	uint32_t bucketFillCountStart = 0;
	uint32_t bucketFillCountMultIncrement = 0;
	uint32_t bucketFillCountEnd = 0;
	uint32_t testCount = 0;
	uint32_t testSelect = std::numeric_limits<uint32_t>::max();
	if (argc > 3) {
		testCount = atol(argv[1]);
		bucketCount = atol(argv[2]);
		bucketFillCountStart = atol(argv[3]);
		if (argc > 5) {
			bucketFillCountMultIncrement = atol(argv[4]);
			bucketFillCountEnd = atol(argv[5]);
		}
		if (argc > 6)
			testSelect = atol(argv[6]);
	}
	else {
		std::cout << "Arguments given: " << std::endl;
		for (int i=0; i < argc; i++) {
			std::cout << argv[i];
		}
		printHelp();
		return 1;
	}
	
	if (bucketCount == 0 || bucketFillCountStart == 0 || testCount == 0) {
		std::cout << "Arguments given: " << std::endl;
		for (int i=0; i < argc; i++) {
			std::cout << argv[i] << " ";
		}
		printHelp();
		return 1;
	}
	
	std::cout << "Test count: " << testCount << std::endl;
	std::cout << "bucket count: " << bucketCount << std::endl;
	std::cout << "bucketFillCountStart: " << bucketFillCountStart << std::endl;
	std::cout << "bucketFillCountMultIncrement: " << bucketFillCountMultIncrement << std::endl;
	std::cout << "bucketFillCountEnd: " << bucketFillCountEnd << std::endl;
	std::cout << "testSelect: " << testSelect << std::endl;
	
	std::vector<TestResult> results;
	for(std::size_t i = bucketFillCountStart; i < bucketFillCountEnd; i *= bucketFillCountMultIncrement) {
		results.push_back( bench(bucketCount, i, testCount, testSelect) );
	}
	
	for(std::size_t i = 0; i < results.size(); ++i)
		results[i].print(std::cout);

	for(std::size_t i = 0; i < results.size(); ++i) {
		results[i].printPlotFriendly(std::cout);
		std::cout << std::endl;
	}


	return 0;
}
