#include <sserialize/utility/utilfuncs.h>
#include <sserialize/containers/ItemIndex.h>

namespace sserialize {

void mergeSortedContainer(std::vector<uint32_t> & out, const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
	if (a.size() == 0) {
		out = std::vector<uint32_t>(b.begin(), b.end());
		return;
	}
	if (b.size() == 0) {
		out = std::vector<uint32_t>(a.begin(), a.end());
		return;
	}
	std::vector<uint32_t> result;
	result.reserve(std::max(a.size(), b.size()));
	std::vector<uint32_t>::const_iterator aIndexIt(a.begin());
	std::vector<uint32_t>::const_iterator aEnd(a.end());
	std::vector<uint32_t>::const_iterator bIndexIt(b.begin());
	std::vector<uint32_t>::const_iterator bEnd(b.end());
	while (aIndexIt != aEnd && bIndexIt != bEnd) {
		uint32_t aItemId = *aIndexIt;
		uint32_t bItemId = *bIndexIt;

		if (aItemId == bItemId) {
			appendOrInsert(aItemId, result);
			++aIndexIt;
			++bIndexIt;
		}
		else if (aItemId < bItemId) {
			appendOrInsert(aItemId, result);
			++aIndexIt;
		}
		else { //bItemId is smaller
			appendOrInsert(bItemId, result);
			++bIndexIt;
		}
	}

	if (aIndexIt != aEnd)
		result.insert(result.end(), aIndexIt, aEnd);
	else if (bIndexIt != bEnd)
		result.insert(result.end(), bIndexIt, bEnd);
	
	result.swap(out);
}


bool haveCommonValue(const std::set<uint32_t> & a, const ItemIndex & b) {
	if (a.size() == 0 || b.size() == 0) {
		return false;
	}
	std::set<uint32_t>::const_iterator aIndexIt(a.begin());
	uint32_t bIndexIt = 0;
	while (aIndexIt != a.end() && bIndexIt < b.size()) {
		uint32_t aItemId = *aIndexIt;
		uint32_t bItemId = b.at(bIndexIt);
		if (aItemId == bItemId) {
			return true;
		}
		else if (aItemId < bItemId) {
			aIndexIt++;
		}
		else { //bItemId is smaller
			bIndexIt++;
		}
	}
	return false;
}

bool haveCommonValue(const std::deque<uint32_t> & a, const ItemIndex & b) {
	if (a.size() == 0 || b.size() == 0) {
		return false;
	}
	for(size_t i = 0; i < a.size(); i++) {
		if (b.count(a[i]) > 0)
			return true;
	}
	return false;
}


}//end namespace