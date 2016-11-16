#include <vector>
#include <sserialize/containers/HuffmanTree.h>
#include <sserialize/Static/HuffmanDecoder.h>
#include <sserialize/iterator/MultiBitBackInserter.h>
#include <sserialize/iterator/MultiBitIterator.h>
#include <sserialize/iterator/UDWIterator.h>
#include <sserialize/iterator/UDWIteratorPrivateHD.h>
#include <sserialize/algorithm/utilfuncs.h>
#include "TestBase.h"

using namespace sserialize;

void print(std::stringstream & /*dest*/) {}

template<typename T, typename ... Args>
void print(std::stringstream & dest, T t, Args ... args) {
	dest << t;
	print(dest, args...);
}


template<typename ... Args>
std::string printToString(Args ... args) {
	std::stringstream  ss;
	print(ss, args...);
	return ss.str();
}

void putWrapper(UByteArrayAdapter & dest, const uint32_t & src) {
	dest.putUint32(src);
}

template<int NumberOfRuns, int TestDataLength>
class HuffmanCodeTest: public sserialize::tests::TestBase {
CPPUNIT_TEST_SUITE( HuffmanCodeTest );
CPPUNIT_TEST( testEquality );
CPPUNIT_TEST_SUITE_END();
private:
	std::vector<uint32_t> createTestData() {
		std::vector<uint32_t> ret;
		for(int i = 0; i < TestDataLength; ++i) {
			ret.push_back( rand() % 0xFFFF );
		}
		return ret;
	}
	
	template<typename TInputIterator>
	void createAlphabet(TInputIterator begin, const TInputIterator & end, std::unordered_map<uint32_t, uint32_t> & dest) {
		for(; begin != end; ++begin) {
			if (dest.count(*begin) == 0)
				dest[*begin] = 1;
			else
				dest[*begin] += 1;
		}
	}
	
	std::vector<uint8_t> createBitsPerLevel(uint8_t first, uint8_t minBits, uint8_t maxBits) {
		std::vector<uint8_t> res;
		res.push_back(first);
		uint32_t sum = first;
		while(sum < 64) {
			uint32_t b = minBits + (rand() % (maxBits-minBits));
			sum += b;
			res.push_back(b);
		}
		return res;
	}
private:
	
public:
	virtual void setUp() {}
	virtual void tearDown() {}
	
	void testEquality() {
		for(int i = 0; i < NumberOfRuns; ++i) {
			std::vector< uint32_t > testData = createTestData();
			std::unordered_map<uint32_t, uint32_t> alphabet;
			std::vector<uint8_t> bitsPerLevel = createBitsPerLevel(4, 2, 4);
			createAlphabet(testData.begin(), testData.end(), alphabet);
			HuffmanTree<uint32_t> ht;
			ht.create(alphabet.begin(), alphabet.end(), static_cast<uint32_t>(testData.size()));
		 	std::unordered_map<uint32_t, HuffmanCodePoint> htMap(ht.codePointMap());
			
			std::vector<uint8_t> dataStore;
			std::vector<uint8_t> decodeTable;
			UByteArrayAdapter dataAdap(&dataStore, false);
			UByteArrayAdapter decodeTableAdap(&decodeTable, false);
			MultiBitBackInserter backInserter(dataAdap);

			HuffmanTree<uint32_t>::ValueSerializer sfn = &putWrapper;
			
			for(std::vector< uint32_t >::const_iterator it(testData.begin()); it != testData.end(); ++it) {
				const HuffmanCodePoint hcp = htMap.at(*it);
				 backInserter.push_back(hcp.code(), hcp.codeLength());
			}
			backInserter.flush();
			dataAdap = backInserter.data();
			dataAdap.resetPtrs();
		
			CPPUNIT_ASSERT_MESSAGE("Serializing huffman tree", ht.serialize(decodeTableAdap, sfn, bitsPerLevel));
			decodeTableAdap.resetPtrs();
			
			RCPtrWrapper<Static::HuffmanDecoder> decoder(new Static::HuffmanDecoder(decodeTableAdap) );
			MultiBitIterator bitIt(dataAdap);
			UDWIterator udwIt(  new UDWIteratorPrivateHD(bitIt, decoder)  );
			
			for(uint32_t i = 0; i < testData.size(); ++i) {
				uint32_t real = testData[i];
				uint32_t decoded = udwIt.next();
				CPPUNIT_ASSERT_EQUAL_MESSAGE(printToString("at position ", i), real, decoded);
			}
		}
	}
};

int main(int argc, char ** argv) {
	sserialize::tests::TestBase::init(argc, argv);
	
	srand(0);
	srandom( 0 );
	CppUnit::TextUi::TestRunner runner;
	runner.eventManager().popProtector();
	runner.addTest( HuffmanCodeTest<10, 1011>::suite() );
	runner.addTest( HuffmanCodeTest<10, 10111>::suite() );
	runner.addTest( HuffmanCodeTest<10, 101111>::suite() );
	bool ok = runner.run();
	return ok ? 0 : 1;
}
