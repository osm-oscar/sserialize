#ifndef SSERIALIZE_UNION_FIND_H
#define SSERIALIZE_UNION_FIND_H
#include <vector>
#include <limits>
#include <stdexcept>

namespace sserialize {

template<typename TValueType>
class UnionFind;

namespace detail {
namespace UnionFind {

template<typename TValueType, typename THandleType>
struct Entry {
	typedef TValueType value_type;
	typedef THandleType handle_type;
	value_type value;
	///by definition: single-element sets point to themselves!
	uint64_t parent:40;
	///rank of this element, grows only logarithmical
	uint64_t rank:24;
	Entry(handle_type parent, const value_type & v = value_type()) : value(v), parent(parent), rank(0) {}
	Entry(handle_type parent, value_type && v) : value(std::move(v)), parent(parent), rank(0) {}
	template<typename... TArgs>
	Entry(handle_type parent, TArgs... args) : value(std::forward<TArgs>(args)...), parent(parent), rank(0) {}
};

template<typename TValueType, typename THandleType>
class Iterator final {
public:
	typedef TValueType value_type;
	typedef THandleType handle_type;
public:
	Iterator() : m_d(0), m_p(std::numeric_limits<handle_type>::max()) {}
	Iterator(sserialize::UnionFind<TValueType> * d, handle_type p) : m_d(d), m_p(p) {}
	~Iterator() {}
	value_type & value() { return m_d->get(m_p); }
	const value_type & value() const { return m_d->get(m_p); }
	handle_type handle() const { return m_p; }
	bool operator!=(const Iterator & other) const { return m_p != other.m_p || m_d != other.m_d; }
	bool operator==(const Iterator & other) const { return m_p == other.m_p && m_d == other.m_d; }
	Iterator & operator++() { ++m_p; return *this; }
private:
	sserialize::UnionFind<value_type> * m_d;
	handle_type m_p;
};

}}//end namespace detail::UnionFind

/** A simple union-find data structure using a std::vector as backend.
  * It uses path-compression and union-by-rank
  * Needs 8 Bytes per entry
  */

template<typename TValueType>
class UnionFind final {
public:
	typedef TValueType value_type;
	typedef uint64_t handle_type;
	typedef uint32_t rank_type;
	
	typedef detail::UnionFind::Iterator<TValueType, handle_type> iterator;
public:
	UnionFind();
	~UnionFind();
public: //storage
	std::size_t size() const;
	void reserve(std::size_t size);
public: //value type interaction
	value_type const & get(handle_type a) const;
	value_type & get(handle_type a);
	handle_type make_set(const value_type & v);
	handle_type make_set(value_type && v);
	template<typename... TArgs>
	handle_type make_set(TArgs... args);
public: //union find operations
	void unite(handle_type a, handle_type b);
	///return the representative for the set a
	handle_type find(handle_type a);
public:
	inline iterator begin() { return iterator(this, 0); }
	inline iterator end() { return iterator(this, size()); }
private:
	typedef detail::UnionFind::Entry<value_type, handle_type> Entry;
	typedef std::vector<Entry> StorageContainer;
private:
	   handle_type find_and_compress(handle_type a);
private:
	StorageContainer m_d;
};

template<typename TValueType>
UnionFind<TValueType>::UnionFind() {}

template<typename TValueType>
UnionFind<TValueType>::~UnionFind() {}

template<typename TValueType>
std::size_t 
UnionFind<TValueType>::size() const {
	return m_d.size();
}

template<typename TValueType>
void
UnionFind<TValueType>::reserve(std::size_t size) {
	m_d.reserve(size);
}

template<typename TValueType>
typename UnionFind<TValueType>::value_type &
UnionFind<TValueType>::get(handle_type a) {
	if (a >= size()) {
		throw std::out_of_range("UnionFind with __a=" + std::to_string(a) + " but size()=" + std::to_string(size()));
	}
	return m_d[a].value;
}

template<typename TValueType>
typename UnionFind<TValueType>::value_type const &
UnionFind<TValueType>::get(handle_type a) const {
	if (a >= size()) {
		throw std::out_of_range("UnionFind with __a=" + std::to_string(a) + " but size()=" + std::to_string(size()));
	}
	return m_d[a].value;
}

template<typename TValueType>
typename UnionFind<TValueType>::handle_type
UnionFind<TValueType>::make_set(const value_type& v) {
	std::size_t p = m_d.size();
	m_d.emplace_back(p, v);
	return p;
}

template<typename TValueType>
typename UnionFind<TValueType>::handle_type
UnionFind<TValueType>::make_set(value_type&& v) {
	std::size_t p = m_d.size();
	m_d.emplace_back(p, std::move(v));
	return p;
}

template<typename TValueType>
template<typename... TArgs>
typename UnionFind<TValueType>::handle_type
UnionFind<TValueType>::make_set(TArgs... args) {
	std::size_t p = m_d.size();
	m_d.emplace_back(p, std::forward<TArgs>(args)...);
	return p;
}

template<typename TValueType>
typename UnionFind<TValueType>::handle_type
UnionFind<TValueType>::find(handle_type a) {
	if (a >= size()) {
		throw std::out_of_range("UnionFind with __a=" + std::to_string(a) + " but size()=" + std::to_string(size()));
	}
	return find_and_compress(a);
}

template<typename TValueType>
void
UnionFind<TValueType>::unite(handle_type a, handle_type b) {
	handle_type pa = find(a);
	handle_type pb = find(b);
	if (pa == pb) {
		return;
	}
	Entry & epa = m_d[pa];
	Entry & epb = m_d[pb];
	if (epa.rank < epb.rank) {
		epa.parent = pb;
	}
	else if (epa.rank > epb.rank) {
		epb.parent = pa;
	}
	else {
		epa.parent = pb;
		epb.rank += 1;
	}
}

template<typename TValueType>
typename UnionFind<TValueType>::handle_type
UnionFind<TValueType>::find_and_compress(handle_type a) {
	Entry & e = m_d[a];
	if (e.parent != a) {
		e.parent = find_and_compress(e.parent);
	}
	return e.parent;
}

}//end namespace

#endif