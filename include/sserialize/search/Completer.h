#ifndef SSERIALIZE_COMPLETER_H
#define SSERIALIZE_COMPLETER_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/search/ItemSet.h>
#include "CompleterPrivate.h"

namespace sserialize {

template<typename DataBaseItemType, typename DataBaseType>
class Completer: public RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> > {
public:
    Completer() : RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> >(new CompleterPrivate<DataBaseItemType, DataBaseType>() ){}
    Completer(CompleterPrivate<DataBaseItemType, DataBaseType> * data) : RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> >(data) {}
    virtual ~Completer() {}
    Completer & operator=(const Completer & other) { RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> >::operator=(other); return *this;}
	ItemSet<DataBaseItemType, DataBaseType> complete(const std::string& query) {
		return RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> >::priv()->complete(query);
	}
	CompleterPrivate<DataBaseItemType, DataBaseType> * getPrivate() {
		return RCWrapper< CompleterPrivate<DataBaseItemType, DataBaseType> >::priv();
	}
};

}//end namespace


#endif