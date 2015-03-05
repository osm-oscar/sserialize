#include <sserialize/containers/TreedCQRImp.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

void TreeHandler::flattenOpNode(const FlatNode * n, uint32_t cellId, sserialize::ItemIndex& idx, FlattenResultType & frt) {
	sserialize::ItemIndex aIdx, bIdx;
	FlattenResultType frtA = FT_NONE;
	FlattenResultType frtB = FT_NONE;
	flatten(n+n->opNode.childA, aIdx, frtA);
	flatten(n+n->opNode.childB, bIdx, frtB);
	assert(frtA != FT_NONE && frtB != FT_NONE);
	switch (n->common.type) {
	case FlatNode::T_INTERSECT:
		if ((frtA | frtB) == FT_FM) {
			frt = FT_FM;
		}
		else {
			if (frtA == FT_FM) {
				idx = bIdx;
				frt = FT_FETCHED;
			}
			else if (frtB == FT_FM) {
				idx = aIdx;
				frt = FT_FETCHED;
			}
			else if (aIdx.size() && bIdx.size()) {
				idx = aIdx / bIdx;
				frt = FT_FETCHED;
			}
			else {
				frt = FT_EMPTY;
			}
		}
		break;
	case FlatNode::T_UNITE:
		if ((frtA | frtB) & FT_FM) {
			frt = FT_FM;
		}
		else if (aIdx.size() || bIdx.size()) {
			idx = aIdx + bIdx;
			frt = FT_FETCHED;
		}
		else {
			frt = FT_EMPTY;
		}
		break;
	case FlatNode::T_DIFF:
		if ((aIdx.size() || frtA == FT_FM) && !(frtB == FT_FM)) {
			if (frtA == FT_FM) {
				aIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			idx = aIdx - bIdx;
			frt = FT_FETCHED;
		}
		else {
			frt = FT_EMPTY;
		}
		break;
	case FlatNode::T_SYM_DIFF:
		if ((frtA | frtB) != FT_FM) {
			//this means at least one is a partial match
			if (frtA == FT_FM) {
				aIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			if (frtB == FT_FM) {
				bIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			idx = aIdx ^ bIdx;
			frt = FT_FETCHED;
		}
		else {
			frt = FT_EMPTY;
		}
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
		break;
	};
}

///fullMatch needs to be set to false, lazy eval only
void TreeHandler::flatten(const FlatNode * n, const ItemIndex * idces, sserialize::ItemIndex& idx, FlattenResultType & frt) {
	if (!n) {}
	switch(n->common.type) {
	case FlatNode::T_PM_LEAF:
		idx = m_idxStore.at(n->pmNode.pmIdxId);
		frt = FT_FETCHED;
		break;
	case FlatNode::T_FM_LEAF:
		frt = FT_FM;
		break;
	case FlatNode::T_FETCHED_LEAF:
		idx = idces[n->fetchedNode.internalIdxId];
		break;
	case FlatNode::T_INTERSECT:
	case FlatNode::T_UNITE:
	case FlatNode::T_DIFF:
	case FlatNode::T_SYM_DIFF:
		{
			flattenOpNode(n, idx, frt);
		}
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
	};
}


void TreeHandler::flatten(const FlatNode * n, FlatNode & destNode, std::vector<ItemIndex> & destInternalIdces)  {
	sserialize::ItemIndex res;
	FlattenResultType frt;
	flatten(n, res, frt);
	if (frt) {
		destNode.type = FlatNode::T_FM_LEAF;
	}
	else {
		destNode.type = FlatNode::T_FETCHED_LEAF;
		destNode.fetchedNode.internalIdxId = destInternalIdces.size();
		destInternalIdces.push_back(res);
	}
}


TreedCQRImp::TreedCQRImp() {}

TreedCQRImp::TreedCQRImp(const ItemIndex & fmIdx, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend());

	uint32_t totalSize = fmIdx.size();
	m_desc.reserve(totalSize);
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, *fmIt, 0);
	}
}

TreedCQRImp::TreedCQRImp(uint32_t cellId, uint32_t cellIdxId, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{
	uint32_t totalSize = 1;
	m_desc.reserve(totalSize);
	m_desc.push_back( CellDesc(0, cellId) );
}

TreedCQRImp::TreedCQRImp(const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{}

TreedCQRImp::~TreedCQRImp() {}

TreedCQRImp::CellDesc::CellDesc(TreedCQRImp::CellDesc && other) :
treeBegin(notree),
treeEnd(notree)
{
	fullMatch = other.fullMatch;
	pmIdxId = other.pmIdxId;
	cellId = other.cellId;
}

TreedCQRImp::CellDesc::CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId) :
treeBegin(notree),
treeEnd(notree)
{
	this->fullMatch = fullMatch;
	this->cellId = cellId;
	this->pmIdxId = pmIdxId;
}

TreedCQRImp::CellDesc::CellDesc(const TreedCQRImp::CellDesc& other) :
treeBegin(notree),
treeEnd(notree)
{
	fullMatch = other.fullMatch;
	pmIdxId = other.pmIdxId;
	cellId = other.cellId;
}

TreedCQRImp::CellDesc::CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId, uint32_t treeBegin, uint32_t treeEnd) :
treeBegin(treeBegin),
treeEnd(treeEnd)
{
	this->fullMatch = fullMatch;
	this->cellId = cellId;
	this->pmIdxId = pmIdxId;
}

TreedCQRImp::CellDesc::~CellDesc() {}

TreedCQRImp * TreedCQRImp::intersect(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	uint32_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		if (!myCD.hasTree() && !oCD.hasTree()) {
			uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
			switch(ct) {
			case 0x0: //both partial, create tree
				{
					uint32_t treeBegin = r.m_trees.size();
					r.m_trees.emplace_back(FlatNode::T_INTERSECT);
					{
						FlatNode & opNode = r.m_trees.back();
						opNode.opNode.childA = 1;
						opNode.opNode.childA = 2;
					}
					
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
					
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
					
					r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
				}
				break;
			case 0x2: //my full
				r.m_desc.push_back(oCD);
				break;
			case 0x1: //o full
				r.m_desc.push_back(myCD);
				break;
			case 0x3: //both full
				r.m_desc.emplace_back(1, myCellId, 0, 0);
				break;
			default:
				break;
			};
		}
		else { //at least one has a tree
			//if any of the two is a full match then we only need to copy the other tree
			if (myCD.fullMatch) { //copy tree from other
				r.m_desc.emplace_back(0, myCellId, 0);
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+myCD.treeBegin, o.m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			else if (oCD.fullMatch) { //copy tree from this
				r.m_desc.emplace_back(0, oCellId, 0);
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			else { //this means at least one has a tree and the other is partial or has a tree as well

				uint32_t treeBegin = r.m_trees.size();
				r.m_trees.emplace_back(FlatNode::T_INTERSECT);
				r.m_trees[treeBegin].opNode.childA = 1;
				
				if (myCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), m_trees.front() + myCD.treeBegin, m_trees.front() + myCD.treeEnd);
					r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = myCellId;
					r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
					r.m_trees[treeBegin].opNode.childB = 2;
				}
			
				if (oCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), o.m_trees.front() + oCD.treeBegin, o.m_trees.front() + oCD.treeEnd);
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = oCellId;
					r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
				}
				r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_desc.size());
			}
		}
		++myI;
		++oI;
	}
	//unite the rest

	assert(r.m_desc.size() <= std::min<uint32_t>(m_desc.size(), o.m_desc.size()));
	return rPtr;
}

TreedCQRImp * TreedCQRImp::unite(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	uint32_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			r.m_desc.push_back(oCD);
			if (oCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+oCD.treeBegin, o.m_trees.cbegin()+oCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			++oI;
			continue;
		}
		if (myCD.fullMatch || oCD.fullMatch) {
			r.m_desc.emplace_back(1, myCellId, 0);
		}
		else if (!myCD.hasTree() && !oCD.hasTree()) { //both are partial and dont have tree
			uint32_t treeBegin = r.m_trees.size();
			r.m_trees.emplace_back(FlatNode::T_UNITE);
			{
				FlatNode & opNode = r.m_trees.back();
				opNode.opNode.childA = 1;
				opNode.opNode.childA = 2;
			}
			
			r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
			r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
			
			r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
			r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
			
			r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
		}
		else { //at least one has a tree, none is full match
			uint32_t treeBegin = r.m_trees.size();
			r.m_trees.emplace_back(FlatNode::T_UNITE);
			r.m_trees[treeBegin].opNode.childA = 1;
			
			if (myCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), m_trees.front() + myCD.treeBegin, m_trees.front() + myCD.treeEnd);
				r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
			}
			else {
				r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
				r.m_trees.back().pmNode.cellId = myCellId;
				r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
				r.m_trees[treeBegin].opNode.childB = 2;
			}
		
			if (oCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), o.m_trees.front() + oCD.treeBegin, o.m_trees.front() + oCD.treeEnd);
			}
			else {
				r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
				r.m_trees.back().pmNode.cellId = oCellId;
				r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
			}
			r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_desc.size());
		}
		++myI;
		++oI;
	}
	//unite the rest
	
	if (myI < myEnd) {
		uint32_t rDescBegin = r.m_desc.size();
		uint32_t rTreesBegin = r.m_trees.size();
		uint32_t lastTreeBegin = m_desc[myI].treeBegin;
		int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
		//adjust tree pointers
		for(uint32_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
			CellDesc & cd = r.m_desc[i];
			if (cd.hasTree()) {
				cd.treeBegin = ((int64_t)(cd.treeBegin) + treeOffsetCorrection);
				cd.treeEnd = ((int64_t)(cd.treeEnd) + treeOffsetCorrection);
			}
		}
	}
	else if (oI < oEnd) {
		uint32_t rDescBegin = r.m_desc.size();
		uint32_t rTreesBegin = r.m_trees.size();
		uint32_t lastTreeBegin = o.m_desc[oI].treeBegin;
		int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin
		r.m_desc.insert(r.m_desc.end(), o.m_desc.cbegin()+oI, o.m_desc.cend());
		r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+m_desc[oI].treeBegin, o.m_trees.cend());
		//adjust tree pointers
		for(uint32_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
			CellDesc & cd = r.m_desc[i];
			if (cd.hasTree()) {
				cd.treeBegin = ((int64_t)(cd.treeBegin) + treeOffsetCorrection);
				cd.treeEnd = ((int64_t)(cd.treeEnd) + treeOffsetCorrection);
			}
		}
	}
	assert(r.m_desc.size() >= std::max<uint32_t>(m_desc.size(), o.m_desc.size()));
	return rPtr;
}

TreedCQRImp * TreedCQRImp::diff(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	uint32_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		if (oCD.fullMatch) {
			continue;
		}
		else {//we definetly have to create a tree, no matter what
			uint32_t treeBegin = r.m_trees.size();
			r.m_trees.emplace_back(FlatNode::T_DIFF);
			r.m_trees[treeBegin].opNode.childA = 1;
			
			if (myCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), m_trees.front() + myCD.treeBegin, m_trees.front() + myCD.treeEnd);
				r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
			}
			else {
				if (myCD.fullMatch) {
					r.m_trees.emplace_back(FlatNode::T_FM_LEAF);
					r.m_trees.back().fmNode.cellId = myCellId;
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = myCellId;
					r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
				}
				r.m_trees[treeBegin].opNode.childB = 2;
			}
		
			if (oCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), o.m_trees.front() + oCD.treeBegin, o.m_trees.front() + oCD.treeEnd);
			}
			else {
				if (oCD.fullMatch) {
					r.m_trees.emplace_back(FlatNode::T_FM_LEAF);
					r.m_trees.back().fmNode.cellId = oCellId;
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = oCellId;
					r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_desc.size());
		}
		++myI;
		++oI;
	}
	//add the rest
	if (myI < myEnd) {
		uint32_t rDescBegin = r.m_desc.size();
		uint32_t rTreesBegin = r.m_trees.size();
		uint32_t lastTreeBegin = m_desc[myI].treeBegin;
		int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
		//adjust tree pointers
		for(uint32_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
			CellDesc & cd = r.m_desc[i];
			if (cd.hasTree()) {
				cd.treeBegin = ((int64_t)(cd.treeBegin) + treeOffsetCorrection);
				cd.treeEnd = ((int64_t)(cd.treeEnd) + treeOffsetCorrection);
			}
		}
	}

	assert(r.m_desc.size() <= m_desc.size());
	return rPtr;
}

TreedCQRImp * TreedCQRImp::symDiff(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	uint32_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			r.m_desc.push_back(oCD);
			if (oCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+oCD.treeBegin, o.m_trees.cbegin()+oCD.treeEnd);
				r.m_desc.back().treeEnd = r.m_trees.size();
			}
			++oI;
			continue;
		}
		if (myCD.fullMatch && oCD.fullMatch) {
			continue;
		}
		else {//we definetly have to create a tree, no matter what
			uint32_t treeBegin = r.m_trees.size();
			r.m_trees.emplace_back(FlatNode::T_SYM_DIFF);
			r.m_trees[treeBegin].opNode.childA = 1;
			
			if (myCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), m_trees.front() + myCD.treeBegin, m_trees.front() + myCD.treeEnd);
				r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
			}
			else {
				if (myCD.fullMatch) {
					r.m_trees.emplace_back(FlatNode::T_FM_LEAF);
					r.m_trees.back().fmNode.cellId = myCellId;
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = myCellId;
					r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
				}
				r.m_trees[treeBegin].opNode.childB = 2;
			}
		
			if (oCD.hasTree()) {
				r.m_trees.insert(r.m_trees.end(), o.m_trees.front() + oCD.treeBegin, o.m_trees.front() + oCD.treeEnd);
			}
			else {
				if (oCD.fullMatch) {
					r.m_trees.emplace_back(FlatNode::T_FM_LEAF);
					r.m_trees.back().fmNode.cellId = oCellId;
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = oCellId;
					r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_desc.size());
		}
		++myI;
		++oI;
	}
	//unite the rest
	
	if (myI < myEnd) {
		uint32_t rDescBegin = r.m_desc.size();
		uint32_t rTreesBegin = r.m_trees.size();
		uint32_t lastTreeBegin = m_desc[myI].treeBegin;
		int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
		//adjust tree pointers
		for(uint32_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
			CellDesc & cd = r.m_desc[i];
			if (cd.hasTree()) {
				cd.treeBegin = ((int64_t)(cd.treeBegin) + treeOffsetCorrection);
				cd.treeEnd = ((int64_t)(cd.treeEnd) + treeOffsetCorrection);
			}
		}
	}
	else if (oI < oEnd) {
		uint32_t rDescBegin = r.m_desc.size();
		uint32_t rTreesBegin = r.m_trees.size();
		uint32_t lastTreeBegin = o.m_desc[oI].treeBegin;
		int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin
		r.m_desc.insert(r.m_desc.end(), o.m_desc.cbegin()+oI, o.m_desc.cend());
		r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+m_desc[oI].treeBegin, o.m_trees.cend());
		//adjust tree pointers
		for(uint32_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
			CellDesc & cd = r.m_desc[i];
			if (cd.hasTree()) {
				cd.treeBegin = ((int64_t)(cd.treeBegin) + treeOffsetCorrection);
				cd.treeEnd = ((int64_t)(cd.treeEnd) + treeOffsetCorrection);
			}
		}
	}
	
	assert(r.m_desc.size() <= m_desc.size() + o.m_desc.size());
	return rPtr;
}

}}}//end namespace