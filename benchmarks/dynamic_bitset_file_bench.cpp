#include <sserialize/containers/DynamicBitSet.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/storage/MmappedFile.h>
#include <iostream>



int main(int argc, char ** argv) {
	if (argc < 3) {
		std::cout << "Usage: prg (c|t) <file> if-c[<number of items> <strings per item>]" << std::endl;
		return -1;
	}
	
	if (argv[1][0] == 'c') {
		if (argc < 5) {
			return -1;
		}
		std::string outFileName(argv[2]);
		int itemCount = atoi(argv[3]);
		int strCount = atoi(argv[4]);

		std::cout << "uncompressed space usage:" << itemCount*strCount*4 << " Bytes = ";
		std::cout << static_cast<double>(itemCount*strCount*4)/0x100000 << std::endl;
		sserialize::UByteArrayAdapter outAdap = sserialize::UByteArrayAdapter::createFile(itemCount*strCount*4+4, outFileName);
		outAdap << static_cast<uint32_t>(0); //dummy max
		srand(0);
		uint32_t max = 0;
		for(int i = 0; i < strCount; i++) {
			for(int j = 0; j < itemCount; ++j) {
				uint32_t id = rand() % 0x7FFFFF;
				outAdap << id;
				max = std::max(id, max);
			}
		}
		return 0;
	}
	else if (argv[1][0] == 't') {
		sserialize::UByteArrayAdapter inAdap = sserialize::UByteArrayAdapter::open(std::string(argv[2]));
		uint32_t max = inAdap.getUint32();
		sserialize::DynamicBitSet bitSet;
		bitSet.set(max);
		
		sserialize::TimeMeasurer tm;
		tm.begin();
		sserialize::UByteArrayAdapter::OffsetType size = inAdap.size()/4 - 1;
		for(sserialize::UByteArrayAdapter::OffsetType i = 0; i < size; ++i) {
			uint32_t id = inAdap.getUint32();
			bitSet.set(id);
		}
		tm.end();
		std::cout << "Conversion to DynamicBitSet took " << tm.elapsedMilliSeconds() << " msec" << std::endl;

		/*
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
		*/
	}
	return -1;
}