#ifndef STATIC_REC_TRIE_STATS_H
#define STATIC_REC_TRIE_STATS_H
#include <ostream>
#include <map>
#include <sstream>
#include <cmath>
#include <sserialize/stats/histogram2d.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>

namespace sserialize {
namespace Static {

class TrieStats {
public:
	enum {IT_EXACT=0, IT_PREFIX=1, IT_SUFFIX=2, IT_SUFFIX_PREFIX=3};
private:
	TrieNode rootNode;
	ItemIndexStore indexStore;
public:
	uint32_t nodeCount;
	std::unordered_map<uint16_t, uint32_t> nodeCountPerLevel;
	uint32_t nodeStringCharCount;
	std::map<std::string, uint32_t> nodeStorageDist;
	
	struct IndexInfo {
		IndexInfo();
		~IndexInfo() {}
		uint64_t cumulatedElementCount;
		uint32_t count;
		std::map<std::string, uint32_t> storageDist;
		std::unordered_map<uint16_t, uint32_t> storageOverDepth;
		std::unordered_map<uint16_t, std::pair<uint8_t, uint8_t> > bitsMinMaxPerLevel;
		std::unordered_map<uint32_t, uint32_t> rawIdFreq;
		std::unordered_map<uint32_t, uint32_t> IdFreq;
		std::unordered_map<uint16_t, uint32_t> countPerLevel;
		double rawIdEntropy;
		double idEntropy;
		double meanBitSize;
		double meanBitsPerRawId;
		std::unordered_map<uint16_t, double> meanBitsPerLevel;
		std::unordered_map<uint16_t, double> bitsVariancePerLevel;
		uint32_t smallestIndex;
		uint32_t largestIndex;
	};
	
	IndexInfo indexInfo[4];
	
private:
	void recurse(const TrieNode & rootNode, uint32_t level);
	void indexBitsVariancePerLevelRecurse(const sserialize::Static::TrieNode & node, uint32_t depth);
	void fillStats();
	
	void writeMaxBitUsageIndexPerLevelRecurse(sserialize::Static::TrieNode node, uint32_t depth, std::deque<uint32_t> & remWriteFilePerLevel, const std::string & fnprefix);
	
public:
	TrieStats(const TrieNode & rootNode, const ItemIndexStore & indexStore);
	~TrieStats();
	void fill() { fillStats();}
	std::ostream & print(std::ostream & out);

	void writeMaxBitUsageIndexPerLevel(uint32_t indexPerLevel, const std::string& fnprefix);
};


bool writeIndexToFile(ItemIndex& idx, const std::string & fileName);
void writeMaxBitUsageIndexPerLevel(sserialize::Static::TrieNode node, Static::ItemIndexStore indexFile, uint32_t indexPerLevel, const std::string& fnprefix);
std::deque<ItemIndex> getLargestIndex(sserialize::Static::TrieNode node, Static::ItemIndexStore indexFile, uint32_t count);
Histogram2D histoStringLenOverDepth(sserialize::Static::TrieNode node);
Histogram2D histoBranchCountOverDepth(sserialize::Static::TrieNode node);
Histogram2D histoBranchCountOverStrLen(sserialize::Static::TrieNode node);


template<typename T1, typename T2>
void printStdMap(std::map<T1, T2> & m, std::ostream & out) {
	for(typename std::map<T1, T2>::iterator it = m.begin(); it != m.end(); it++) {
		out << it->first << ": " << it->second << "; ";
	}
}

template<typename T1, typename T2>
void printStdMap(std::unordered_map<T1, T2> & m, std::ostream & out) {
	for(typename std::unordered_map<T1, T2>::iterator it = m.begin(); it != m.end(); it++) {
		out << it->first << ": " << it->second << "; ";
	}
}

template<typename T1, typename T2, typename TDIV>
void printStdMapDivSecond(std::map<T1, T2> & m, TDIV div, std::ostream & out) {
	for(typename std::map<T1, T2>::iterator it = m.begin(); it != m.end(); it++) {
		out << it->first << ": " << it->second/div << "; ";
	}
}

template<typename T1, typename T2, typename TDIV>
void printStdMapDivSecond(std::unordered_map<T1, T2> & m, TDIV div, std::ostream & out) {
	for(typename std::unordered_map<T1, T2>::iterator it = m.begin(); it != m.end(); it++) {
		out << it->first << ": " << it->second/div << "; ";
	}
}

template<typename T1, typename T2>
void plotPrint(std::map<T1, T2> & m, const std::string & title, std::ostream & out) {
	out << "plot \"-\" using 1:2 with linespoints title \"" << title << "\"" << std::endl; 
	for(typename std::map<T1, T2>::iterator it = m.begin(); it != m.end(); it++) {
		out << it->first << "\t" << it->second << std::endl;
	}
}

std::ostream & printTrieStats(const sserialize::Static::GeneralizedTrie & trie, std::ostream & out, std::string fnprefix = "indexinfo-per-level-defstr");
double variance(std::deque<long> & values);

inline double testfunc() { return 0; }

}}//end namespace

#endif