#ifndef SSERIALIZE_HASH_SPECIALIZATIONS_H
#define SSERIALIZE_HASH_SPECIALIZATIONS_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/spatial/GeoPoint.h>
#include <string>
#include <vector>
#include <functional>
#include <cryptopp/sha3.h>

namespace CryptoPP {


#if CRYPTOPP_VERSION >= 820
class SHA3_128: public SHA3_Final<16> {};
#else
class SHA3_128 : public SHA3
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = 16)

	//! \brief Construct a SHA3-128 message digest
	SHA3_128() : SHA3(DIGESTSIZE) {}
	#if CRYPTOPP_VERSION >= 564
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "SHA3-128";}
	#else
	static const char *StaticAlgorithmName() {return "SHA3-128";}
	#endif
};
#endif

} // end namespace CryptoPP

namespace sserialize {

struct ShaHasherDigestData {
	static constexpr std::size_t DigestSize = 16;
	std::array<uint8_t, DigestSize> data;
	auto begin() { return data.begin(); }
	auto begin() const { return data.begin(); }
	auto end() { return data.end(); }
	auto end() const { return data.end(); }
	bool operator==(const ShaHasherDigestData & other) const { return data == other.data; }
	bool operator!=(const ShaHasherDigestData & other) const { return data != other.data; }
};

template<typename T>
class ShaHasher;

template<>
class ShaHasher<sserialize::UByteArrayAdapter::MemoryView> {
public:
	static constexpr uint32_t DigestSize = 16;
	using Hasher = CryptoPP::SHA3_128;
	using DigestData = ShaHasherDigestData;
	using value_type = sserialize::UByteArrayAdapter::MemoryView;
public:
	inline DigestData operator()(const value_type & value) const {
		Hasher hasher;
		DigestData data;
		hasher.CalculateDigest(data.begin(), value.begin(), value.size());
		return data;
	}
};

template<>
class ShaHasher<sserialize::UByteArrayAdapter> {
public:
	static constexpr uint32_t DigestSize = 16;
	static constexpr sserialize::UByteArrayAdapter::SizeType UpdateBlockSize = 1024*1024*1024;
	using Hasher = CryptoPP::SHA3_128;
	using DigestData = ShaHasherDigestData;
	using value_type = sserialize::UByteArrayAdapter;
public:
	ShaHasher() : m_updateBlockSize(UpdateBlockSize) {}
	inline DigestData operator()(const value_type & value) const {
		if (value.isContiguous() || value.size() < m_updateBlockSize) {
			return m_h(value.asMemView());
		}
		else {
			Hasher hasher;
			DigestData data;
			sserialize::UByteArrayAdapter::SizeType pos = 0;
			sserialize::UByteArrayAdapter::SizeType size = value.size();
			for(;pos < size; pos += m_updateBlockSize, size -= m_updateBlockSize) {
				auto blockSize = std::min(m_updateBlockSize, size);
				auto memv = value.getMemView(pos, blockSize);
				hasher.Update(memv.begin(), blockSize);
			}
			hasher.Final(data.begin());
			return data;
		}
	}
private:
	ShaHasher<sserialize::UByteArrayAdapter::MemoryView> m_h;
	sserialize::UByteArrayAdapter::SizeType m_updateBlockSize;
};

template<>
class ShaHasher<std::vector<uint8_t>> {
public:
	static constexpr uint32_t DigestSize = 16;
	static constexpr sserialize::UByteArrayAdapter::SizeType UpdateBlockSize = 1024*1024*1024;
	using Hasher = CryptoPP::SHA3_128;
	using DigestData = ShaHasherDigestData;
	using value_type = std::vector<uint8_t>;
public:
	inline DigestData operator()(const value_type & value) const {
		Hasher hasher;
		DigestData data;
		hasher.CalculateDigest(data.begin(), &(value[0]), value.size());
		return data;
	}
};
	
} //end namespace sserialize


//from http://stackoverflow.com/questions/7222143/unordered-map-hash-function-c, which is from boost

inline void hash_combine(uint64_t & seed, const char v) {
	seed ^= static_cast<uint64_t>(v) + static_cast<uint64_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

inline void hash_combine(uint64_t & seed, const unsigned char v) {
	seed ^= static_cast<uint64_t>(v) + static_cast<uint64_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_combine(uint64_t & seed, const T & v, const std::hash<T> & hasher) {
	seed ^= static_cast<uint64_t>(hasher(v)) + static_cast<uint64_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_combine(uint64_t & seed, const T & v) {
	std::hash<T> hasher;
	hash_combine(seed, v, hasher);
}

//the same for uint32_t

inline void hash_combine(uint32_t & seed, const char v) {
	seed ^= static_cast<uint32_t>(v) + static_cast<uint32_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

inline void hash_combine(uint32_t & seed, const unsigned char v) {
	seed ^= static_cast<uint32_t>(v) + static_cast<uint32_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_combine(uint32_t & seed, const T & v, const std::hash<T> & hasher) {
	seed ^= static_cast<uint32_t>(hasher(v)) + static_cast<uint32_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_combine(uint32_t & seed, const T & v) {
	std::hash<T> hasher;
	hash_combine(seed, v, hasher);
}

namespace std {

template<typename S, typename V> 
struct hash< std::pair<S, V> > {
	std::hash<S> hS;
	std::hash<V> hV;
	inline std::size_t operator()(const pair<S, V> & v) const {
		std::size_t seed = 0;
		::hash_combine(seed, v.first, hS);
		::hash_combine(seed, v.second, hV);
		return seed;
	}
};

template<>
struct hash< pair<uint32_t, uint32_t> > {
	std::hash<uint64_t> hS;
	inline std::size_t operator()(const pair<uint32_t, uint32_t> & v) const {
		return hS( (static_cast<uint64_t>(v.first) << 32) | static_cast<uint64_t>(v.second) );
	}
};

template<typename T>
struct hash< std::vector<T> > {
	std::hash<T> hasher;
	inline std::size_t operator()(const std::vector<T> & v) const {
		std::size_t seed = 0;
		for(typename std::vector<T>::const_iterator it(v.begin()), end(v.end()); it != end; ++it) {
			::hash_combine(seed, *it, hasher);
		}
		return seed;
	}
};

template<>
struct hash< sserialize::ShaHasherDigestData > {
	inline std::size_t operator()(const sserialize::ShaHasherDigestData & v) const {
		std::size_t h;
		constexpr std::size_t s = (sizeof(h) < sserialize::ShaHasherDigestData::DigestSize ? sizeof(h) : sserialize::ShaHasherDigestData::DigestSize);
		::memmove(&h, v.begin(), s);
		return h;
	}
};

template<>
struct hash<sserialize::spatial::GeoPoint> {
	std::hash<double> hash;
	inline std::size_t operator()(const sserialize::spatial::GeoPoint & v) const {
		std::size_t seed = 0;
		::hash_combine(seed, v.lat(), hash);
		::hash_combine(seed, v.lon(), hash);
		return seed;
	}
};

}//end namespace std

namespace sserialize {

template<typename TPtr, typename T = typename std::remove_cv<typename std::remove_pointer<TPtr>::type>::type >
struct DerefHasher {
	std::hash<T> hasher;
	inline size_t operator()(const TPtr & t) const {
		return hasher(*t);
	}
};

template<typename TPtr>
struct DerefEq {
	inline bool operator()(const TPtr & t1, const TPtr & t2) const {
		return *t1 == *t2;
	}
};

template<typename TPtr>
struct DerefSmaler {
	inline bool operator()(const TPtr & t1, const TPtr & t2) const {
		return *t1 < *t2;
	}
};

}//end namespace sserialize

#endif
