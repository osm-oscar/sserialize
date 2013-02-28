#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_MMAPPED_FILE_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_MMAPPED_FILE_H
#include "UByteArrayAdapterPrivateArray.h"
#include <sserialize/utility/mmappedfile.h>

namespace sserialize {

class UByteArrayAdapterPrivateMmappedFile: public UByteArrayAdapterPrivateArray {
	MmappedFile m_file;
public:
	UByteArrayAdapterPrivateMmappedFile(MmappedFile & file);
	virtual ~UByteArrayAdapterPrivateMmappedFile();
	virtual void setDeleteOnClose(bool del);
	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size);
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType size);
};

}//end namespace

#endif