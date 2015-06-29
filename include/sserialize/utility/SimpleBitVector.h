#ifndef SSERIALIZE_SIMPLE_BIT_VECTOR_H
#define SSERIALIZE_SIMPLE_BIT_VECTOR_H
#include <vector>

namespace sserialize {

class SimpleBitVector {
	std::vector<uint64_t> m_d;
public:
	inline void resize(uint32_t count) { m_d.resize(count/64+1, 0); }
	inline void set(uint32_t pos) { m_d.at(pos/64) |= (static_cast<uint64_t>(1) << (pos%64)); }
	inline bool isSet(uint32_t pos) { return m_d.at(pos/64) & (static_cast<uint64_t>(1) << (pos%64)); }
	inline void reset() { m_d.assign(m_d.size(), 0); }
	template<typename TIT>
	void set(TIT begin, const TIT & end) {
		for(; begin != end; ++begin) {
			set(*begin);
		}
	}
};

}//end namespace sserialize


#endif