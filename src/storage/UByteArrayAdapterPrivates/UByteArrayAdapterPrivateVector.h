#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_VECTOR_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_VECTOR_H
#include "UByteArrayAdapterPrivateContainer.h"
#include "UByteArrayAdapterPrivateArray.h"
#include <vector>

namespace sserialize {

class UByteArrayAdapterPrivateVector: public UByteArrayAdapterPrivateArray {
	std::vector<uint8_t> * m_data;
public:
	UByteArrayAdapterPrivateVector(std::vector<uint8_t> * data);
	virtual ~UByteArrayAdapterPrivateVector();
	virtual OffsetType size() const override;

	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) override;
	virtual bool growStorage(UByteArrayAdapter::OffsetType size) override;
};

}

#endif
