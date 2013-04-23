#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <iostream>


int main(int argc, char ** argv) {
	if (argc < 3) {
		std::cout << "Usage: prg <number of items> <strings per item>" << std::endl;
		return -1;
	}
	
	int itemCount = atoi(argv[1]);
	int strCount = atoi(argv[1]);
	
	std::vector<uint32_t> values;
	values.reserve(itemCount*strCount);
	
	std::cout << "uncompressed space usage:" << itemCount*strCount*4 << " Bytes = ";
	std::cout << static_cast<double>(itemCount*strCount*4)/0x100000 << std::cout;
	
	srand(0);
	uint32_t max = 0;
	for(uint32_t i = 0; i < strCount; i++) {
		for(uint32_t j = 0; j < itemCount; ++j) {
			uint32_t id = rand() % 0x7FFFFF;
			values.push_back();
			max = std::max(id, max);
		}
	}
	
	sserialize::DynamicBitSet bitSet;
	bitSet.set(max);
	
	std::vector<uint32_t>::const_iterator end(values.end());
	sserialize::TimeMeasurer tm;
	tm.begin();
	for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
		bitSet.set(*it);
	}
	tm.end();
	
	std::cout << "Conversion to DynamicBitSet took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	return 0;
}