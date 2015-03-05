#include <sserialize/containers/TreedCQRImp.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/utility/UByteArrayAdapter.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

Node* TreeHandler::copy(const Node * src) const {
	if (!src) {
		return 0;
	}
	switch(src->type) {
	case Node::T_PM_LEAF:
		return NodeCaster<Node::T_PM_LEAF>::copy(src);
	case Node::T_FM_LEAF:
		return NodeCaster<Node::T_FM_LEAF>::copy(src);
	case Node::T_FETCHED_LEAF:
		return NodeCaster<Node::T_FETCHED_LEAF>::copy(src);
	case Node::T_INTERSECT:
	case Node::T_UNITE:
	case Node::T_DIFF:
	case Node::T_SYM_DIFF:
		{
			OpNode * opNode = NodeCaster<Node::T_INTERSECT>::cast(src);
			OpNode * cpNode = new OpNode(*opNode);
			cpNode->children[0] = copy(opNode->children[0]);
			cpNode->children[1] = copy(opNode->children[1]);
			return cpNode;
		}
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
	};
}

void TreeHandler::deleteTree(Node* src) {
	if (!src) {}
	switch(src->type) {
	case Node::T_PM_LEAF:
		delete NodeCaster<Node::T_PM_LEAF>::cast(src);
		break;
	case Node::T_FM_LEAF:
		delete NodeCaster<Node::T_FM_LEAF>::cast(src);
		break;
	case Node::T_FETCHED_LEAF:
		delete NodeCaster<Node::T_FETCHED_LEAF>::cast(src);
		break;
	case Node::T_INTERSECT:
	case Node::T_UNITE:
	case Node::T_DIFF:
	case Node::T_SYM_DIFF:
		{
			OpNode * opNode = NodeCaster<Node::T_INTERSECT>::cast(src);
			deleteTree(opNode->children[0], opNode->children[1]);
			delete opNode;
		}
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
	};
}

void TreeHandler::flattenOpNode(const sserialize::detail::TreedCellQueryResult::Node* n, uint32_t cellId, sserialize::ItemIndex& idx, FlattenResultType & frt) {
	sserialize::ItemIndex aIdx, bIdx;
	FlattenResultType frtA = FT_NONE;
	FlattenResultType frtB = FT_NONE;
	const OpNode * opNode = NodeCaster<Node::T_INTERSECT>::cast(n);
	flatten(opNode->children[0], aIdx, frtA);
	flatten(opNode->children[1], bIdx, frtB);
	assert(frtA != FT_NONE && frtB != FT_NONE);
	switch (n->type) {
	case Node::T_INTERSECT:
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
	case Node::T_UNITE:
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
	case Node::T_DIFF:
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
	case Node::T_SYM_DIFF:
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
void TreeHandler::flatten(const sserialize::detail::TreedCellQueryResult::Node* n, sserialize::ItemIndex& idx, bool& fullMatch) {
	if (!n) {}
	switch(n->type) {
	case Node::T_PM_LEAF:
		idx = m_idxStore.at(NodeCaster<Node::T_PM_LEAF>::cast(n)->idxId());
		break;
	case Node::T_FM_LEAF:
		fullMatch = true;
		break;
	case Node::T_FETCHED_LEAF:
		idx = NodeCaster<Node::T_FETCHED_LEAF>::cast(n)->idx();
		break;
	case Node::T_INTERSECT:
	case Node::T_UNITE:
	case Node::T_DIFF:
	case Node::T_SYM_DIFF:
		{
			flattenOpNode(n, idx, fullMatch);
		}
		break;
	default:
		throw sserialize::TypeMissMatchException("TreedQueryResult: invalid node type");
	};
}


Node* TreeHandler::flatten(const Node* n, uint32_t cellId)  {
	sserialize::ItemIndex res;
	bool fullMatch = false;
	flatten(n, res, fullMatch);
	if (fullMatch) {
		return new FMLeafNode(cellId);
	}
	else {
		return new FetchedLeafNode(res);
	}
}


TreedCQRImp::TreedCQRImp() :
m_idx(0)
{}

TreedCQRImp::TreedCQRImp(const ItemIndex & fmIdx, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend());

	uint32_t totalSize = fmIdx.size();
	m_desc.reserve(totalSize);
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.push_back( CellDesc(1, 0, *fmIt) );
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

TreedCQRImp::~TreedCQRImp() {

}

TreedCQRImp::CellDesc::CellDesc(TreedCQRImp::CellDesc && other) :
tree(other.tree)
{
	other.tree = 0;
	fullMatch = other.fullMatch;
	pmIdxId = other.pmIdxId;
	cellId = other.cellId;
}

TreedCQRImp::CellDesc::CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId, Node* tree) :
tree(tree)
{
	this->fullMatch = fullMatch;
	this->cellId = cellId;
	this->pmIdxId = pmIdxId;
}

TreedCQRImp::CellDesc::CellDesc(const TreedCQRImp::CellDesc& other) :
tree(TreeHandler::copy(other.tree))
{
	fullMatch = other.fullMatch;
	pmIdxId = other.pmIdxId;
	cellId = other.cellId;
}

TreedCQRImp::CellDesc::~CellDesc() {
	TreeHandler::deleteTree(tree);
}

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
		if (!myCD.tree && !oCD.tree) {
			uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
			switch(ct) {
			case 0x0: //both partial, create tree
				{
					OpNode * opNode = new OpNode(Node::T_INTERSECT);
					opNode->children[0] = new PMLeafNode(myCD.pmIdxId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
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
			OpNode * opNode = new OpNode(Node::T_INTERSECT);
			if (myCD.tree) {
				opNode->children[0] = TreeHandler::copy(myCD.tree);
			}
			else {
				if (myCD.fullMatch) {
					opNode->children[0] = new FMLeafNode(myCellId);
				}
				else {
					opNode->children[0] = new PMLeafNode(myCellId);
				}
			}
			if (oCD.tree) {
				opNode->children[1] = TreeHandler::copy(oCD.tree);
			}
			else {
				if (oCD.fullMatch) {
					opNode->children[1] = new FMLeafNode(oCellId);
				}
				else {
					opNode->children[1] = new PMLeafNode(oCellId);
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, opNode);
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
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			//do copy
			r.m_desc.push_back(oCD);
			++oI;
			continue;
		}
		if (!myCD.tree && !oCD.tree) {
			uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
			switch(ct) {
			case 0x0: //both partial, create tree
				{
					OpNode * opNode = new OpNode(Node::T_UNITE);
					opNode->children[0] = new PMLeafNode(myCD.pmIdxId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x2: //my full
			case 0x1: //o full
			case 0x3: //both full
				r.m_desc.emplace_back(1, myCellId, 0, 0);
				break;
			default:
				break;
			};
		}
		else { //at least one has a tree
			OpNode * opNode = new OpNode(Node::T_UNITE);
			if (myCD.tree) {
				opNode->children[0] = TreeHandler::copy(myCD.tree);
			}
			else {
				if (myCD.fullMatch) {
					opNode->children[0] = new FMLeafNode(myCellId);
				}
				else {
					opNode->children[0] = new PMLeafNode(myCellId);
				}
			}
			if (oCD.tree) {
				opNode->children[1] = TreeHandler::copy(oCD.tree);
			}
			else {
				if (oCD.fullMatch) {
					opNode->children[1] = new FMLeafNode(oCellId);
				}
				else {
					opNode->children[1] = new PMLeafNode(oCellId);
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, opNode);
		}
		++myI;
		++oI;
	}
	//unite the rest
	
	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		r.m_desc.push_back(myCD);
		++myI;
	}

	for(; oI < oEnd;) {
		const CellDesc & oCD = o.m_desc[oI];
		r.m_desc.push_back(oCD);
		++oI;
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
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			++oI;
			continue;
		}
		if (!myCD.tree && !oCD.tree) {
			uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
			switch(ct) {
			case 0x0: //both partial, create tree
				{
					OpNode * opNode = new OpNode(Node::T_DIFF);
					opNode->children[0] = new PMLeafNode(myCD.pmIdxId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x2: //my full, other is partial
				{
					OpNode * opNode = new OpNode(Node::T_DIFF);
					opNode->children[0] = new FMLeafNode(myCellId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x1: //o full
			case 0x3: //both full
			default:
				break;
			};
		}
		else { //at least one has a tree
			OpNode * opNode = new OpNode(Node::T_DIFF);
			if (myCD.tree) {
				opNode->children[0] = TreeHandler::copy(myCD.tree);
			}
			else {
				if (myCD.fullMatch) {
					opNode->children[0] = new FMLeafNode(myCellId);
				}
				else {
					opNode->children[0] = new PMLeafNode(myCellId);
				}
			}
			if (oCD.tree) {
				opNode->children[1] = TreeHandler::copy(oCD.tree);
			}
			else {
				if (oCD.fullMatch) {
					opNode->children[1] = new FMLeafNode(oCellId);
				}
				else {
					opNode->children[1] = new PMLeafNode(oCellId);
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, opNode);
		}
		++myI;
		++oI;
	}
	//unite the rest
	
	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		r.m_desc.push_back(myCD);
		++myI;
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
			++myI;
			continue;
		}
		else if ( oCellId < myCellId ) {
			//do copy
			r.m_desc.push_back(oCD);
			++oI;
			continue;
		}
		if (!myCD.tree && !oCD.tree) {
			uint32_t ct = (myCD.fullMatch << 1) | oCD.fullMatch;
			switch(ct) {
			case 0x0: //both partial, create tree
				{
					OpNode * opNode = new OpNode(Node::T_SYM_DIFF);
					opNode->children[0] = new PMLeafNode(myCD.pmIdxId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x2: //my full
				{
					OpNode * opNode = new OpNode(Node::T_SYM_DIFF);
					opNode->children[0] = new FMLeafNode(myCellId);
					opNode->children[1] = new PMLeafNode(oCD.pmIdxId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x1: //o full
				{
					OpNode * opNode = new OpNode(Node::T_SYM_DIFF);
					opNode->children[0] = new PMLeafNode(myCD.pmIdxId);
					opNode->children[1] = new FMLeafNode(oCellId);
					r.m_desc.emplace_back(0, myCellId, 0, opNode);
				}
				break;
			case 0x3: //both full
			default:
				break;
			};
		}
		else { //at least one has a tree
			OpNode * opNode = new OpNode(Node::T_SYM_DIFF);
			if (myCD.tree) {
				opNode->children[0] = TreeHandler::copy(myCD.tree);
			}
			else {
				if (myCD.fullMatch) {
					opNode->children[0] = new FMLeafNode(myCellId);
				}
				else {
					opNode->children[0] = new PMLeafNode(myCellId);
				}
			}
			if (oCD.tree) {
				opNode->children[1] = TreeHandler::copy(oCD.tree);
			}
			else {
				if (oCD.fullMatch) {
					opNode->children[1] = new FMLeafNode(oCellId);
				}
				else {
					opNode->children[1] = new PMLeafNode(oCellId);
				}
			}
			r.m_desc.emplace_back(0, myCellId, 0, opNode);
		}
		++myI;
		++oI;
	}
	//unite the rest
	
	for(; myI < myEnd;) {
		const CellDesc & myCD = m_desc[myI];
		r.m_desc.push_back(myCD);
		++myI;
	}

	for(; oI < oEnd;) {
		const CellDesc & oCD = o.m_desc[oI];
		r.m_desc.push_back(oCD);
		++oI;
	}
	assert(r.m_desc.size() <= m_desc.size() + o.m_desc.size());
	return rPtr;
}

}}}//end namespace