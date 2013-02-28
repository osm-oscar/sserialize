#include "ItemIndexIteratorSetOp.h"

namespace sserialize {


ItemIndexIteratorSetOp::ItemIndexIteratorSetOp() : 
m_type(OPT_UNITE)
{}

ItemIndexIteratorSetOp::ItemIndexIteratorSetOp(const sserialize::ItemIndexIterator & first, const sserialize::ItemIndexIterator & second, sserialize::ItemIndexIteratorSetOp::OpTypes type) :
ItemIndexIteratorPrivate(),
m_type(type),
m_first(first),
m_second(second),
m_valid(true)
{
	next();
}


ItemIndexIteratorSetOp::~ItemIndexIteratorSetOp() {}

uint32_t ItemIndexIteratorSetOp::maxSize() const {
	switch (m_type) {
	case (OPT_UNITE):
	case(OPT_XOR):
		return m_first.maxSize() + m_second.maxSize();
	case (OPT_INTERSECT):
		return std::min(m_first.maxSize(), m_second.maxSize());
	case(OPT_DIFF):
		return m_first.maxSize();
	default:
		return 0;
	};
}


uint32_t ItemIndexIteratorSetOp::operator*() const {
	return m_val;
}

bool ItemIndexIteratorSetOp::valid() const {
	return m_valid;
}

ItemIndexIteratorPrivate* ItemIndexIteratorSetOp::copy() const {
	ItemIndexIteratorSetOp * newPriv = new ItemIndexIteratorSetOp();
	newPriv->m_first = m_first;
	newPriv->m_second = m_second;
	newPriv->m_val = m_val;
	newPriv->m_valid = m_valid;
	newPriv->m_type = m_type;
	return newPriv;
}

void ItemIndexIteratorSetOp::next() {
	switch (m_type) {
	case (OPT_UNITE):
		uniteNext();
		break;
	case (OPT_INTERSECT):
		intersectNext();
		break;
	case(OPT_DIFF):
		diffNext();
		break;
	case(OPT_XOR):
		xorNext();
		break;
	default:
		break;
	};
}

void ItemIndexIteratorSetOp::reset() {
	m_first.reset();
	m_second.reset();
	m_valid = true;
	next();
}


void ItemIndexIteratorSetOp::uniteNext() {
	if (!m_first.valid() && !m_second.valid()) {
		m_valid = false;
		m_val = 0;
		return;
	}
	if (m_first.valid() && m_second.valid()) {
		if (*m_first < *m_second) {
			m_val = *m_first;
			++m_first;
		}
		else if (*m_first > *m_second) {
			m_val = *m_second;
			++m_second;
		}
		else {
			m_val = *m_first;
			++m_first;
			++m_second;
		}
	}
	else if (m_first.valid()) {
		m_val = *m_first;
		++m_first;
	}
	else if (m_second.valid()) {
		m_val = *m_second;
		++m_second;
	}
}

void ItemIndexIteratorSetOp::intersectNext() {
	while(m_first.valid() && m_second.valid()) {
		if (*m_first == *m_second) {
			m_val = *m_first;
			m_valid = true;
			++m_first;
			++m_second;
			return;
		}
		else if (*m_first < *m_second) {
			++m_first;
		}
		else {
			++m_second;
		}
	}
	m_val = 0;
	m_valid = false;
}

void ItemIndexIteratorSetOp::diffNext() {
	while (m_first.valid() && m_second.valid()) {
		if (*m_first == *m_second) {
			++m_first;
			++m_second;
		}
		else if (*m_first < *m_second) {
			m_valid = true;
			m_val = *m_first;
			++m_first;
			return;
		}
		else {
			++m_second;
		}
	}
	while (m_first.valid()) {
		m_val = *m_first;
		m_valid = true;
		++m_first;
		return;
	}
	m_val = 0;
	m_valid = false;
}

void ItemIndexIteratorSetOp::xorNext() {
	while (m_first.valid() && m_second.valid()) {
		if (*m_first == *m_second) {
			++m_first;
			++m_second;
		}
		else if (*m_first < *m_second) {
			m_val = *m_first;
			m_valid = true;
			++m_first;
			return;
		}
		else {
			m_val = *m_second;
			m_valid = true;
			++m_second;
			return;
		}
	}
	while(m_first.valid()) {
		m_val = *m_first;
		m_valid = true;
		++m_first;
		return;
	}
	while(m_second.valid()) {
		m_val = *m_second;
		m_valid = true;
		++m_second;
		return;
	}
	m_val = 0;
	m_valid = false;
}


}//end namespace