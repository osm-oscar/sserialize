#ifndef SSERIALIZE_GEO_COMPLETER_PRIVATE_PROXY_H
#define SSERIALIZE_GEO_COMPLETER_PRIVATE_PROXY_H
#include "GeoCompleterPrivate.h"

namespace sserialize {

template<typename RealCompleter>
class GeoCompleterPrivateProxy: public GeoCompleterPrivate {
private:
	RealCompleter m_realCmp;
public:
	GeoCompleterPrivateProxy(const RealCompleter & realCompleter) : GeoCompleterPrivate(), m_realCmp(realCompleter) {}
	virtual ~GeoCompleterPrivateProxy() {}
	virtual ItemIndex complete(const spatial::GeoRect & rect, bool approximate) { return m_realCmp.complete(rect, approximate);}
	virtual ItemIndexIterator partialComplete(const spatial::GeoRect & rect, bool approximate) { return m_realCmp.partialComplete(rect, approximate);}
	virtual ItemIndex filter(const spatial::GeoRect& rect, bool approximate, const ItemIndex& partner) { return m_realCmp.filter(rect, approximate, partner);}
	virtual ItemIndexIterator filter(const spatial::GeoRect& rect, bool approximate, const ItemIndexIterator& partner) { return m_realCmp.filter(rect, approximate, partner);}
	virtual std::ostream & printStats(std::ostream & out) const {
		m_realCmp.printStats(out);
		return out;
	}
	virtual std::string getName() const { return m_realCmp.getName(); }
};


}//end namespace

#endif