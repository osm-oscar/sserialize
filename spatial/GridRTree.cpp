#include <sserialize/spatial/GridRTree.h>
#include <sserialize/spatial/ItemGeoGrid.h>
#include <iostream>

namespace sserialize {
namespace spatial {

static uint32_t primes[12] = {2,3,5,7,11,13,17,19,23,29,31,37};

uint32_t smallest12PrimesDivisor(uint32_t num) {
	for(uint32_t i = 0; i < 12; ++i)
		if (num % primes[i] == 0)
			return primes[i];
	return 1;
}

RTree::Node * createTree(ItemGeoGrid & grid, uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax) {
	uint32_t xcount = xmax-xmin+1;
	uint32_t ycount = ymax-ymin+1;
	if (xcount == 1 && ycount == 1) { //we have reached the end, create leafNode) {
		std::set<uint32_t>* &s = grid.storage().at(grid.selectBin(xmin,ymin));
		if (s) {
			RTree::LeafNode * ln = new RTree::LeafNode();
			ln->leafNode = true;
			ln->rect = grid.cellBoundary(xmin,ymin);
			ln->items = std::vector<uint32_t>(s->begin(), s->end());
			delete s;
			s = 0;
			return ln;
		}
	}
	else if (xcount >= 1 && ycount >= 1) { //split in half
		RTree::InnerNode * in = new RTree::InnerNode();
		in->leafNode = false;
		if (xcount >= 2 && ycount >= 2) { //for children
			RTree::Node * node = createTree(grid, xmin, ymin, xmin+xcount/2, ymin+ymin/2); //bottom left
			if (node)
				in->children.push_back(node);
			node = createTree(grid, xmin+xcount/2+1, ymin, xmax, ymin+ymin/2); //bottom right
			if (node)
				in->children.push_back(node);
			node = createTree(grid, xmin, ymin+ycount/2+1, xmin+xcount/2, ymax); //top left
			if (node)
				in->children.push_back(node);
			node = createTree(grid, xmin+xcount/2+1, ymin+ycount/2+1, xmax, ymax); //top right
			if (node)
				in->children.push_back(node);
		}
		else if (xcount >= 2) {
			RTree::Node * node = createTree(grid, xmin, ymin, xmin+xcount/2, ymax); //left child
			if (node)
				in->children.push_back(node);
			node = createTree(grid, xmin+xcount/2+1, ymin, xmax, ymax); //right child
			if (node)
				in->children.push_back(node);
		}
		else { //ycount >= 2
			RTree::Node * node = createTree(grid, xmin, ymin, xmax, ymin+ycount/2); //bottom child
			if (node)
				in->children.push_back(node);
			node = createTree(grid, xmin, ymin+ycount/2+1, xmax, ymax); //top child
			if (node)
				in->children.push_back(node);
		}
		
		
		if (in->children.size() > 1) {
			//calculate bbox
			in->rect = in->children.front()->rect;
			for(uint32_t i = 1; i < in->children.size(); ++i)
				in->rect.enlarge(in->children[i]->rect);
			return in;
		}
		else if (in->children.size() == 1) {
			RTree::Node * n = in->children.front();
			delete in;
			return n;
		}
		else
			delete in;
	}
	return 0;
}

void GridRTree::bulkLoad(const sserialize::spatial::GridRTree::BulkLoadType & idToRect) {
	ItemGeoGrid grid(m_bbox, m_latCount, m_lonCount);
	std::cout << "GridRTree::buildLoad: creating grid..." << std::flush;
	for(BulkLoadType::const_iterator it(idToRect.begin()); it != idToRect.end(); ++it) {
		grid.addItem(it->first, it->second);
	}
	std::cout << "done" << std::endl;
	rootNode() = createTree(grid, 0, 0, m_latCount-1, m_lonCount-1);
}


}}//end namespace