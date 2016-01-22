#include <sserialize/spatial/TreedCQRImp.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/assert.h>

/* Potential Reordering:

	(a+b) / d ==d klein=> (a/d) + (b/d)
	(a-b) / d ==d klein=> (a/d) - b
	(a/b) / d => (min(a,b,d) / max(a,b,d)) / mid(a,b,d)



*/
namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

void TreedCQRImp::flattenCell(const FlatNode * n, uint32_t cellId, sserialize::ItemIndex & idx, uint32_t & pmIdxId, FlattenResultType & frt) const {
	switch(n->common.type) {
	case FlatNode::T_PM_LEAF:
		pmIdxId = n->pmNode.pmIdxId;
		frt = FT_PM;
		return;
	case FlatNode::T_FETCHED_LEAF:
		frt = FT_FETCHED;
		idx = m_fetchedIdx.at(n->fetchedNode.internalIdxId);
		return;
	case FlatNode::T_FM_LEAF:
		frt = FT_FM;
		return;
	case FlatNode::T_TO_FM:
	{
		sserialize::ItemIndex aIdx;
		FlattenResultType frtA;
		uint32_t pmIdxIdA;
		flattenCell(n+1, cellId, aIdx, pmIdxIdA, frtA);
		if (frtA == FT_PM || (frtA == FT_FETCHED && aIdx.size())) {
			frtA = FT_FM;
		}
		//else: frtA is either FT_FM or FT_EMPTY
	}
		return;
	case FlatNode::T_INTERSECT:
	case FlatNode::T_DIFF:
	case FlatNode::T_UNITE:
	case FlatNode::T_SYM_DIFF:
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
	}
	
	//node is a opnode
	
	sserialize::ItemIndex aIdx, bIdx;
	FlattenResultType frtA, frtB;
	uint32_t pmIdxIdA, pmIdxIdB;
	flattenCell(n+1, cellId, aIdx, pmIdxIdA, frtA);
	flattenCell(n+n->opNode.childB, cellId, bIdx, pmIdxIdB, frtB);
	
	assert(frtA != FT_NONE && frtB != FT_NONE);
	
	switch (n->common.type) {
	case FlatNode::T_INTERSECT:
		if ((frtA | frtB) == FT_FM) { //both are FT_FM
			frt = FT_FM;
		}
		else { //at least one is partial or fetched
			if (frtA == FT_FM) {
				frt = frtB;
				idx = bIdx;
				pmIdxId = pmIdxIdB;
			}
			else if (frtB == FT_FM) {
				frt = frtA;
				idx = aIdx;
				pmIdxId = pmIdxIdA;
			}
			else {//both are partial or fetched TODO: how to optimize likely branches like this one
				if (frtA == FT_PM) {
					aIdx = m_idxStore.at(pmIdxIdA); 
				}
				if (frtB == FT_PM) {
					bIdx = m_idxStore.at(pmIdxIdB);
				}
				idx = aIdx / bIdx;
				frt = ((idx.size() > 0) ? FT_FETCHED : FT_EMPTY);
			}
		}
		break;
	case FlatNode::T_UNITE:
		if ((frtA | frtB) & FT_FM) { //at least one is full match
			frt = FT_FM;
		}
		else { //both are either pm or fetched
			if (frtA == FT_PM) {
				aIdx = m_idxStore.at(pmIdxIdA); 
			}
			if (frtB == FT_PM) {
				bIdx = m_idxStore.at(pmIdxIdB);
			}
			idx = aIdx + bIdx;
			frt = FT_FETCHED;
		}
		break;
	case FlatNode::T_DIFF:
		if (frtB == FT_FM || frtA == FT_EMPTY) { //right hand is fm -> result is empty
			frt = FT_EMPTY;
		}
		else { //frtB is either fetched or partial
			if (frtA == FT_PM) {
				aIdx = m_idxStore.at(pmIdxIdA); 
			}
			else if (frtA == FT_FM) {
				aIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			if (frtB == FT_PM) {
				bIdx = m_idxStore.at(pmIdxIdB);
			}
			idx = aIdx - bIdx;
			frt = ((idx.size() > 0) ? FT_FETCHED : FT_EMPTY);
		}
		break;
	case FlatNode::T_SYM_DIFF:
		if ((frtA | frtB) != FT_FM) { //this means at least one is a partial match or fetched
			if (frtA == FT_PM) {
				aIdx = m_idxStore.at(pmIdxIdA); 
			}
			else if (frtA == FT_FM) {
				aIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			if (frtB == FT_PM) {
				bIdx = m_idxStore.at(pmIdxIdB);
			}
			else if (frtB == FT_FM) {
				bIdx = m_idxStore.at( m_gh.cellItemsPtr(cellId) );
			}
			idx = aIdx ^ bIdx;
			frt = ((idx.size() > 0) ? FT_FETCHED : FT_EMPTY);
		}
		else { //result is empty
			frt = FT_EMPTY;
		}
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
		break;
	};
}


TreedCQRImp::TreedCQRImp() :
m_hasFetchedNodes(false)
{}

TreedCQRImp::TreedCQRImp(const ItemIndex & fmIdx, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore),
m_hasFetchedNodes(false)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend());

	uint32_t totalSize = fmIdx.size();
	m_desc.reserve(totalSize);
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, *fmIt, 0);
	}
}

TreedCQRImp::TreedCQRImp(bool fullMatch, uint32_t cellId, const GeoHierarchy & gh, const ItemIndexStore & idxStore, uint32_t cellIdxId) :
m_gh(gh),
m_idxStore(idxStore),
m_hasFetchedNodes(false)
{
	uint32_t totalSize = 1;
	m_desc.reserve(totalSize);
	m_desc.push_back( CellDesc((fullMatch ? (uint32_t)1 : 0), cellId, ((uint32_t)fullMatch ? 0 : cellIdxId) ) );
}

TreedCQRImp::TreedCQRImp(const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore),
m_hasFetchedNodes(false)
{}


TreedCQRImp::TreedCQRImp(
	const sserialize::ItemIndex & fmIdx,
	const sserialize::ItemIndex & pmIdx,
	std::vector<sserialize::ItemIndex>::const_iterator pmItemsIt,
	const GeoHierarchy & gh,
	const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore),
m_hasFetchedNodes(pmIdx.size())
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend()), pmIt(pmIdx.cbegin()), pmEnd(pmIdx.cend());

	uint32_t totalSize = fmIdx.size() + pmIdx.size();
	m_desc.reserve(totalSize);

	for(; fmIt != fmEnd && pmIt != pmEnd;) {
		uint32_t fCellId = *fmIt;
		uint32_t pCellId = *pmIt;
		if(fCellId <= pCellId) {
			m_desc.emplace_back(1, fCellId, 0);
			++fmIt;
		}
		else {
			m_desc.emplace_back(0, pCellId, 0);
			CellDesc & cd = m_desc.back();
			cd.hasFetchedNode = 1;
			
			//create the node
			cd.treeBegin = m_trees.size();
			m_trees.push_back(FlatNode::T_FETCHED_LEAF);
			cd.treeEnd = m_trees.size();
			
			//add the index
			m_trees.back().fetchedNode.internalIdxId = m_fetchedIdx.size();
			m_fetchedIdx.push_back(*pmItemsIt);
			
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.emplace_back(1, *fmIt, 0);
	}
	
	for(; pmIt != pmEnd; ++pmIt, ++pmItemsIt) {
		m_desc.emplace_back(0, *pmIt, 0);
		CellDesc & cd = m_desc.back();
		cd.hasFetchedNode = 1;
		
		//create the node
		cd.treeBegin = m_trees.size();
		m_trees.push_back(FlatNode::T_FETCHED_LEAF);
		cd.treeEnd = m_trees.size();
		
		//add the index
		m_trees.back().fetchedNode.internalIdxId = m_fetchedIdx.size();
		m_fetchedIdx.push_back(*pmItemsIt);
	}
}


TreedCQRImp::~TreedCQRImp() {}

TreedCQRImp::CellDesc::CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId) :
treeBegin(notree),
treeEnd(notree)
{
	this->fullMatch = fullMatch;
	this->cellId = cellId;
	this->pmIdxId = pmIdxId;
}

TreedCQRImp::CellDesc::CellDesc(const TreedCQRImp::CellDesc& other) :
treeBegin(other.treeBegin),
treeEnd(other.treeEnd)
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

TreedCQRImp::CellDesc& TreedCQRImp::CellDesc::operator=(const TreedCQRImp::CellDesc& other) {
	//can't we just use memmove to copy the whole struct?
	treeBegin = other.treeBegin;
	treeEnd = other.treeEnd;
	fullMatch = other.fullMatch;
	pmIdxId = other.pmIdxId;
	cellId = other.cellId;
	return *this;
}


TreedCQRImp * TreedCQRImp::intersect(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	std::size_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for( ;myI < myEnd && oI < oEnd; ) {
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
		else {
			if (!myCD.hasTree() && !oCD.hasTree()) {
				int ct = (myCD.fullMatch << 1) | oCD.fullMatch;
				switch(ct) {
				case 0x0: //both partial, create tree
					{
						std::size_t treeBegin = r.m_trees.size();
						r.m_trees.emplace_back(FlatNode::T_INTERSECT);
						{
							FlatNode & opNode = r.m_trees.back();
							opNode.opNode.childB = 2;
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
					r.m_desc.emplace_back(1, myCellId, 0);
					break;
				default:
					break;
				};
			}
			else { //at least one has a tree
				//if any of the two is a full match then we only need to copy the other tree TODO:take care of indexes
				if (myCD.fullMatch) { //copy tree from other
					r.m_desc.emplace_back(0, myCellId, 0);
					r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
					r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+oCD.treeBegin, o.m_trees.cbegin()+oCD.treeEnd);
					r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
				}
				else if (oCD.fullMatch) { //copy tree from this
					r.m_desc.emplace_back(0, oCellId, 0);
					r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
					r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
					r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
				}
				else { //this means at least one has a tree and the other is partial or has a tree as well

					std::size_t treeBegin = r.m_trees.size();
					r.m_trees.emplace_back(FlatNode::T_INTERSECT);
					
					if (myCD.hasTree()) {
						r.m_trees.insert(r.m_trees.end(), m_trees.cbegin() + myCD.treeBegin, m_trees.cbegin() + myCD.treeEnd);
						r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
					}
					else {
						r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
						r.m_trees.back().pmNode.cellId = myCellId;
						r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
						r.m_trees[treeBegin].opNode.childB = 2;
					}
				
					if (oCD.hasTree()) {
						r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin() + oCD.treeBegin, o.m_trees.cbegin() + oCD.treeEnd);
					}
					else {
						r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
						r.m_trees.back().pmNode.cellId = oCellId;
						r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
					}
					r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
				}
			}
			++myI;
			++oI;
		}
	}
	//unite the rest

	SSERIALIZE_CHEAP_ASSERT(r.m_desc.size() <= std::min<std::size_t>(m_desc.size(), o.m_desc.size()));
	return rPtr;
}

TreedCQRImp * TreedCQRImp::unite(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	std::size_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			r.m_desc.push_back(oCD);
			if (oCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+oCD.treeBegin, o.m_trees.cbegin()+oCD.treeEnd);
				r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
			}
			++oI;
			continue;
		}
		else {
			if (myCD.fullMatch || oCD.fullMatch) {
				r.m_desc.emplace_back(1, myCellId, 0);
			}
			else { //none is full match
				std::size_t treeBegin = r.m_trees.size();
				r.m_trees.emplace_back(FlatNode::T_UNITE);
				
				if (myCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), m_trees.cbegin() + myCD.treeBegin, m_trees.cbegin() + myCD.treeEnd);
					r.m_trees[treeBegin].opNode.childB = 1+myCD.treeSize();
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = myCellId;
					r.m_trees.back().pmNode.pmIdxId = myCD.pmIdxId;
					r.m_trees[treeBegin].opNode.childB = 2;
				}
			
				if (oCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin() + oCD.treeBegin, o.m_trees.cbegin() + oCD.treeEnd);
				}
				else {
					r.m_trees.emplace_back(FlatNode::T_PM_LEAF);
					r.m_trees.back().pmNode.cellId = oCellId;
					r.m_trees.back().pmNode.pmIdxId = oCD.pmIdxId;
				}
				r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
			}
			++myI;
			++oI;
		}
	}
	//unite the rest
	
	if (myI < myEnd) {
		std::size_t rDescBegin = r.m_desc.size();
		
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		//move to cell with a tree
		for(std::size_t s(r.m_desc.size()); rDescBegin < s && !r.m_desc[rDescBegin].hasTree(); ++rDescBegin, ++myI) {}
		
		if (myI < myEnd) {
			std::size_t rTreesBegin = r.m_trees.size();
			uint32_t lastTreeBegin = m_desc[myI].treeBegin;
			int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin

			r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
			//adjust tree pointers
			for(std::size_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
				CellDesc & cd = r.m_desc[i];
				if (cd.hasTree()) {
					cd.treeBegin = (uint32_t)((int64_t)(cd.treeBegin) - treeOffsetCorrection);
					cd.treeEnd = (uint32_t)((int64_t)(cd.treeEnd) - treeOffsetCorrection);
				}
			}
		}
	}
	else if (oI < oEnd) {
		std::size_t rDescBegin = r.m_desc.size();
		
		r.m_desc.insert(r.m_desc.end(), o.m_desc.cbegin()+oI, o.m_desc.cend());
		//move to cell with a tree
		for(std::size_t s(r.m_desc.size()); rDescBegin < s && !r.m_desc[rDescBegin].hasTree(); ++rDescBegin, ++oI) {}
		
		if (oI < oEnd) {
			std::size_t rTreesBegin = r.m_trees.size();
			uint32_t lastTreeBegin = o.m_desc[oI].treeBegin;
			int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin

			r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+o.m_desc[oI].treeBegin, o.m_trees.cend());
			//adjust tree pointers
			for(std::size_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
				CellDesc & cd = r.m_desc[i];
				if (cd.hasTree()) {
					cd.treeBegin = (uint32_t)((int64_t)(cd.treeBegin) - treeOffsetCorrection);
					cd.treeEnd = (uint32_t)((int64_t)(cd.treeEnd) - treeOffsetCorrection);
				}
			}
		}
	}
	SSERIALIZE_CHEAP_ASSERT(r.m_desc.size() >= std::max<std::size_t>(m_desc.size(), o.m_desc.size()));
	return rPtr;
}

TreedCQRImp * TreedCQRImp::diff(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	std::size_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		else {
			if (!oCD.fullMatch) {//we definetly have to create a tree, no matter what
				std::size_t treeBegin = r.m_trees.size();
				r.m_trees.emplace_back(FlatNode::T_DIFF);
				
				if (myCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), m_trees.cbegin() + myCD.treeBegin, m_trees.cbegin() + myCD.treeEnd);
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
					r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin() + oCD.treeBegin, o.m_trees.cbegin() + oCD.treeEnd);
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
				r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
			}
			++myI;
			++oI;
		}
	}
	//add the rest
	if (myI < myEnd) {
		std::size_t rDescBegin = r.m_desc.size();
		
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		//move to cell with a tree
		for(std::size_t s(r.m_desc.size()); rDescBegin < s && !r.m_desc[rDescBegin].hasTree(); ++rDescBegin, ++myI) {}
		
		if (myI < myEnd) {
			std::size_t rTreesBegin = r.m_trees.size();
			uint32_t lastTreeBegin = m_desc[myI].treeBegin;
			int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin

			r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
			//adjust tree pointers
			for(std::size_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
				CellDesc & cd = r.m_desc[i];
				if (cd.hasTree()) {
					cd.treeBegin = (uint32_t)((int64_t)(cd.treeBegin) - treeOffsetCorrection);
					cd.treeEnd = (uint32_t)((int64_t)(cd.treeEnd) - treeOffsetCorrection);
				}
			}
		}
	}

	SSERIALIZE_CHEAP_ASSERT(r.m_desc.size() <= m_desc.size());
	return rPtr;
}

TreedCQRImp * TreedCQRImp::symDiff(const TreedCQRImp * other) const {
	const TreedCQRImp & o = *other;
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	std::size_t myI(0), myEnd(m_desc.size()), oI(0), oEnd(o.m_desc.size());
	for(; myI < myEnd && oI < oEnd;) {
		const CellDesc & myCD = m_desc[myI];
		const CellDesc & oCD = o.m_desc[oI];
		uint32_t myCellId = myCD.cellId;
		uint32_t oCellId = oCD.cellId;
		if (myCellId < oCellId) {
			r.m_desc.push_back(myCD);
			if (myCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+myCD.treeBegin, m_trees.cbegin()+myCD.treeEnd);
				r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
			}
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			r.m_desc.push_back(oCD);
			if (oCD.hasTree()) {//adjust tree info
				r.m_desc.back().treeBegin = (uint32_t)r.m_trees.size();
				r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+oCD.treeBegin, o.m_trees.cbegin()+oCD.treeEnd);
				r.m_desc.back().treeEnd = (uint32_t)r.m_trees.size();
			}
			++oI;
			continue;
		}
		else {
			if (!myCD.fullMatch || !oCD.fullMatch) {//we definetly have to create a tree, no matter what
				std::size_t treeBegin = r.m_trees.size();
				r.m_trees.emplace_back(FlatNode::T_SYM_DIFF);
				
				if (myCD.hasTree()) {
					r.m_trees.insert(r.m_trees.end(), m_trees.cbegin() + myCD.treeBegin, m_trees.cbegin() + myCD.treeEnd);
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
					r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin() + oCD.treeBegin, o.m_trees.cbegin() + oCD.treeEnd);
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
				r.m_desc.emplace_back(0, myCellId, 0, treeBegin, r.m_trees.size());
			}
			++myI;
			++oI;
		}
	}
	//unite the rest
	
	if (myI < myEnd) {
		std::size_t rDescBegin = r.m_desc.size();
		
		r.m_desc.insert(r.m_desc.end(), m_desc.cbegin()+myI, m_desc.cend());
		//move to cell with a tree
		for(std::size_t s(r.m_desc.size()); rDescBegin < s && !r.m_desc[rDescBegin].hasTree(); ++rDescBegin, ++myI) {}
		
		if (myI < myEnd) {
			std::size_t rTreesBegin = r.m_trees.size();
			uint32_t lastTreeBegin = m_desc[myI].treeBegin;
			int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin

			r.m_trees.insert(r.m_trees.end(), m_trees.cbegin()+m_desc[myI].treeBegin, m_trees.cend());
			//adjust tree pointers
			for(std::size_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
				CellDesc & cd = r.m_desc[i];
				if (cd.hasTree()) {
					cd.treeBegin = (uint32_t)((int64_t)(cd.treeBegin) - treeOffsetCorrection);
					cd.treeEnd = (uint32_t)((int64_t)(cd.treeEnd) - treeOffsetCorrection);
				}
			}
		}
	}
	else if (oI < oEnd) {
		std::size_t rDescBegin = r.m_desc.size();
		
		r.m_desc.insert(r.m_desc.end(), o.m_desc.cbegin()+oI, o.m_desc.cend());
		//move to cell with a tree
		for(std::size_t s(r.m_desc.size()); rDescBegin < s && !r.m_desc[rDescBegin].hasTree(); ++rDescBegin, ++oI) {}
		
		if (oI < oEnd) {
			std::size_t rTreesBegin = r.m_trees.size();
			uint32_t lastTreeBegin = o.m_desc[oI].treeBegin;
			int64_t treeOffsetCorrection = (int64_t)lastTreeBegin - (int64_t)rTreesBegin; //rTreesBegin+treeOffsetCorrection = lastTreeBegin

			r.m_trees.insert(r.m_trees.end(), o.m_trees.cbegin()+o.m_desc[oI].treeBegin, o.m_trees.cend());
			//adjust tree pointers
			for(std::size_t i(rDescBegin), s(r.m_desc.size()); i < s; ++i) {
				CellDesc & cd = r.m_desc[i];
				if (cd.hasTree()) {
					cd.treeBegin = (uint32_t)((int64_t)(cd.treeBegin) - treeOffsetCorrection);
					cd.treeEnd = (uint32_t)((int64_t)(cd.treeEnd) - treeOffsetCorrection);
				}
			}
		}
	}
	
	SSERIALIZE_CHEAP_ASSERT(r.m_desc.size() <= m_desc.size() + o.m_desc.size());
	return rPtr;
}

TreedCQRImp* TreedCQRImp::allToFull() const {
	TreedCQRImp * rPtr = new TreedCQRImp(m_gh, m_idxStore);
	TreedCQRImp & r = *rPtr;
	r.m_desc.reserve(m_desc.size());
	r.m_trees.reserve(m_trees.size()+m_desc.size());
	
	for(std::size_t myI(0), myEnd(m_desc.size()); myI < myEnd; ++myI) {
		const CellDesc & myCD = m_desc[myI];
		std::size_t treeBegin = r.m_trees.size();
		if (myCD.hasTree()) {
			r.m_trees.insert(r.m_trees.end(), m_trees.cbegin() + myCD.treeBegin, m_trees.cbegin() + myCD.treeEnd);
			r.m_desc.emplace_back(0, myCD.cellId, 0, treeBegin, r.m_trees.size());
		}
		else {
			r.m_desc.emplace_back(1, myCD.cellId, 0);
		}
	}
	
	SSERIALIZE_CHEAP_ASSERT_EQUAL(m_desc.size(), rPtr->m_desc.size());
	
	return rPtr;
}

}}}//end namespace