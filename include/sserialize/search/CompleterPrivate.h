#ifndef SSERIALIZE_COMPLETER_PRIVATE_H
#define SSERIALIZE_COMPLETER_PRIVATE_H
#include <sserialize/utility/refcounting.h>
#include <sserialize/search/ItemSet.h>
#include <sserialize/containers/ItemSetIterator.h>

namespace sserialize {

template<typename DataBaseItemType, typename DataBaseType>
class CompleterPrivate: public RefCountObject {
public:
	CompleterPrivate() : RefCountObject() {}
	virtual ~CompleterPrivate() {}
	virtual ItemSet<DataBaseItemType, DataBaseType> complete(const std::string& /*query*/) {
		return ItemSet<DataBaseItemType, DataBaseType>();
	}
	virtual ItemSetIterator<DataBaseItemType, DataBaseType> partialComplete(const std::string& /*query*/) {
		return ItemSetIterator<DataBaseItemType, DataBaseType>();
	}
};

}//end namespace


#endif