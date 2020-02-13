#pragma once

#include <sserialize/Static/ItemIndexStore.h>

#include <sserialize/spatial/dgg/HCQR.h>

namespace sserialize {
	class ItemIndexFactory;
	class CellQueryResult;
}

namespace sserialize::spatial::dgg::impl::detail::HCQRSpatialGrid {

/**
	* We assume the following: 
	* A Node is either an internal node and only has children OR a leaf node.
	* A Node is either a full-match node or a partial-match node
	* 
	*/
class TreeNode final {
public:
	using Children = std::vector<std::unique_ptr<TreeNode>>;
	using PixelId = sserialize::spatial::dgg::interface::SpatialGrid::PixelId;
	enum : int {NONE=0x0, IS_INTERNAL=0x1, IS_PARTIAL_MATCH=0x2, IS_FULL_MATCH=0x4, IS_FETCHED=0x8} Flags;
	static constexpr uint32_t npos = std::numeric_limits<uint32_t>::max();
public:
	TreeNode();
	TreeNode(TreeNode const &) = delete;
	~TreeNode() {}
	//copies flags, pixelId and itemIndexId if IS_FETCHED is false 
	std::unique_ptr<TreeNode> shallowCopy() const; 
	//copies flags, pixelId if IS_FETCHED is true and sets the new fetchedItemIndexId 
	std::unique_ptr<TreeNode> shallowCopy(uint32_t fetchedItemIndexId) const;
	//copies flags, pixelId if isInternal() is true
	std::unique_ptr<TreeNode> shallowCopy(Children && newChildren) const; 
public:
	static std::unique_ptr<TreeNode> make_unique(PixelId pixelId, int flags, uint32_t itemIndexId = npos);
public:
	inline PixelId pixelId() const { return m_pid; }
	inline bool isInternal() const { return children().size(); }
	inline bool isLeaf() const { return !children().size(); }
	inline bool isFullMatch() const { return m_f & IS_FULL_MATCH; }
	inline bool isFetched() const { return m_f & IS_FETCHED; }

	inline uint32_t itemIndexId() const { return m_itemIndexId; }
	///children HAVE to be sorted according to their pixelId
	inline Children const & children() const { return m_children; }
	inline int flags() const { return m_f; }
public:
	inline void setItemIndexId(uint32_t id) { m_itemIndexId = id; }
	inline void setFlags(int f) { m_f = f; }
	inline Children & children() { return m_children; }
public:
	void sortChildren();
public:
	bool valid() const;
private:
	TreeNode(PixelId pixelId, int flags, uint32_t itemIndexId);
private:
	PixelId m_pid;
	int m_f;
	uint32_t m_itemIndexId;
	Children m_children;
};
	
} //end namespace sserialize::spatial::dgg::impl::detail::HCQRSpatialGrid

namespace sserialize::spatial::dgg::impl {

///In memory variant
class HCQRSpatialGrid: public sserialize::spatial::dgg::interface::HCQRSpatialGrid {
public:
    using Parent = interface::HCQRSpatialGrid;
    using Self = sserialize::spatial::dgg::impl::HCQRSpatialGrid;
	using TreeNode = detail::HCQRSpatialGrid::TreeNode;
	using TreeNodePtr = std::unique_ptr<TreeNode>;
public:
    HCQRSpatialGrid(
        sserialize::Static::ItemIndexStore idxStore,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
    );
    HCQRSpatialGrid(
		sserialize::CellQueryResult const & cqr,
        sserialize::Static::ItemIndexStore idxStore,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
    );
    HCQRSpatialGrid(
		TreeNodePtr && root,
        sserialize::Static::ItemIndexStore idxStore,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGrid> sg,
        sserialize::RCPtrWrapper<sserialize::spatial::dgg::interface::SpatialGridInfo> sgi
    );
    ~HCQRSpatialGrid() override;
public:
    SizeType depth() const override;
    SizeType numberOfItems() const override;
	SizeType numberOfNodes() const override;
    ItemIndex items() const override;
public:
    HCQRPtr operator/(HCQR const & other) const override;
    HCQRPtr operator+(HCQR const & other) const override;
    HCQRPtr operator-(HCQR const & other) const override;
public:
    HCQRPtr compactified(SizeType maxPMLevel = 0) const override;
    HCQRPtr expanded(SizeType level) const override;
    HCQRPtr allToFull() const override;
public:
	TreeNodePtr const & root() const;
public:
    sserialize::ItemIndex items(TreeNode const & node) const;
	PixelLevel level(TreeNode const & node) const;
	void flushFetchedItems(sserialize::ItemIndexFactory & idxFactory);
public:
    sserialize::Static::ItemIndexStore const & idxStore() const { return m_items; }
    auto const & fetchedItems() const { return m_fetchedItems; }
private:
    struct HCQRSpatialGridOpHelper;
private:
    TreeNodePtr m_root;
    sserialize::Static::ItemIndexStore m_items;
    std::vector<sserialize::ItemIndex> m_fetchedItems;
};

} //end namespace sserialize::spatial::dgg::impl
