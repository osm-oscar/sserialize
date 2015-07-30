#ifndef ITEM_INDEX_SET_FUNCTIONS_H
#define ITEM_INDEX_SET_FUNCTIONS_H

namespace sserialize {

class ItemIndexPrivateSimpleCreator;
class ItemIndexPrivate;

void intersectWithBinarySearch(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator);
void intersectWithMerge2(const sserialize::ItemIndexPrivate * aindex, const sserialize::ItemIndexPrivate * bindex, sserialize::ItemIndexPrivateSimpleCreator & creator);

void uniteWithMerge2(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator);

void differenceWithBinarySearch(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator);
void differenceWithMerge(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator);

void symmetricDifferenceWithMerge(const ItemIndexPrivate * aindex, const ItemIndexPrivate * bindex, ItemIndexPrivateSimpleCreator & creator);

}//end namespace

#endif