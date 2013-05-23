#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/utility/TimeMeasuerer.h>
#include <sserialize/utility/CompactUintArray.h>
#include <iostream>


int main(int argc, char ** argv) {
	if (argc < 3) {
		std::cout << "Usage: prg <number of items> <strings per item>" << std::endl;
		return -1;
	}
	
	int itemCount = atoi(argv[1]);
	int strCount = atoi(argv[2]);

	std::cout << "uncompressed space usage:" << itemCount*strCount*4 << " Bytes = ";
	std::cout << static_cast<double>(itemCount*strCount*4)/0x100000 << std::endl;
	

	std::vector<uint32_t> values;
	values.reserve(itemCount*strCount);
	

	
	srand(0);
	uint32_t max = 0;
	for(uint32_t i = 0; i < strCount; i++) {
		for(uint32_t j = 0; j < itemCount; ++j) {
			uint32_t id = rand() % 0x7FFFFF;
			values.push_back(id);
			max = std::max(id, max);
		}
	}
	std::vector<uint32_t>::const_iterator end(values.end());
	sserialize::TimeMeasurer tm;


	{
		sserialize::CompactUintArray carr(sserialize::UByteArrayAdapter(new std::vector<uint8_t>(), true), sserialize::CompactUintArray::minStorageBits(max) );
		carr.reserve(values.size());
		uint32_t pos = 0;
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			carr.set(pos, *it);
			++pos;
		}
		uint32_t mBitSetLen32 = max/32+1;
		uint32_t * mBitSet32 = new uint32_t[mBitSetLen32];
		tm.begin();
		uint32_t cs = values.size();
		for(uint32_t i = 0; i < cs; ++i) {
			uint32_t pos = carr.at(i);
			uint32_t offset = pos / 32;
			uint32_t inByteOffset = pos % 32;
			mBitSet32[offset] |= (static_cast<uint32_t>(1) << inByteOffset);
		}
		tm.end();
		std::cout << "Conversion from CompactUintArray to ManualBitSet32 took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	
	{
		sserialize::DynamicBitSet bitSet;
		bitSet.set(max);
		tm.begin();
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			bitSet.set(*it);
		}
		tm.end();
		std::cout << "Conversion to DynamicBitSet took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	{
		uint32_t mBitSetLen8 = max/8+1;
		uint8_t * mBitSet8 = new uint8_t[mBitSetLen8];
		tm.begin();
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			uint32_t pos = *it;
			uint32_t offset = pos / 8;
			uint32_t inByteOffset = pos % 8;
			mBitSet8[offset] |= (static_cast<uint8_t>(1) << inByteOffset);
		}
		tm.end();
		std::cout << "Conversion to ManualBitSet8 took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	{
		uint32_t mBitSetLen16 = max/16+1;
		uint16_t * mBitSet16 = new uint16_t[mBitSetLen16];
		tm.begin();
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			uint32_t pos = *it;
			uint32_t offset = pos / 16;
			uint32_t inByteOffset = pos % 16;
			mBitSet16[offset] |= (static_cast<uint16_t>(1) << inByteOffset);
		}
		tm.end();
		std::cout << "Conversion to ManualBitSet16 took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	{
		uint32_t mBitSetLen32 = max/32+1;
		uint32_t * mBitSet32 = new uint32_t[mBitSetLen32];
		tm.begin();
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			uint32_t pos = *it;
			uint32_t offset = pos / 32;
			uint32_t inByteOffset = pos % 32;
			mBitSet32[offset] |= (static_cast<uint32_t>(1) << inByteOffset);
		}
		tm.end();
		std::cout << "Conversion to ManualBitSet32 took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	{
		uint32_t mBitSetLen64 = max/64+1;
		uint64_t * mBitSet64 = new uint64_t[mBitSetLen64];
		tm.begin();
		for(std::vector<uint32_t>::const_iterator it(values.begin()); it != end; ++it) {
			uint32_t pos = *it;
			uint32_t offset = pos / 64;
			uint32_t inByteOffset = pos % 64;
			mBitSet64[offset] |= (static_cast<uint64_t>(1) << inByteOffset);
		}
		tm.end();
		std::cout << "Conversion to ManualBitSet64 took " << tm.elapsedMilliSeconds() << " msec" << std::endl;
	}
	
	return 0;
}