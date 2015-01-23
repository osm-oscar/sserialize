#ifndef SSERIALIZE_SPATIAL_GEO_CONSTRAINT_SET_OP_TREE_EF_H
#define SSERIALIZE_SPATIAL_GEO_CONSTRAINT_SET_OP_TREE_EF_H
#include <sserialize/containers/SetOpTree.h>
#include <sserialize/containers/ItemIndexIteratorGeoDB.h>
#include <sserialize/utility/stringfunctions.h>
#include <sserialize/utility/log.h>
#include "GeoRect.h"
#include <cstdlib>

namespace sserialize {
namespace spatial {


/** This filter has special support for AND. It just filters the given indices
  *
  * It parses the following String: lat_min;lat_max;lon_min;lon_max;boolean
  *
  */
template<typename TCompleter>
class GeoConstraintSetOpTreeSF: public sserialize::SetOpTree::SelectableOpFilter {
	TCompleter m_completer;
protected:
	bool parse(const std::string & str, GeoRect & rect, bool & approximate) const {
		std::vector<std::string> splitStr = split< std::vector<std::string> >(str, ';');
		if (splitStr.size() < 4)
			return false;
		rect.lat()[0] = std::atof(splitStr[0].c_str() );
		rect.lat()[1] = std::atof(splitStr[1].c_str() );
		rect.lon()[0] = std::atof(splitStr[2].c_str() );
		rect.lon()[1] = std::atof(splitStr[3].c_str() );
		approximate = false;
		if (splitStr.size() > 4)
			approximate = toBool(splitStr[4]);
		return true;
	}
public:
	GeoConstraintSetOpTreeSF(const TCompleter & completer) : m_completer(completer) {}
	virtual ~GeoConstraintSetOpTreeSF() {}
	TCompleter & completer() { return m_completer; }
	virtual SupportedOps supportedOps() const { return sserialize::SetOpTree::SelectableOpFilter::OP_INTERSECT;}
	/** This should invoke the faster routine */
	virtual ItemIndex operator()(SupportedOps /*op*/, const std::string & str, const ItemIndex & partner) {
		GeoRect rect;
		bool approximate;
		if (!parse(str, rect, approximate))
			return ItemIndex();
		return m_completer.filter(rect, approximate, partner);
	}
	virtual ItemIndex complete(const std::string & str) {
		GeoRect rect;
		bool approximate;
		if (!parse(str, rect, approximate))
			return ItemIndex();
		return m_completer.complete(rect, approximate);
	}
	virtual ItemIndexIterator operator()(SupportedOps /*op*/, const std::string & str, const ItemIndexIterator & partner) {
		GeoRect rect;
		bool approximate;
		if (!parse(str, rect, approximate))
			return ItemIndexIterator();
		return m_completer.filter(rect, approximate, partner);
	}
	virtual ItemIndexIterator partialComplete(const std::string & str) {
		GeoRect rect;
		bool approximate;
		if (!parse(str, rect, approximate))
			return ItemIndexIterator();
		return m_completer.partialComplete(rect, approximate);
	}
	virtual const std::string cmdString() const { return std::string("GEO"); }
	
	virtual std::string describe() const { return std::string("GeoConstraintSetOpTreeSF");}
};

}}//end namespace


#endif