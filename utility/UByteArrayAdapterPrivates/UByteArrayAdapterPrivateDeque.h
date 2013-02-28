#ifndef UBYTE_ARRAY_ADAPTER_PRIVATE_DEQUE_H
#define UBYTE_ARRAY_ADAPTER_PRIVATE_DEQUE_H
#include "UByteArrayAdapterPrivateContainer.h"
#include <deque>

namespace sserialize {
	typedef UByteArrayAdapterPrivateContainer< std::deque<uint8_t> > UByteArrayAdapterPrivateDeque;
}

#endif
