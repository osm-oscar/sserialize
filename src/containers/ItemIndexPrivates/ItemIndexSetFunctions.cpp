#include "ItemIndexSetFunctions.h"
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>

namespace sserialize {

void intersectWithBinarySearch(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {
	const ItemIndexPrivate * smallerIndex;
	const ItemIndexPrivate * largerIndex;
	if (aindex->size() < bindex->size()) {
		smallerIndex = aindex;
		largerIndex = bindex;
	}
	else {
		smallerIndex = bindex;
		largerIndex = aindex;
	}
	uint32_t curId;
	uint32_t size = smallerIndex->size();
	for(size_t i = 0; i < size; i++) {
		curId = smallerIndex->at(i);
		if (largerIndex->find(curId) >= 0 ) {
			creator.push_back(curId);
		}
	}
}

void intersectWithMerge2(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = aindex->size();
	uint32_t bSize = bindex->size();
	if (aIndexIt < aSize && bIndexIt < bSize) {
		uint32_t aItemId = aindex->at(aIndexIt);
		uint32_t bItemId = bindex->at(bIndexIt);;
		while (true) {
			if (aItemId == bItemId) {
				creator.push_back(aItemId);
				aIndexIt++;
				bIndexIt++;
				if (aIndexIt < aSize) {
					aItemId = aindex->at(aIndexIt);
				}
				else {
					break;
				}
				if (bIndexIt < bSize) {
					bItemId = bindex->at(bIndexIt);
				}
				else {
					break;
				}
			}
			else if (aItemId < bItemId) {
				aIndexIt++;
				if (aIndexIt < aSize) {
					aItemId = aindex->at(aIndexIt);
				}
				else {
					break;
				}
			}
			else { //bItemId is smaller
				bIndexIt++;
				if (bIndexIt < bSize) {
					bItemId = bindex->at(bIndexIt);
				}
				else {
					break;
				}
			}
		}
	}
}

void uniteWithMerge2(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = aindex->size();
	uint32_t bSize = bindex->size();
	
	if (aIndexIt < aSize && bIndexIt < bSize) {
		uint32_t aItemId = aindex->at(aIndexIt);
		uint32_t bItemId = bindex->at(bIndexIt);
		while (true) {
			if (aItemId == bItemId) {
				creator.push_back(aItemId);
				aIndexIt++;
				bIndexIt++;
				if (aIndexIt < aSize)
					aItemId = aindex->at(aIndexIt);
				else
					break;
				if (bIndexIt < bSize)
					bItemId = bindex->at(bIndexIt);
				else
					break;
			}
			else if (aItemId < bItemId) {
				creator.push_back(aItemId);
				aIndexIt++;
				if (aIndexIt < aSize)
					aItemId = aindex->at(aIndexIt);
				else
					break;
			}
			else { //bItemId is smaller
				creator.push_back(bItemId);
				bIndexIt++;
				if (bIndexIt < bSize)
					bItemId = bindex->at(bIndexIt);
				else
					break;
			}
		}
	}

	while (aIndexIt < aSize) { //if there are still some elements left in aindex
		creator.push_back(aindex->at(aIndexIt));
		aIndexIt++;
	}

	while (bIndexIt < bSize) { //if there are still some elements left in bindex
		creator.push_back(bindex->at(bIndexIt));
		bIndexIt++;
	}
}

void differenceWithBinarySearch(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {
	uint32_t size = aindex->size();
	uint32_t itemId;
	for(uint32_t i = 0; i < size; i++) {
		itemId = aindex->at(i);
		if (bindex->find(itemId) < 0) {
			creator.push_back(itemId);
		}
	}
}

void differenceWithMerge(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {
	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = aindex->size();
	uint32_t bSize = bindex->size();
	uint32_t aItemId = aindex->at(aIndexIt);
	uint32_t bItemId = bindex->at(bIndexIt);;
	while (aIndexIt < aSize && bIndexIt < bSize) {
		if (aItemId == bItemId) {
			aIndexIt++;
			bIndexIt++;
			if (aIndexIt < aSize) {
				aItemId = aindex->at(aIndexIt);
			}
			else {
				break;
			}
			if (bIndexIt < bSize) {
				bItemId = bindex->at(bIndexIt);
			}
			else {
				break;
			}
		}
		else if (aItemId < bItemId) {
			creator.push_back(aItemId);
			aIndexIt++;
			if (aIndexIt < aSize) {
				aItemId = aindex->at(aIndexIt);
			}
			else {
				break;
			}
		}
		else { //bItemId is smaller
			bIndexIt++;
			if (bIndexIt < bSize) {
				bItemId = bindex->at(bIndexIt);
			}
			else {
				break;
			}
		}
	}

	while (aIndexIt < aSize) { //if there are still some elements left in aindex
		creator.push_back(aindex->at(aIndexIt));
		aIndexIt++;
	}
}

void symmetricDifferenceWithMerge(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator) {

	uint32_t aIndexIt = 0;
	uint32_t bIndexIt = 0;
	uint32_t aSize = aindex->size();
	uint32_t bSize = bindex->size();
	if (aIndexIt < aSize && bIndexIt < bSize) {
		uint32_t aItemId = aindex->at(aIndexIt);
		uint32_t bItemId = bindex->at(bIndexIt);
		while (true) {
			if (aItemId == bItemId) {
				aIndexIt++;
				bIndexIt++;
				if (aIndexIt < aSize) {
					aItemId = aindex->at(aIndexIt);
				}
				else {
					break;
				}
				if (bIndexIt < bSize) {
					bItemId = bindex->at(bIndexIt);
				}
				else {
					break;
				}
			}
			else if (aItemId < bItemId) {
				creator.push_back(aItemId);
				aIndexIt++;
				if (aIndexIt < aSize) {
					aItemId = aindex->at(aIndexIt);
				}
				else {
					break;
				}
			}
			else { //bItemId is smaller
				creator.push_back(bItemId);
				bIndexIt++;
				if (bIndexIt < bSize) {
					bItemId = bindex->at(bIndexIt);
				}
				else {
					break;
				}
			}
		}
	}

	while (aIndexIt < aSize) { //if there are still some elements left in aindex
		creator.push_back(aindex->at(aIndexIt));
		aIndexIt++;
	}

	while (bIndexIt < bSize) { //if there are still some elements left in bindex
		creator.push_back(bindex->at(bIndexIt));
		bIndexIt++;
	}
}

}//end namespace