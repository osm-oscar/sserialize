#include <sserialize/utility/MmappedMemoy.h>
#include <iostream>

int main(int argc, char ** argv) {
	sserialize::UByteArrayAdapter::setTempFilePrefix(argv[0]);
	uint64_t size = atoll(argv[1]);
	sserialize::MmappedMemory<int> mem(size, false);
	std::cout << "Mapped " << size << " ints" << std::endl;
	exit(0);
	return 0;
}