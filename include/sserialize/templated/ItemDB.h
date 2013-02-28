#ifndef SSERIALIZE_ITEM_DB_H
#define SSERIALIZE_ITEM_DB_H
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/refcounting.h>

namespace sserialize {

class ItemReAssignComparator {
public:
	ItemReAssignComparator() {}
	virtual ~ItemReAssignComparator() {}
	virtual bool operator()(uint32_t itemIdA, uint32_t itemIdB) {
		return itemIdA < itemIdB;
	}
};


}//end namespace



#endif