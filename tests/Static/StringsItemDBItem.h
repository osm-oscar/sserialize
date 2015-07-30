#ifndef SSERIALIZE_STATIC_STRINGS_ITEM_DB_ITEM_H
#define SSERIALIZE_STATIC_STRINGS_ITEM_DB_ITEM_H
#include "../containers/StringsItemDBItem.h"
#include <sserialize/storage/UByteArrayAdapter.h>

namespace sserialize {
namespace Static {

template<typename DataBaseType, typename MetaDataDeSerializable>
class StringsItemDBItem: public sserialize::StringsItem<DataBaseType> {
	UByteArrayAdapter m_payload;
protected:
	UByteArrayAdapter payloadData() const {
		return m_payload;
	}
public:
	StringsItemDBItem() : sserialize::StringsItem<DataBaseType>() {}
	StringsItemDBItem(uint32_t id, const DataBaseType & db)  : sserialize::StringsItem<DataBaseType>(id, db), m_payload(db.itemDataAt(id)) {}

	MetaDataDeSerializable data() const {
		if (m_payload.size() > 0) {
			return MetaDataDeSerializable(payloadData());
		}
		else {
			return MetaDataDeSerializable();
		}
	}
};

}}//end namespace

#endif