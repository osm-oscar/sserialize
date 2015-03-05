#ifndef SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#define SSERIALIZE_TREED_CELL_QUERY_RESULT_PRIVATE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <memory>
#include <string.h>

namespace sserialize {
namespace detail {
namespace TreedCellQueryResult  {

	//TODO: Implement this with a flat tree aproach without pointers which should improve the speed a lot since then its all just sequential memory access

	union FlatNode {
		typedef enum {T_INVALID=0, T_PM_LEAF=1, T_FM_LEAF=2, T_FETCHED_LEAF=3, T_INTERSECT=4, T_UNITE=5, T_DIFF=6, T_SYM_DIFF=7} Type;
		uint64_t raw;
		struct {
			uint64_t type:3;
			uint64_t value:61;
		} common;
		struct {
			uint64_t type:3;
			uint64_t padding:1;
			uint64_t childA:30;//offset to child A from the position of THIS node of the tree
			uint64_t childB:30;//offset to child B from the position of THIS node of the tree
		} opNode;
		struct {
			uint64_t type:3;
			uint64_t padding:29;
			uint64_t pmIdxId:32;
		} pmNode;
		struct {
			uint64_t type:3;
			uint64_t padding:29;
			uint64_t cellId:32;
		} fmNode;
		struct {
			uint64_t type:3;
			uint64_t internalIdxId;
		} fetchedNode;
		FlatNode(Type t) {
			common.type = t;
		}
		FlatNode(Type t) {
			common.type = t;
		}
	};
	
	struct Node {
		typedef enum {T_INVALID=0, T_PM_LEAF=1, T_FM_LEAF=2, T_FETCHED_LEAF=3, T_INTERSECT=4, T_UNITE=5, T_DIFF=6, T_SYM_DIFF=7} Type;
		uint64_t type:3;
		uint64_t value:61;
		Node() {}
		Node(uint32_t type, uint64_t value) {
			this->type = type;
			this->value = value;
		}
		Node(const Node & other) {
			type = other.type;
			value = other.value;
		}
		Type type() const { return (Type) type; }
	};

	struct PMLeafNode: Node {
		PMLeafNode(uint32_t idxId) : Node(Node::T_PM_LEAF, idxId) {}
		PMLeafNode(PMLeafNode & other) : Node(other) {}
		inline uint32_t idxId() const { return Node::value; }
	};
	
	struct FMLeafNode: Node {
		FMLeafNode(uint32_t cellId) : Node(Node::T_FM_LEAF, cellId) {}
		FMLeafNode(const FMLeafNode & other) : Node(other) {}
		inline uint32_t cellId() const { return Node::value; }
	};
	
	struct FetchedLeafNode: Node {
		FetchedLeafNode(const sserialize::ItemIndex & idx) : Node(Node::T_FETCHED_LEAF), m_idx(idx) {}
		FetchedLeafNode(const FetchedLeafNode & other) : Node(other), m_idx(other.m_idx) {}
		sserialize::ItemIndex m_idx;
		const sserialize::ItemIndex & idx() const { return m_idx; }
	};
	
	struct OpNode: Node {
		///Does not copy ptrs
		OpNode(const OpNode & other) : Node(other) {}
		OpNode(Node::Type type) : Node(type, 0) {}
		Node * children[2]; //this always has 2 children as it is created by a call to an operator which takes 2 arguments
	};
	
	template<Node::Type nodeType>
	struct NodeTypeCalculator;
	
	#define NODE_TYPE_CALCULATOR_SPECIALIZATION(__ENUMTYPE, __NODETYPE) template<> struct NodeTypeCalculator<__ENUMTYPE> { typedef __NODETYPE type; };
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_PM_LEAF, PMLeafNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_FM_LEAF, FMLeafNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_FETCHED_LEAF, FetchedLeafNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_INTERSECT, OpNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_UNITE, OpNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_DIFF, OpNode)
	NODE_TYPE_CALCULATOR_SPECIALIZATION(Node::Type::T_SYM_DIFF, OpNode)
	#undef NODE_TYPE_CALCULATOR_SPECIALIZATION
	
	template<Node::Type nodeType>
	struct NodeCaster {
		typedef NodeTypeCalculator<nodeType> type;
		static type * cast(Node * base) { 
			return static_cast<type*>(base);
		}
		static const type * cast(const Node * base) { 
			return static_cast<type*>(base);
		}
		static type * copy(Node * base) {
			return new type(*cast(base));
		}
		static const type * copy(const Node * base) {
			return new type(*cast(base));
		}
	};
	
	class TreeHandler {
	public:
		typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
		typedef sserialize::Static::ItemIndexStore ItemIndexStore;
	private:
		typedef enum {FT_NONE=0x0, FT_PM=0x1, FT_FM=0x2, FT_FETCHED=0x4, FT_EMPTY=0x8} FlattenResultType;
	private:
		GeoHierarchy m_gh;
		ItemIndexStore m_idxStore;
	private:
		void flattenOpNode(const Node * n, uint32_t cellId, sserialize::ItemIndex & idx, FlattenResultType & frt);
		void flatten(const Node * n, uint32_t cellId, sserialize::ItemIndex & idx, FlattenResultType & frt);
	public:
		TreeHandler(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
		//flatten the node and adjust its type
		Node* flatten(const Node* n, uint32_t cellId);
		static Node * copy(const Node * src) const;
		static void deleteTree(Node * node);
	};

///Base class for the TreedCQR, which is a delayed set ops CQR. It delays the set operations of cells until calling finalize() which will return a CQR
class TreedCQRImp: public RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	typedef sserialize::Static::ItemIndexStore ItemIndexStore;
private:
	
	
	struct CellDesc {
		CellDesc(uint32_t fullMatch, uint32_t cellId, uint32_t pmIdxId, Node * tree);
		CellDesc(const CellDesc & other);
		CellDesc(CellDesc && other);
		~CellDesc();
		Node * tree;
		uint64_t fullMatch:1;
		uint64_t cellId:31;
		uint64_t pmIdxId:32;
	};
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::ItemIndexStore m_idxStore;
	std::vector<CellDesc> m_desc;
private:
	void uncheckedSet(uint32_t pos, const sserialize::ItemIndex & idx);
	TreedCQRImp(const GeoHierarchy & gh, const ItemIndexStore & idxStore);
public:
	TreedCQRImp();
	TreedCQRImp(const ItemIndex & fullMatches, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	TreedCQRImp(uint32_t cellId, uint32_t cellIdxId, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	inline const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const { return m_gh; }
	
	///@parameter fmBegin begining of the fully matched cells
	template<typename T_PMITEMSPTR_IT>
	TreedCQRImp(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
					T_PMITEMSPTR_IT pmItemsBegin, const GeoHierarchy & gh, const ItemIndexStore & idxStore);
	virtual ~TreedCQRImp();
	sserialize::ItemIndex::Types defaultIndexType() const { return m_idxStore.indexType(); }
	inline bool fullMatch(uint32_t pos) const { return m_desc[pos].fullMatch; }
	inline uint32_t cellId(uint32_t pos) const { return m_desc[pos].cellId;}
	inline uint32_t cellCount() const { return m_desc.size();}
	TreedCQRImp * intersect(const TreedCQRImp * other) const;
	TreedCQRImp * unite(const TreedCQRImp * other) const;
	TreedCQRImp * diff(const TreedCQRImp * other) const;
	TreedCQRImp * symDiff(const TreedCQRImp * other) const;
};

template<typename T_PMITEMSPTR_IT>
CellQueryResult::CellQueryResult(const sserialize::ItemIndex & fmIdx, const sserialize::ItemIndex & pmIdx,
				T_PMITEMSPTR_IT pmItemsIt, const GeoHierarchy & gh, const ItemIndexStore & idxStore) :
m_gh(gh),
m_idxStore(idxStore)
{
	sserialize::ItemIndex::const_iterator fmIt(fmIdx.cbegin()), fmEnd(fmIdx.cend()), pmIt(pmIdx.cbegin()), pmEnd(pmIdx.cend());

	uint32_t totalSize = fmIdx.size() + pmIdx.size();
	m_desc.reserve(totalSize);
	m_idx = (IndexDesc*) malloc(totalSize * sizeof(IndexDesc));
	IndexDesc * idxPtr = m_idx;

	for(; fmIt != fmEnd && pmIt != pmEnd; ++idxPtr) {
		uint32_t fCellId = *fmIt;
		uint32_t pCellId = *pmIt;
		if(fCellId <= pCellId) {
			m_desc.push_back(CellDesc(1, 0, fCellId));
			++fmIt;
		}
		else {
			m_desc.push_back(CellDesc(0, 0, pCellId));
			idxPtr->idxPtr = *pmItemsIt;
			++pmIt;
			++pmItemsIt;
		}
	}
	for(; fmIt != fmEnd; ++fmIt) {
		m_desc.push_back( CellDesc(1, 0, *fmIt) );
	}
	
	for(; pmIt != pmEnd; ++idxPtr, ++pmIt, ++pmItemsIt) {
		m_desc.push_back(CellDesc(0, 0, *pmIt));
		idxPtr->idxPtr = *pmItemsIt;
	}

}


}}//end namespace


#endif