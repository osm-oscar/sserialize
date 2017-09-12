#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_DEQUE_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_DEQUE_H
#include "UByteArrayAdapterPrivateContainer.h"
#include <deque>

namespace sserialize {

#ifndef SSERIALIZE_UBA_ONLY_CONTIGUOUS
inline
#endif
namespace UByteArrayAdapterNonContiguous {
	typedef UByteArrayAdapterPrivateContainer< std::deque<uint8_t> > UByteArrayAdapterPrivateDeque;
} //end namespace UByteArrayAdapterNonContiguous

} //end sserialize

#endif
