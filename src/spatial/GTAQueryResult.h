#ifndef SSERIALIZE_SPATIAL_GTA_QUERY_RESULT_H
#define SSERIALIZE_SPATIAL_GTA_QUERY_RESULT_H
#include <sserialize/Static/ItemIndexStore.h>


namespace sserialize {
	
namespace spatial {
namespace interface {
	
class GridTreeArrangement {
	
};
	
}//end namespace interface
	
namespace detail {
namespace GTAQueryResult {
	
	struct Node {
		typedef enum {FullMatch=0x1, Fetched=0x2} Type;
		uint8_t type;
		uint32_t nodeId;
		uint32_t idxId;
		sserialize::ItemIndex idx;
	};
	
	struct Filter {
		bool refine(const Node & node);
		bool filter(const Node & node);
	};
}}

///A GTAQueryResult holds all item and region candidates for a given query
class GTAQueryResult {
public:
	typedef enum {FF_NONE=0x0, FF_EXACT=0x1} FeatureFlags;
public:
	GTAQueryResult();
	~GTAQueryResult();
public:
	GTAQueryResult operator/(const GTAQueryResult & other) const;
	GTAQueryResult operator+(const GTAQueryResult & other) const;
	///This is correct if the other GTAQueryResult was filtered before
	GTAQueryResult operator-(const GTAQueryResult & other) const;
public:
	GTAQueryResult filter() const;
private:
	std::shared_ptr<sserialize::spatial::interface::GridTreeArrangement> m_gta;
	sserialize::Static::ItemIndexStore m_idxStore;
};
	
}}//end namespace 


#endif
