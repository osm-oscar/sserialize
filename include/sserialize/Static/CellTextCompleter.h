#ifndef SSERIALIZE_STATIC_CELL_TEXT_COMPLETER_H
#define SSERIALIZE_STATIC_CELL_TEXT_COMPLETER_H
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/Static/UnicodeTrie/Trie.h>
#include <sserialize/containers/UnicodeStringMap.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/containers/RLEStream.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>

#ifndef SSERIALIZE_CELL_TEXT_COMPLETER_TAG_COMPLETION_PREFIX
	#define SSERIALIZE_CELL_TEXT_COMPLETER_TAG_COMPLETION_PREFIX '@'
#endif

#define SSERIALIZE_STATIC_CELL_TEXT_COMPLETER_VERSION 3

namespace sserialize {
namespace Static {

/** FileFormat v3
  *
  * struct CellTextCompleter {
  *   uint<8> version(3);
  *   uint<8> supportedQueries;
  *   uint<8> cqrFlags;
  *   uint<8> TrieTypeMarker;
  *   TrieType<CellTextCompleter::Payload> trie;
  * }
  * 
  * struct CellTextCompleter::Payload {
  *   uint<8> types;
  *   //The first offset is always 0 and will not be stored, offsets are stored in relative distance
  *   vuint<32> offsets;
  *   RLEStream ptrs; // in order fmPtr, pPtr, pItemsPtr
  * }
  * 
  */
namespace detail {

class CellTextCompleter: public sserialize::RefCountObject {
public:
	using CellInfo = sserialize::CellQueryResult::CellInfo;
	
	class Payload {
	public:
		using QuerryType = sserialize::StringCompleter::QuerryType;
		class Type {
		public:
			typedef sserialize::RLEStream const_iterator;
			typedef sserialize::RLEStream BaseContainer;
		private:
			uint32_t m_fmPtr{std::numeric_limits<uint32_t>::max()};
			uint32_t m_pPtr{std::numeric_limits<uint32_t>::max()};
			BaseContainer m_data;
		public:
			Type() {}
			Type(const BaseContainer & d) : m_data(d) {
				m_fmPtr = *m_data;
				++m_data;
				m_pPtr = *m_data;
				++m_data;
			}
			inline bool valid() const { return m_fmPtr != std::numeric_limits<uint32_t>::max() && m_pPtr != std::numeric_limits<uint32_t>::max(); }
			inline const BaseContainer & data() const { return m_data; }
			inline uint32_t fmPtr() const {
				return m_fmPtr;
			}
			inline uint32_t pPtr() const {
				return m_pPtr;
			}
			inline const_iterator pItemsPtrBegin() const {
				return m_data;
			}
		};
	private:
		uint32_t m_types;
		uint32_t m_offsets[3];
		sserialize::UByteArrayAdapter m_data;
	public:
		Payload() {}
		Payload(const sserialize::UByteArrayAdapter& d);
		~Payload() {}
		inline QuerryType types() const {
			return (QuerryType) (m_types & 0xF);
		}
		Type type(int qt) const;
		UByteArrayAdapter typeData(int qt) const;
	public:
		static uint32_t qt2Pos(int requested, int available);
	};
	typedef enum {TT_TRIE=0, TT_FLAT_TRIE=1} TrieTypeMarker;
	typedef sserialize::UnicodeStringMap<Payload> Trie;
	typedef sserialize::Static::UnicodeTrie::UnicodeStringMapFlatTrie<Payload> FlatTrieType;
	typedef sserialize::Static::UnicodeTrie::UnicodeStringMapTrie<Payload> TrieType;
// 	typedef sserialize::Static::UnicodeTrie::Trie<Payload> Trie;
// 	typedef Trie::Node Node;
private:
	sserialize::StringCompleter::SupportedQuerries m_sq;
	int m_flags;
	TrieTypeMarker m_tt;
	Trie m_trie;
	sserialize::Static::ItemIndexStore m_idxStore;
	sserialize::Static::spatial::GeoHierarchy m_gh;
	CellInfo m_ci;
	sserialize::Static::spatial::TriangulationGeoHierarchyArrangement m_ra;
public:
	CellTextCompleter();
	CellTextCompleter(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore, const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra);
	virtual ~CellTextCompleter();
	inline const Trie & trie() const { return m_trie;}
	inline Trie & trie() { return m_trie;}
	
	inline const CellInfo & cellInfo() const { return m_ci;}
	inline const Static::spatial::GeoHierarchy & geoHierarchy() const { return m_gh;}
	inline const sserialize::Static::ItemIndexStore & idxStore() const { return m_idxStore; }
	inline const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & regionArrangement() const { return m_ra; }
	
	///defined by sserialize::CellQueryResult::FeatureFlags
	inline int flags() const { return m_flags; }
	
	bool count(const std::string::const_iterator& begin, const std::string::const_iterator& end) const;
	Payload::Type typeFromCompletion(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;
	std::ostream & printStats(std::ostream & out) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE fromRegionStoreId(uint32_t id) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE regionExclusiveCells(uint32_t regionId) const;
	
	template<typename T_CQR_TYPE, typename T_ITERATOR>
	T_CQR_TYPE fromCellIds(const T_ITERATOR & begin, const T_ITERATOR & end) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE fromCellId(uint32_t id) const;
	
	template<typename T_CQR_TYPE, typename T_ITERATOR>
	T_CQR_TYPE fromTriangleIds(const T_ITERATOR & begin, const T_ITERATOR & end) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE fromTriangleId(uint32_t id) const;
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	T_CQR_TYPE cqrFromRect(const sserialize::spatial::GeoRect & rect) const;
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	T_CQR_TYPE cqrFromPoint(const sserialize::spatial::GeoPoint & point, double radius) const;

	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	T_CQR_TYPE cqrBetween(const sserialize::spatial::GeoPoint & start, const sserialize::spatial::GeoPoint & end, double radius) const;
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult, typename T_GEOPOINT_ITERATOR>
	T_CQR_TYPE cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const;
};

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::complete(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(m_idxStore.at( t.fmPtr() ), m_idxStore.at( t.pPtr() ), t.pItemsPtrBegin(), m_ci, m_idxStore, flags());
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE(m_ci, m_idxStore, flags());
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::regions(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(m_idxStore.at( t.fmPtr() ), m_ci, m_idxStore, flags());
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE(m_ci, m_idxStore, flags());
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::items(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(sserialize::ItemIndex(), m_idxStore.at( t.pPtr() ), t.pItemsPtrBegin(), m_ci, m_idxStore, flags());
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE(m_ci, m_idxStore, flags());
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::fromRegionStoreId(uint32_t storeId) const {
	if (storeId < m_gh.regionSize()) {
		uint32_t regionCellPtr = m_gh.regionCellIdxPtr(m_gh.storeIdToGhId(storeId));
		return T_CQR_TYPE(m_idxStore.at(regionCellPtr), m_ci, m_idxStore, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::regionExclusiveCells(uint32_t storeId) const {
	if (storeId < m_gh.regionSize()) {
		uint32_t cellPtr = m_gh.regionExclusiveCellIdxPtr(m_gh.storeIdToGhId(storeId));
		return T_CQR_TYPE(m_idxStore.at(cellPtr), m_ci, m_idxStore, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE, typename T_ITERATOR>
T_CQR_TYPE CellTextCompleter::fromCellIds(const T_ITERATOR & begin, const T_ITERATOR & end) const {
	SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(begin, end));
	if (begin != end) {
		return T_CQR_TYPE(sserialize::ItemIndex(std::vector<uint32_t>(begin, end)), m_ci, m_idxStore, flags());
	}
	else {
		return T_CQR_TYPE(m_ci, m_idxStore, flags());
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::fromCellId(uint32_t cellId) const {
	if (cellId < m_gh.cellSize()) {
		return T_CQR_TYPE(true, cellId, m_ci, m_idxStore, 0, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE, typename T_ITERATOR>
T_CQR_TYPE CellTextCompleter::fromTriangleIds(const T_ITERATOR & begin, const T_ITERATOR & end) const {
	SSERIALIZE_NORMAL_ASSERT(sserialize::is_strong_monotone_ascending(begin, end));
	if (begin != end) {
		std::set<uint32_t> tmp;
		for(; begin != end; ++begin) {
			tmp.insert(m_ra.cellIdFromFaceId(*begin));
		}
		return T_CQR_TYPE(sserialize::ItemIndex(std::vector<uint32_t>(tmp.begin(), tmp.end())), m_ci, m_idxStore, flags());
		
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::fromTriangleId(uint32_t triangleId) const {
	if (triangleId < 1) {
		uint32_t cellId = m_ra.cellIdFromFaceId(triangleId);
		return T_CQR_TYPE(true, cellId, m_ci, m_idxStore, 0, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::cqrFromRect(const sserialize::spatial::GeoRect & rect) const {
	T_CQR_TYPE retCQR;
	sserialize::ItemIndex tmp = m_gh.intersectingCells(idxStore(), rect);
	return T_CQR_TYPE(tmp, m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::cqrFromPoint(const sserialize::spatial::GeoPoint & point, double radius) const {
	if (radius <= 0) {
		return fromCellId<T_CQR_TYPE>( m_ra.cellId(point) );
	}
	else {
		return cqrFromRect<T_CQR_TYPE>( sserialize::spatial::GeoRect(point.lat(), point.lon(), radius) );
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::cqrBetween(const sserialize::spatial::GeoPoint& start, const sserialize::spatial::GeoPoint& end, double radius) const {
	sserialize::ItemIndex idx( m_ra.cellsBetween(start, end, radius) );
	if (idx.size()) {
		return T_CQR_TYPE(idx, m_ci, m_idxStore, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

template<typename T_CQR_TYPE, typename T_GEOPOINT_ITERATOR>
T_CQR_TYPE CellTextCompleter::cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
	sserialize::ItemIndex idx( m_ra.cellsAlongPath(radius, begin, end) );
	if (idx.size()) {
		return T_CQR_TYPE(idx, m_ci, m_idxStore, flags());
	}
	return T_CQR_TYPE(m_ci, m_idxStore, flags());
}

}//end namespace detail

class CellTextCompleter {
public:
// 	typedef detail::CellTextCompleter Node;
	typedef detail::CellTextCompleter::CellInfo CellInfo;
	typedef detail::CellTextCompleter::Payload Payload;
	typedef detail::CellTextCompleter::Trie Trie;
	typedef detail::CellTextCompleter::FlatTrieType FlatTrieType;
	typedef detail::CellTextCompleter::TrieType TrieType;
	typedef detail::CellTextCompleter::TrieTypeMarker TrieTypeMarker;
private:
	sserialize::RCPtrWrapper<detail::CellTextCompleter> m_priv;
private:
	inline detail::CellTextCompleter* priv() { return m_priv.priv(); }
	inline const detail::CellTextCompleter* priv() const { return m_priv.priv(); }
public:
	CellTextCompleter() : m_priv(new detail::CellTextCompleter()) {}
	CellTextCompleter(detail::CellTextCompleter * priv) : m_priv(priv) {}
	CellTextCompleter(const sserialize::RCPtrWrapper<detail::CellTextCompleter> & priv) : m_priv(priv) {}
	///@param ra this is need to provide the cellsAlongPath and cqrBetween completions (optional)
	CellTextCompleter(
		const UByteArrayAdapter & d,
		const Static::ItemIndexStore & idxStore,
		const Static::spatial::GeoHierarchy & gh,
		const Static::spatial::TriangulationGeoHierarchyArrangement & ra = Static::spatial::TriangulationGeoHierarchyArrangement()
	) :
	m_priv(new detail::CellTextCompleter(d, idxStore, gh, ra))
	{}
	virtual ~CellTextCompleter() {}
	inline const Trie & trie() const {
		return priv()->trie();
	}
	inline Trie & trie() {
		return priv()->trie();
	}
	
	inline const CellInfo & cellInfo() const {
		return priv()->cellInfo();
	}
	
	inline const Static::spatial::GeoHierarchy & geoHierarchy() const {
		return priv()->geoHierarchy();
	}
	
	inline const sserialize::Static::ItemIndexStore & idxStore() const {
		return priv()->idxStore();
	}
	
	inline const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & regionArrangement() const {
		return priv()->regionArrangement();
	}
	
	inline Payload::Type typeFromCompletion(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->typeFromCompletion(qstr, qt);
	}
	
	inline int flags() const {
		return priv()->flags();
	}
	
	inline bool count(const std::string::const_iterator & begin, const std::string::const_iterator & end) const {
		return priv()->count(begin, end);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->complete<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->regions<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->items<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE regionExclusiveCells(uint32_t regionId) const {
		return priv()->regionExclusiveCells<T_CQR_TYPE>(regionId);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromRegionStoreId(uint32_t id) const {
		return priv()->fromRegionStoreId<T_CQR_TYPE>(id);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult, typename T_ITERATOR>
	inline T_CQR_TYPE cqrFromCellIds(const T_ITERATOR & begin, const T_ITERATOR & end) const {
		return priv()->fromCellIds<T_CQR_TYPE>(begin, end);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromCellId(uint32_t id) const {
		return priv()->fromCellId<T_CQR_TYPE>(id);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromTriangleId(uint32_t id) const {
		return priv()->fromTriangleId<T_CQR_TYPE>(id);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromRect(const sserialize::spatial::GeoRect & rect) const {
		return priv()->cqrFromRect<T_CQR_TYPE>(rect);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromPoint(const sserialize::spatial::GeoPoint & point, double radius) const {
		return priv()->cqrFromPoint<T_CQR_TYPE>(point, radius);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrBetween(const sserialize::spatial::GeoPoint & start, const sserialize::spatial::GeoPoint & end, double radius) const {
		return priv()->cqrBetween<T_CQR_TYPE>(start, end, radius);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult, typename T_GEOPOINT_ITERATOR>
	inline T_CQR_TYPE cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
		return priv()->cqrAlongPath<T_CQR_TYPE>(radius, begin, end);
	}
	
	inline sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return priv()->getSupportedQuerries();
	}
	
	inline std::ostream & printStats(std::ostream & out) const {
		return priv()->printStats(out);
	}
	
	inline std::string getName() const { return "sserialize::Static::CellTextCompleter"; }
};


class CellTextCompleterUnclustered: public sserialize::StringCompleterPrivate {
private:
	CellTextCompleter m_cellTextCompleter;
	sserialize::Static::spatial::GeoHierarchy m_gh;
public:
	CellTextCompleterUnclustered() {}
	CellTextCompleterUnclustered(const CellTextCompleter & c, const sserialize::Static::spatial::GeoHierarchy & gh) : m_cellTextCompleter(c), m_gh(gh) {}
	virtual ~CellTextCompleterUnclustered() {}
	virtual sserialize::ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual sserialize::ItemIndex indexFromId(uint32_t idxId) const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
	const CellTextCompleter & cellTextCompleter() const { return m_cellTextCompleter; }
};

}}

#endif
