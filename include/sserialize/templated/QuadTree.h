#ifndef SSERIALIZE_QUAD_TREE_H
#define SSERIALIZE_QUAD_TREE_H
#include <set>
#include <sserialize/utility/UByteArrayAdapter.h>


/*
 *
 *
 *-------
 * 10|11|
 *------|
 * 00|01|
 *-------
 */
 
namespace sserialize {
 
/** @param ItemType: ItemType has to implement a comparison operator for Point such that value < Point = true, if values position*/
template<typename NumberType, typename ItemType>
class QuadTree {
public:
	class Point {
		NumberType m_lat;
		NumberType m_lon;
	public:
		Point(NumberType x, NumberType y) : m_lat(x), m_lon(y) {}
		NumberType & lat() { return m_lat;}
		NumberType & lon() { return m_lon;}
		NumberType & x() { return m_lat;}
		NumberType & y() { return m_lon;}
	};

	class Node {
		enum ChildPosition {CP_BOTTOM_LEFT=0, CP_BOTTOM_RIGHT=1, CP_TOP_LEFT=2, CP_TOP_RIGHT=3, CP_OUT_OF_BOUNDS=4};
		Node * m_parent;
		Node* m_children[4];
		Point m_bottomLeft;
		Point m_topRight;
		std::set<ItemType> m_values;
	public:
		Node(Node * parent, const Point & bottomLeft, const Point & topRight) : 
			m_parent(parent),
			m_bottomLeft(bottomLeft),
			m_topRight(topRight)
		{
			for(size_t i = 0; i < 4; i++)
				m_children[i] = 0;
		}
		~Node() {
			for(size_t i = 0; i < 4; i++)
				if (m_children[i])
					delete m_children[i];
		}

		ChildPosition childPos(const Point & p) const {
			if (p.x() < bottomLeft().x() || p.x() > topRight.x() || p.y() < bottomLeft().y() || p.y() > topRight.y()) {
				return CP_OUT_OF_BOUNDS;
			}
			Point splitPoint((topRight().x()-bottomLeft().x())/2, (topRight().y()-bottomLeft().y())/2);
			ChildPosition childPos= CP_BOTTOM_LEFT;
			if (p.x() > splitPoint.x()) {//right
				childPos |= 1;
			}
			if (p.y() > splitPoint.y()) { //top
				childPos |= 1;
			}
			return childPos;
		}

		/** @return: childnode, that encloses p. Function create new node if necessary */
		Node* operator[](const Point p, bool createNew=true) {
			if (p.x() < bottomLeft().x() || p.x() > topRight.x() || p.y() < bottomLeft().y() || p.y() > topRight.y()) {
				return 0;
			}
			Point splitPoint((topRight().x()-bottomLeft().x())/2, (topRight().y()-bottomLeft().y())/2);
			ChildPosition childPos= CP_BOTTOM_LEFT;
			if (p.x() > splitPoint.x()) {//right
				childPos |= 1;
			}
			if (p.y() > splitPoint.y()) { //top
				childPos |= 1;
			}
			
			//check if we need to create it
			if (!m_children[childPos] && createNew) {
				Point newBottomLeft;
				Point newTopRight;
				if (childPos & 0x1) { //right 
					newBottomLeft.x() = splitPoint.x();
					newTopRight.x() = topRight().x();
				}
				else {//left
					newBottomLeft.x() = bottomLeft().x();
					newTopRight.x() = splitPoint().x();
				}

				if (childPos & 0x2) { //top
					newBottomLeft.y() = splitPoint.y();
					newTopRight.y() = topRight().y();
				}
				else { //bottom
					newBottomLeft.y() = bottomLeft().y();
					newTopRight.y() = splitPoint.y();
				}
				m_children[childPos] = new Node(this, newBottomLeft, newTopRight);
			}
			return m_children[childPos];
		}
		Node* &operator[](uint8_t pos) { return m_children[pos];}
		Node* parent() { return m_parent;}
		const Point & bottomLeft() const { return m_bottomLeft;}
		const Point & topRight() const { return m_topRight;}
		std::deque<ItemType> & values() { return m_values;}
		/** @return returns all Items that are within this boundary box*/
		void intersect(const Point & bottomLeft, const Point & topRight, std::set<ItemType> & destination) const {
			
			uint8_t bLPos = childPos(bottomLeft);
			uint8_t tRPos = childPos(topRight);

			if (bLPos == tRPos) {
				if (m_children[bLPos] != 0)
					m_children[bLPos]->intersect(bottomLeft, topRight, destination);
			}
			else {
				for(uint8_t xPos = (bLPos & 0x1); xPos <= (tRPos & 0x1); xPos++) {
					for(uint8_t yPos = ((bLPos & 0x2) >> 1); yPos <= ((tRPos & 0x2) >> 1); yPos++) {
						uint8_t childPos = xPos | (yPos << 1);
						if (m_children[childPos])
							m_children[childPos]->intersect(bottomLeft, topRight, destination);
					}
				}
			}
			//Intersect own
			for(size_t i = 0; i < m_values.size(); i++) {
				if (m_values[i] < topRight && m_values[i] > bottomLeft) {
					destination.insert(m_values[i]);
				}
			}
		}
	};


private:
	uint8_t m_maxDepth;
	Node * m_root;
public:
	QuadTree(NumberType lat, NumberType y, uint8_t maxDepth);
	~QuadTree();
	Node* operator[](const Point & p);
	std::deque<ItemType> intersect(const Point & bottomLeft, const Point & topRight);
};

}

UByteArrayAdapter& operator<<(UByteArrayAdapter & destination, const sserialize::QuadTree & qt);

#endif