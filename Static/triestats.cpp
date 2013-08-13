#include "triestats.h"
#include <stdlib.h>
#include <cstdlib> 
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <limits>

#ifdef __ANDROID__
inline double log2(double d) {return log(d)/0.6931471805599453;}
#endif

namespace sserialize {
namespace Static {

inline double myPow2(double d) { return d*d; }


std::string idxTypeNames[4] = { "EXACT", "PREFIX", "SUFFIX", "SUFFIX_PREFIX"};


TrieStats::IndexInfo::IndexInfo() :
cumulatedElementCount(0),
count(0),
rawIdEntropy(0),
idEntropy(0),
meanBitSize(0),
meanBitsPerRawId(0),
smallestIndex(std::numeric_limits<uint32_t>::max()),
largestIndex(std::numeric_limits<uint32_t>::min())
{
	storageDist["total"]= 0;
	storageDist["header"] = 0;
	storageDist["regline"] = 0;
	storageDist["index"] = 0;
}


std::ostream& TrieStats::print(std::ostream& out) {

	out << "Node count: " << nodeCount << std::endl;
	out << "utf8 char count in all nodes: " << nodeStringCharCount << std::endl;
	out << "Global node storage:" << std::endl;
	out << "Header: " << nodeStorageDist["header"] << "(" << (double)nodeStorageDist["header"]/nodeStorageDist["total"]*100 << "%)" << std::endl;
	out << "Nodestring: " << nodeStorageDist["string"] << "(" << (double)nodeStorageDist["string"]/nodeStorageDist["total"]*100 << "%)" << std::endl;
	out << "Child-Chars: " << nodeStorageDist["chars"] << "(" << (double)nodeStorageDist["chars"]/nodeStorageDist["total"]*100 << "%)" << std::endl;
	out << "Child-Ptrs: " << nodeStorageDist["ptrs"] << "(" << (double)nodeStorageDist["ptrs"]/nodeStorageDist["total"]*100 << "%)" << std::endl;
	out << "Index-Ptr: " << nodeStorageDist["indexptr"] << "(" << (double)nodeStorageDist["indexptr"]/nodeStorageDist["total"]*100 << "%)" << std::endl;
	out << "Node count per Level: " << std::endl;
	printStdMap(nodeCountPerLevel, out); out << std::endl;
	
	


	for(size_t idxType = 0; idxType < 4; idxType++) {
		IndexInfo & info = indexInfo[idxType];
		
		out << "Index stats for " << idxTypeNames[idxType] << " index: BEGIN" << std::endl;
		out << "Element count (sum { index.size()}): " << info.cumulatedElementCount << std::endl;
		out << "Storage counts; Header-Bytes: " << info.storageDist.at("header") << "(" << (double)info.storageDist["header"]/info.storageDist.at("total")*100 << "%)";
		out << "; RegressionLine-Bytes: " << info.storageDist["regline"] << "(" << (double)info.storageDist["regline"]/info.storageDist["total"]*100 << "%)";
		out << "; Id-Bytes: " << info.storageDist["index"] << "(" << (double)info.storageDist["index"]/info.storageDist["total"]*100 << "%)" << std::endl; 

		out << std::endl << "Mean index storage bits:" << info.meanBitSize << std::endl;
		
		out << std::endl << "Mean Bits per raw id: " << info.meanBitsPerRawId << std::endl;
		out << "Entropy of raw item index ids: " << info.rawIdEntropy << std::endl;
		out << "Entropy of item index ids: " << info.idEntropy << std::endl;

		out << "Index storage bits over depth:" << std::endl;
		out << "Level Min Max Mean Variance StdDev" << std::endl;
		for(size_t i = 0; i < info.meanBitsPerLevel.size(); i++) {
			if (info.meanBitsPerLevel.count(i) == 0)
				continue;
			out << i << " ";
			out << static_cast<int>(info.bitsMinMaxPerLevel.at(i).first) << " " << static_cast<int>(info.bitsMinMaxPerLevel.at(i).second) << " ";
			out << info.meanBitsPerLevel.at(i) << " " << info.bitsVariancePerLevel.at(i) << " ";
			out << sqrt(info.bitsVariancePerLevel.at(i));
			out << std::endl;
		}
		out << std::endl;
		out << "Index Storage amount over depth:" << std::endl;
		out << std::endl << "Index Storage amount over depth in percent:" << std::endl;
		double indexSize = ((double) indexStore.getSizeInBytes()) / 100;
		printStdMapDivSecond(info.storageOverDepth, indexSize, out);

		out << std::endl << "Index Storage amount over depth in percent (accumulated):" << std::endl;
		uint32_t curAcc = 0;
		for(std::unordered_map<uint16_t, uint32_t>::iterator it = info.storageOverDepth.begin(); it != info.storageOverDepth.end(); it++) {
			curAcc += it->second;
			out << it->first << ": " << curAcc/indexSize << "; ";
		}
		out << std::endl;

		out << "Index stats for " << idxTypeNames[idxType] << " index: END" << std::endl;
	}


	/*
	

	char strBuffer[256];
	std::string qstr;

	std::cout << "Do you want to write out itemindex raw id frequencies?" << std::endl;
	std::cin.getline(strBuffer, 256);
	qstr = std::string(strBuffer);
	if (qstr == "y") {
		for(size_t idxType = 0; idxType < 4; idxType++) {
			std::stringstream ss;
			ss << fnprefix << "rawId-frequencies_" << idxTypeNames[idxType] << ".plot";
			std::ofstream file;
			file.open(fnprefix.c_str());
			if (file.is_open()) {
				plotPrint(stats.itemIdxRawIdFreq[idxType], "ItemIndex RawId Frequencies (Absolute)", file);
				file.close();
			}
		}
	}

	std::cout << "Do you want to write out index stats?" << std::endl;
	std::cin.getline(strBuffer, 256);
	qstr = std::string(strBuffer);

	if (qstr == "y") {
		std::cout << "Files per level?";
		int fpl;
		std::cin >> fpl;
		if (fpl > 1 && fpl < 15) {
			writeMaxBitUsageIndexPerLevel(trie.getRootNode(), trie.getIndexStore(), fpl, fnprefix);
		}
		else std::cout << "too large/small number!" << std::endl;
		std::cin.getline(strBuffer, 256); //flush return
	}

	std::cout << "Do you want to write out the n largest index stats?" << std::endl;
	std::cin.getline(strBuffer, 256);
	qstr = std::string(strBuffer);
	if (qstr == "y") {
		std::cout << "How many?";
		int count;
		std::cin >> count;
		if (count > 1 && count < 100) {
			std::deque<ItemIndex> idxs = getLargestIndex(trie.getRootNode(), trie.getIndexStore(), count);
			for(size_t i = 0; i < idxs.size(); i++) {
				std::stringstream ss;
				ss << fnprefix << "-largest-" << i;
				writeIndexToFile(idxs[i], ss.str());
			}
		}
		else std::cout << "too large/small number!" << std::endl;
		std::cin.getline(strBuffer, 256); //flush return
	}
*/

	return out;
}

TrieStats::TrieStats(const TrieNode& rootNode, const ItemIndexStore& indexStore) :
rootNode(rootNode),
indexStore(indexStore),
nodeCount(0),
nodeStringCharCount(0)
{
	nodeStorageDist["total"] = 0;
	nodeStorageDist["header"] = 0;
	nodeStorageDist["string"] = 0;
	nodeStorageDist["chars"] = 0;
	nodeStorageDist["ptrs"] = 0;
	nodeStorageDist["indexptr"] = 0;
}

TrieStats::~TrieStats()
{

}


void TrieStats::recurse(const sserialize::Static::TrieNode& node, uint32_t level) {
	{
		nodeCount++;
		if (nodeCountPerLevel.count(level) == 0)
			nodeCountPerLevel[level] = 0;
		nodeCountPerLevel[level] += 1;
		
		nodeStringCharCount += node.str().size();

		nodeStorageDist["total"] += node.getStorageSize();
		nodeStorageDist["header"] += node.getHeaderStorageSize();
		nodeStorageDist["string"] += node.getNodeStringStorageSize();
		nodeStorageDist["chars"] += node.getChildCharStorageSize();
		nodeStorageDist["ptrs"] += node.getChildPtrStorageSize();
		nodeStorageDist["indexptr"] += node.getIndexPtrStorageSize();

		//Index stats

		bool hasIndexType[4] = {
			node.hasExactIndex(),
			node.hasPrefixIndex(),
			node.hasSuffixIndex(),
			node.hasSuffixPrefixIndex()
		};
		uint32_t idxPtr[4] = {
			node.getExactIndexPtr(),
			node.getPrefixIndexPtr(),
			node.getSuffixIndexPtr(),
			node.getSuffixPrefixIndexPtr()
		};
		for(size_t idxType = 0; idxType < 4; idxType++) {
			if (!hasIndexType[idxType])
				continue;
			ItemIndex idx = indexStore.at( idxPtr[idxType] );
			IndexInfo & info = indexInfo[idxType];
			
			info.cumulatedElementCount += idx.size();

			info.storageDist["total"] += idx.getSizeInBytes();
			
			info.smallestIndex = std::min<uint32_t>(info.smallestIndex, idx.getSizeInBytes());
			info.largestIndex = std::max<uint32_t>(info.largestIndex, idx.getSizeInBytes());

			if (info.storageOverDepth.count(level) == 0)
				info.storageOverDepth[level] = 0;
			info.storageOverDepth[level] += idx.getSizeInBytes();
			
			if (info.bitsMinMaxPerLevel.count(level) == 0)
				info.bitsMinMaxPerLevel[level] = std::pair<uint8_t, uint8_t>(255,0);
			if (info.bitsMinMaxPerLevel[level].first > idx.bpn())
				info.bitsMinMaxPerLevel[level].first = idx.bpn();
			if (info.bitsMinMaxPerLevel[level].second < idx.bpn())
				info.bitsMinMaxPerLevel[level].second = idx.bpn();

			//raw id frequencies
			for(size_t i = 0; i < idx.size(); i++) {
				uint32_t id = idx.at(i);
	
				if (info.IdFreq.count(id) == 0) {
					info.IdFreq[id] = 0;
				}
				info.IdFreq[id] += 1;
			}

				info.meanBitsPerRawId += idx.bpn()*idx.size();

			info.meanBitSize += idx.bpn(); //fill() does the normalization
			info.count += 1;
			
			if (info.countPerLevel.count(level) == 0)
				info.countPerLevel[level] = 0;
			info.countPerLevel[level] += 1;
			
			if (info.meanBitsPerLevel.count(level) == 0)
				info.meanBitsPerLevel[level] = 0;
			info.meanBitsPerLevel[level] += idx.bpn();
		}
	}
	//Recurse
	for(uint32_t i=0; i < node.childCount(); i++) {
		recurse(node.childAt(i), level+1);
	}
}

void TrieStats::indexBitsVariancePerLevelRecurse(const TrieNode& node, uint32_t depth) {
	bool hasIndexType[4] = {
		node.hasExactIndex(),
		node.hasPrefixIndex(),
		node.hasSuffixIndex(),
		node.hasSuffixPrefixIndex()
	};
	uint32_t idxPtr[4] = {
		node.getExactIndexPtr(),
		node.getPrefixIndexPtr(),
		node.getSuffixIndexPtr(),
		node.getSuffixPrefixIndexPtr()
	};
	for(size_t idxType = 0; idxType < 4; idxType++) {
		if (!hasIndexType[idxType])
			continue;
		ItemIndex idx = indexStore.at( idxPtr[idxType] );

		if (indexInfo[idxType].bitsVariancePerLevel.count(depth) == 0)
			indexInfo[idxType].bitsVariancePerLevel[depth] = 0;
		indexInfo[idxType].bitsVariancePerLevel[depth] += myPow2(idx.bpn()-indexInfo[idxType].meanBitsPerLevel.at(depth));
	}

	//Recurse
	for(uint32_t i=0; i < node.childCount(); i++) {
		indexBitsVariancePerLevelRecurse(node.childAt(i), depth+1);
	}
}


void TrieStats::fillStats() {
	recurse(rootNode, 0);
	
	for(size_t idxType = 0; idxType < 4; idxType++) {
		double entropy = 0.0;
		for(std::unordered_map<uint32_t, uint32_t>::iterator it = indexInfo[idxType].rawIdFreq.begin(); it != indexInfo[idxType].rawIdFreq.end(); ++it) {
			double wn = (double)it->second;
			entropy += wn * log2(wn);
		}
		indexInfo[idxType].rawIdEntropy = - (entropy/indexInfo[idxType].cumulatedElementCount) + log2(indexInfo[idxType].cumulatedElementCount);

		entropy = 0.0;
		for(std::unordered_map<uint32_t, uint32_t>::iterator it = indexInfo[idxType].IdFreq.begin(); it != indexInfo[idxType].IdFreq.end(); ++it) {
			double wn = (double)it->second;
			entropy += wn * log2(wn);
		}
		indexInfo[idxType].idEntropy = -(entropy/indexInfo[idxType].cumulatedElementCount) + log2(indexInfo[idxType].cumulatedElementCount);
		
		indexInfo[idxType].meanBitsPerRawId = indexInfo[idxType].meanBitsPerRawId / indexInfo[idxType].cumulatedElementCount;
		
		indexInfo[idxType].meanBitSize = indexInfo[idxType].meanBitSize/ indexInfo[idxType].count;
		
		for(std::unordered_map<uint16_t, double>::iterator it = indexInfo[idxType].meanBitsPerLevel.begin(); it != indexInfo[idxType].meanBitsPerLevel.end(); ++it) {
			it->second /= indexInfo[idxType].countPerLevel.at(it->first);
		}

	}
	
	indexBitsVariancePerLevelRecurse(rootNode, 0);
	for(size_t idxType = 0; idxType < 4; idxType++) {
		for(std::unordered_map<uint16_t, double>::iterator it = indexInfo[idxType].bitsVariancePerLevel.begin(); it != indexInfo[idxType].bitsVariancePerLevel.end(); ++it) {
			it->second /= indexInfo[idxType].countPerLevel.at(it->first);
		}
	}

}

void
TrieStats::
writeMaxBitUsageIndexPerLevelRecurse(sserialize::Static::TrieNode node, uint32_t depth, std::deque<uint32_t> & remWriteFilePerLevel, const std::string & fnprefix) {
	{
		bool hasIndexType[4] = {
			node.hasExactIndex(),
			node.hasPrefixIndex(),
			node.hasSuffixIndex(),
			node.hasSuffixPrefixIndex()
		};
		uint32_t idxPtr[4] = {
			node.getExactIndexPtr(),
			node.getPrefixIndexPtr(),
			node.getSuffixIndexPtr(),
			node.getSuffixPrefixIndexPtr()
		};
		for(size_t idxType = 0; idxType < 4; idxType++) {
			if (!hasIndexType[idxType])
				continue;
			ItemIndex idx = indexStore.at( idxPtr[idxType] );

			if (remWriteFilePerLevel.at(depth) > 0 && indexInfo[idxType].bitsMinMaxPerLevel.at(depth).second == idx.bpn()) {
				std::stringstream ss;
				ss << fnprefix << "_maxbitusage_" << idxTypeNames[idxType] << "_level" << depth << "-count" << remWriteFilePerLevel[depth];
				
				writeIndexToFile(idx, ss.str());
				remWriteFilePerLevel[depth] -= 1;
			}
		}
	}
	for(uint32_t i=0; i < node.childCount(); i++) {
		writeMaxBitUsageIndexPerLevelRecurse(node.childAt(i), depth+1, remWriteFilePerLevel, fnprefix);
	}
}

void TrieStats::writeMaxBitUsageIndexPerLevel(uint32_t indexPerLevel, const std::string& fnprefix) {
	std::deque<uint32_t> remWriteFilePerLevel(nodeCountPerLevel.size(), indexPerLevel);
	writeMaxBitUsageIndexPerLevelRecurse(rootNode, 0, remWriteFilePerLevel, fnprefix);
}



//Global funcs

bool writeIndexToFile(ItemIndex& idx, const std::string & fileName) {
	std::ofstream file;
	file.open(fileName.c_str());
	if (!file.is_open()) return false;
	if (idx.size() == 0) return true;
	
	uint32_t maxDiff = 0;
	uint32_t curDiff = 0;
	int32_t curOffSetCorrection = 0;
	int32_t maxOffsetPositive = 0;
	int32_t minOffsetNegative = 0;
	int32_t curOffsetDiff = 0;
	file << "plot \"-\" using 1:2 with linespoints title \"IDs with " << static_cast<uint32_t>(idx.bpn()) << " bits\", ";
	file << 0 << "\t" << idx.at(0) << "\t" << 0 << "\t" << 0 << std::endl;
	uint32_t count = 0;
	for(size_t i = 1; i < idx.size(); i++) {
		curOffsetDiff = idx.at(i) - curOffSetCorrection;
		if (curOffsetDiff > maxOffsetPositive) maxOffsetPositive = curOffsetDiff;
		if (curOffsetDiff < minOffsetNegative) minOffsetNegative = curOffsetDiff;
		curDiff = (idx.at(i)-idx.at(i-1));
		if (curDiff > maxDiff) maxDiff = curDiff;
		file << i << "\t" << idx.at(i) << "\t" << curDiff << "\t" << curOffsetDiff << std::endl;
		count++;
	}
	file << "# Bits:" << static_cast<int>(idx.bpn()) << "; Maxdiff: " << maxDiff;
	file << "; maxOffsetDiff: " << maxOffsetPositive << "; MinOffsetDiff: " << minOffsetNegative;
	file.close();
	return true;
}



void getLargestIndexRecurse( Static::TrieNode node, Static::ItemIndexStore indexFile, uint32_t count, std::map<uint32_t, uint32_t> * indexPositions) {
	{
		ItemIndex idx(indexFile.at(node.getIndexPtr()));
		uint32_t idxsize =  idx.getSizeInBytes();
		if (indexPositions->size() > count) {
			if (indexPositions->begin()->first < idxsize) {
				indexPositions->erase(indexPositions->begin());
				indexPositions->insert(std::pair<uint32_t, uint32_t>(idxsize, node.getIndexPtr()));
			}
		}
		else {
			indexPositions->insert(std::pair<uint32_t, uint32_t>(idxsize, node.getIndexPtr()));
		}
		
	}
	for(uint32_t i=0; i < node.childCount(); i++) {
		getLargestIndexRecurse(node.childAt(i), indexFile, count, indexPositions);
	}
}

std::deque<ItemIndex> getLargestIndex(Static::TrieNode node, Static::ItemIndexStore indexFile, uint32_t count) {
	std::map<uint32_t, uint32_t> idxPos;
	getLargestIndexRecurse(node, indexFile, count, &idxPos);
	std::deque<ItemIndex> r;
	for(std::map<uint32_t, uint32_t>::iterator it = idxPos.begin(); it != idxPos.end(); it++) {
		r.push_back(ItemIndex(indexFile.at(it->second)));
	}
	return r;
}

void histoStringLenOverDepthRecurse(sserialize::Static::TrieNode node, int depth, Histogram2D * histo) {
	histo->inc(node.strLen(), depth);
	for(uint32_t i=0; i < node.childCount(); i++) {
		histoStringLenOverDepthRecurse(node.childAt(i), depth+1, histo);
	}
}

void histoBranchCountOverDepthRecurse(Static::TrieNode node, int depth, Histogram2D * histo) {
	histo->inc(node.childCount(), depth);
	for(uint32_t i=0; i < node.childCount(); i++) {
		histoBranchCountOverDepthRecurse(node.childAt(i), depth+1, histo);
	}
}

void histoBranchCountOverStrLenRecurse(Static::TrieNode node, Histogram2D * histo) {
	histo->inc(node.strLen(), node.childCount());
	for(uint32_t i=0; i < node.childCount(); i++) {
		histoBranchCountOverStrLenRecurse(node.childAt(i), histo);
	}
}

Histogram2D histoStringLenOverDepth(Static::TrieNode node) {
	Histogram2D histo;
	histoStringLenOverDepthRecurse(node, 0, &histo);
	return histo;
}

Histogram2D histoBranchCountOverDepth(Static::TrieNode node) {
	Histogram2D histo;
	histoBranchCountOverDepthRecurse(node, 0, &histo);
	return histo;
}

Histogram2D histoBranchCountOverStrLen(Static::TrieNode node) {
	Histogram2D histo;
	histoBranchCountOverStrLenRecurse(node, &histo);
	return histo;
}

double variance(std::deque<long> & values) {
	long double mean = 0;
	for(unsigned int i = 0; i < values.size(); i++) {
		mean += values.at(i);
	}
	mean = mean / values.size();
	long double var = 0;
	for(unsigned int i = 0; i < values.size(); i++) {
		var += (values.at(i) - mean)*(values.at(i) - mean);
	}
	return var/values.size();
}

std::ostream& printTrieStats(const GeneralizedTrie& trie, std::ostream& out, std::string fnprefix) {
	TrieStats stats(trie.getRootNode(), trie.getIndexStore());
	stats.fill();
	stats.print(out);
	return out;
}


}}//end namespace
