#include <sserialize/containers/ItemIndexIteratorIntersecting.h>

namespace sserialize {

ItemIndexIteratorIntersecting::ItemIndexIteratorIntersecting() : 
m_val(0),
m_valid(false)
{}

ItemIndexIteratorIntersecting::ItemIndexIteratorIntersecting(const std::deque< ItemIndexIterator >& intersect) :
m_idx(intersect),
m_valid(true)
{
	moveToNext();
}

ItemIndexIteratorIntersecting::~ItemIndexIteratorIntersecting() {}

uint32_t ItemIndexIteratorIntersecting::maxSize() const {
	uint32_t minMaxSize = 0xFFFFFFFF;
	for(size_t i = 0; i < m_idx.size(); i++) {
		minMaxSize = std::min(m_idx[i].maxSize(), minMaxSize);
	}
	return minMaxSize;
}

bool ItemIndexIteratorIntersecting::valid() const {
	return m_valid;
}

uint32_t ItemIndexIteratorIntersecting::operator*() const {
	return m_val;
}

void ItemIndexIteratorIntersecting::next() {
	if (m_valid) {
		moveToNext();
	}
}

void ItemIndexIteratorIntersecting::reset() {
	for(size_t i = 0; i < m_idx.size(); i++) {
		m_idx[i].reset();
	}
	m_valid = true;
	moveToNext();
}


ItemIndexIteratorPrivate* ItemIndexIteratorIntersecting::copy() const {
	ItemIndexIteratorIntersecting * newIdx = new ItemIndexIteratorIntersecting();
	newIdx->m_idx = m_idx;
	newIdx->m_val = m_val;
	newIdx->m_valid = m_valid;
	return newIdx;
}

void ItemIndexIteratorIntersecting::moveToNext()  {
	while (true) {
		uint32_t smallest = 0xFFFFFFFF;
		bool allValid = true;
		for(size_t i = 0; i < m_idx.size(); i++) {
			allValid &= m_idx[i].valid();
			smallest = std::min(smallest, *(m_idx[i]));
		}
		if (!allValid)
			break;
		
		size_t moveCount = 0;
		for(size_t i = 0; i < m_idx.size(); i++) {
			if (*(m_idx[i]) == smallest) {
				++m_idx[i];
				allValid &= m_idx[i].valid();
				moveCount++;
			}
		}
		
		if (moveCount == m_idx.size()) {
			m_val = smallest;
			return;
		}

		if (!allValid)
			break;
	}
	m_valid = false;
	m_val = 0;
}




}//end namespace