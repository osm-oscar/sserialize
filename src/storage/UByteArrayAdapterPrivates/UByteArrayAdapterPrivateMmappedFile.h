#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MMAPPED_FILE_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MMAPPED_FILE_H
#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/storage/MmappedFile.h>

namespace sserialize {

class UByteArrayAdapterPrivateMmappedFile: public UByteArrayAdapterPrivateArray {
	MmappedFile m_file;
public:
	UByteArrayAdapterPrivateMmappedFile(MmappedFile file);
	virtual ~UByteArrayAdapterPrivateMmappedFile();
	virtual void advice(UByteArrayAdapter::AdviseType /*at*/, UByteArrayAdapter::SizeType /*begin*/, UByteArrayAdapter::SizeType /*end*/) override;
	virtual void sync() override;
	virtual UByteArrayAdapter::OffsetType size() const override;
	virtual void setDeleteOnClose(bool del) override;
	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) override;
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType size) override;
};

}//end namespace

#endif
