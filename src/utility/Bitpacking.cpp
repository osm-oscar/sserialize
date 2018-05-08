#include <sserialize/utility/Bitpacking.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
	
std::unique_ptr<BitunpackerInterface> BitunpackerInterface::unpacker(uint32_t bpn) {
#define C(__BPN) case __BPN: return std::unique_ptr<BitunpackerInterface>( new Bitunpacker<__BPN>() );
	switch (bpn) {
	C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31); C(32);
	default:
		throw sserialize::UnsupportedFeatureException("ItemIndexFoR: unsupported block bits");
	};
#undef C
}

}//end namespace sserialize
