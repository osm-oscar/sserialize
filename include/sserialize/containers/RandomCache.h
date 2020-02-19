#ifndef SSERIALIZE_RANDOM_CACHE_H
#define SSERIALIZE_RANDOM_CACHE_H
#include <sserialize/utility/assert.h>
#include <unordered_map>
#include <deque>
#include <random>

namespace sserialize {

template<typename TKey, typename TValue>
class RandomCache {
public:
	using key_type = TKey;
	using value_type = TValue;
	using size_type = std::size_t;
public:
	RandomCache() : m_maxSize(16) {
		std::uniform_int_distribution<size_type> d;
		std::default_random_engine g;
		m_a = d(g);
		m_b = d(g);
		m_x = d(g);
	}
	~RandomCache() {}
	
	void setSize(size_type size) {
		m_maxSize = size;
		if (m_maxSize == 0) {
			m_maxSize = 1;
		}
		while (this->size() > maxSize()) {
			evict();
		}
	}

	size_type maxSize() const {
		return m_maxSize;
	}

	size_type size() const {
		return m_kv.size();
	}
	
	void insert(const TKey & key, const TValue & value) {
		if (size()+1 >= m_maxSize) {
			m_x = (m_a*m_x+m_b)%size();
			m_kv.erase(m_k.at(m_x));
			m_k.at(m_x) = key;
			m_kv[key] = value;
		}
		else {
			m_kv[key] = value;
			m_k.push_back(key);
		}
	}
	
	
	void evict() {
		m_x = (m_a*m_x+m_b)%size();
		key_type & k = m_k.at(m_x);
		m_kv.erase(k);
		if (m_k.size() > 1) {
			k = std::move(m_k.back());
		}
		m_k.pop_back();
	}
	
	TValue const & find(key_type const & key) {
		return m_kv.at(key);
	}

	bool contains(key_type const & key) {
		return count(key);
	}
	
	size_type count(key_type const & key) {
		return m_kv.count(key);
	}

	void clear() {
		m_kv.clear();
		m_k.clear();
	}
private:
	size_type m_maxSize;
	std::unordered_map<key_type, value_type> m_kv;
	std::vector<key_type> m_k;
	///lcg
	size_type m_a;
	size_type m_b;
	size_type m_x;
};

}//end namespace sserialize

#endif
