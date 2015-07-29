#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sserialize/utility/UByteArrayAdapter.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <sserialize/utility/mmappedfile.h>
#include <sserialize/utility/CompactUintArray.h>
#include "datacreationfuncs.h"

using namespace sserialize;


enum CODEDNUMTYPES { CT_UINT8, CT_UINT16, CT_UINT24, CT_UINT32, CT_VLUINT32, CT_VLINT32 };

bool fillStrings(std::deque<std::string> & strings, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	for(size_t i = 0; i < strings.size(); i++) {
		int len = adapter.putString(curPos, strings[i]);
		if (len < 0)
			return false;
		curPos += len;
	}
	return true;
}

bool fillStringsStreaming(std::deque<std::string> & strings, UByteArrayAdapter & adapter) {
	for(size_t i = 0; i < strings.size(); i++) {
		adapter << strings[i]; 
	}
	return true;
}

bool testStrings(std::deque<std::string> & strings, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	for(size_t i = 0; i < strings.size(); i++) {
		int len;
		std::string str(adapter.getString(curPos, &len));
		if (len < 0 || strings[i] != str)
			return false;
		curPos += len;
	}
	return true;
}

bool testStringsStreaming(std::deque<std::string> & strings, UByteArrayAdapter adapter) {
	for(size_t i = 0; i < strings.size(); i++) {
		std::string str;
		adapter >> str;
		if (strings[i] != str)
			return false;
	}
	return true;
}

bool testStringsData(std::deque<std::string> & strings, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	for(size_t i = 0; i < strings.size(); i++) {
		int len;
		UByteArrayAdapter strData = adapter.getStringData(curPos, &len);
		std::string str;
		str.resize(strData.size());
		for(size_t j = 0; j < strData.size(); j++) {
			str[j] = strData.at(j);
		}
		if (len < 0 || strings[i] != str)
			return false;
		curPos += len;
	}
	return true;
}

bool testStringsDataStreaming(std::deque<std::string> & strings, UByteArrayAdapter adapter) {
	for(size_t i = 0; i < strings.size(); i++) {
		UByteArrayAdapter strData = adapter.getStringData();
		std::string str;
		str.resize(strData.size());
		for(size_t j = 0; j < strData.size(); j++) {
			str[j] = strData.at(j);
		}
		if (strings[i] != str)
			return false;
	}
	return true;
}

bool fillAndTestStrings(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data(maxStrLen*strCount, 0);
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStrings(strs, adap))
		return false;
	return testStrings(strs, adap);
}

bool fillAndTestStringsStreaming(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data(maxStrLen*strCount, 0);
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStrings(strs, adap))
		return false;
	adap.resetPtrs();
	return testStringsStreaming(strs, adap);
}

bool fillAndTestStringsData(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data(maxStrLen*strCount, 0);
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStrings(strs, adap))
		return false;
	return testStringsData(strs, adap);
}

bool fillAndTestStringsDataStreaming(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data(maxStrLen*strCount, 0);
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStrings(strs, adap))
		return false;
	adap.resetPtrs();
	return testStringsDataStreaming(strs, adap);
}

bool fillStreamingAndTestStrings(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStringsStreaming(strs, adap))
		return false;
	adap.resetPtrs();
	return testStrings(strs, adap);
}

bool fillStreamAndTestStringsStreaming(uint32_t maxStrLen, uint32_t strCount) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adap(&data);

	std::deque<std::string> strs(createStrings(maxStrLen, strCount));
	if (!fillStringsStreaming(strs, adap))
		return false;
	adap.resetPtrs();
	return testStringsStreaming(strs, adap);
}

bool fillFixedSize(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			adapter.putUint8(curPos, num);
			curPos += 1;
			break;
		case(2):
			adapter.putUint16(curPos, num);
			curPos += 2;
			break;
		case(3):
			adapter.putUint24(curPos, num);
			curPos += 3;
			break;
		case(4):
			adapter.putUint32(curPos, num);
			curPos += 4;
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
	}
	return true;
}

bool fillFixedSizeStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter & adapter) {
	adapter.resetPtrs();
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			adapter.putUint8(num);
			break;
		case(2):
			adapter.putUint16(num);
			break;
		case(3):
			adapter.putUint24(num);
			break;
		case(4):
			adapter.putUint32(num);
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
	}
	return true;
}

bool testFixedSize(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	//Now test them
	uint32_t curPos = 0;
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		uint32_t codedNum = 0;
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			codedNum = adapter.getUint8(curPos);
			curPos += 1;
			break;
		case(2):
			codedNum = adapter.getUint16(curPos);
			curPos += 2;
			break;
		case(3):
			codedNum = adapter.getUint24(curPos);
			curPos += 3;
			break;
		case(4):
			codedNum = adapter.getUint32(curPos);
			curPos += 4;
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
		if (codedNum != num) {
// 			std::cout << "Failed with codedNum=" << codedNum << " and realNum=" << num << std::endl;
			return false;
		}
	}
	return true;
}


bool testFixedSizeStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	adapter.resetPtrs();
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		uint32_t codedNum = 0xFEFE;
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			codedNum = adapter.getUint8();
			break;
		case(2):
			codedNum = adapter.getUint16();
			break;
		case(3):
			codedNum = adapter.getUint24();
			break;
		case(4):
			codedNum = adapter.getUint32();
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
		if (codedNum != num) {
// 			std::cout << "Failed with codedNum=" << codedNum << " and realNum=" << num << std::endl;
			return false;
		}
	}
	return true;
}


bool fillAndTestFixedSize(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	fillFixedSize(realNumbers, adapter);
	return testFixedSize(realNumbers, adapter);
}

bool fillAndTestStreamingApi(std::deque<uint32_t> & realNumbers) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adapter(&data);
	fillFixedSizeStreaming(realNumbers, adapter);
	return testFixedSizeStreaming(realNumbers, adapter);
}

bool fillFixedSizeStreamingAndTestFixedSize(std::deque<uint32_t> & realNumbers) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adapter(&data);
	fillFixedSizeStreaming(realNumbers, adapter);
	return testFixedSize(realNumbers, adapter);
}

bool fillFixedSizeAndTestStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	fillFixedSize(realNumbers, adapter);
	return testFixedSizeStreaming(realNumbers, adapter);
}

bool fillVariableSize(const std::deque<uint32_t> & realNumbers, std::deque<uint32_t> & codedPositions, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	codedPositions.resize(realNumbers.size(), 0);
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		int len = adapter.putVlPackedUint32(curPos, num);
		if (len > 0) {
			codedPositions[i] = curPos;
			curPos += len;
		}
		else {
			std::cout << "Failed to encode number " << num << std::endl;
			return false;
		}
	}
	return true;
}

bool fillVariableSizeStreaming(const std::deque<uint32_t> & realNumbers, UByteArrayAdapter & adapter) {
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		adapter.putVlPackedUint32(num);
	}
	return true;
}

bool testVariableSize(std::deque<uint32_t> & realNumbers, const std::deque<uint32_t> & codedPositions, UByteArrayAdapter adapter) {
	//Now test them
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		uint32_t codedNum;
		int len;
		codedNum = adapter.getVlPackedUint32(codedPositions[i], &len);
		if (len <= 0 || codedNum != num) {
			return false;
		}
	}
	return true;
}

bool testVariableSizeStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	//Now test them
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		uint32_t codedNum;
		codedNum = adapter.getVlPackedUint32();
		if (codedNum != num) {
			return false;
		}
	}
	return true;
}

bool fillAndTestVariableSize(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	std::deque<uint32_t> codedPositions;
	if (!fillVariableSize(realNumbers, codedPositions, adapter))
		return false;
	return testVariableSize(realNumbers,codedPositions, adapter);
}

bool fillStreamingAndTestVariableSizeStreaming(std::deque<uint32_t> & realNumbers) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adap(&data);
	if (!fillVariableSizeStreaming(realNumbers, adap))
		return false;
	adap.resetPtrs();
	return testVariableSizeStreaming(realNumbers,adap);
}

bool fillAndTestVariableSizeStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	std::deque<uint32_t> codedPositions;
	if (!fillVariableSize(realNumbers, codedPositions, adapter))
		return false;
	adapter.resetPtrs();
	return testVariableSizeStreaming(realNumbers, adapter);
}

bool fillVariableSizeNegative(const std::deque<uint32_t> & realNumbers, std::deque<uint32_t> & codedPositions, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	codedPositions.resize(realNumbers.size(), 0);
	for(size_t i = 0; i < realNumbers.size(); i++) {
		int32_t num = -realNumbers.at(i);
		int len = adapter.putVlPackedInt32(curPos, num);
		if (len > 0) {
			codedPositions[i] = curPos;
			curPos += len;
		}
		else {
			std::cout << "Failed to encode number " << num << std::endl;
			return false;
		}
	}
	return true;
}

bool fillVariableSizeNegativeStreaming(const std::deque<uint32_t> & realNumbers, UByteArrayAdapter & adapter) {
	for(size_t i = 0; i < realNumbers.size(); i++) {
		int32_t num = -realNumbers.at(i);
		int len = adapter.putVlPackedInt32(num);
		if (len < 0) {
			std::cout << "Failed to encode number " << num << std::endl;
			return false;
		}
	}
	return true;
}

bool testVariableSizeNegative(const std::deque<uint32_t> & realNumbers, const std::deque<uint32_t> & codedPositions, UByteArrayAdapter adapter) {
	//Now test them
	for(size_t i = 0; i < realNumbers.size(); i++) {
		int32_t num = -realNumbers.at(i);
		int32_t codedNum;
		int len;
		codedNum = adapter.getVlPackedInt32(codedPositions[i], &len);
		if (len <= 0 || codedNum != num) {
			return false;
		}
	}
	return true;
}

bool testVariableSizeNegativeStreaming(const std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	//Now test them
	for(size_t i = 0; i < realNumbers.size(); i++) {
		int32_t num = -realNumbers.at(i);
		int32_t codedNum;
		codedNum = adapter.getVlPackedInt32();
		if (codedNum != num) {
			return false;
		}
	}
	return true;
}

bool fillAndTestVariableSizeNegative(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	std::deque<uint32_t> codedPositions;
	if (!fillVariableSizeNegative(realNumbers, codedPositions, adapter))
		return false;
	return testVariableSizeNegative(realNumbers, codedPositions, adapter);
}

bool fillAndTestVariableSizeNegativeStreaming(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	std::deque<uint32_t> codedPositions;
	if (!fillVariableSizeNegative(realNumbers, codedPositions, adapter))
		return false;
	return testVariableSizeNegativeStreaming(realNumbers, adapter);
}

bool fillStreamingAndTestVariableSizeNegativeStreaming(std::deque<uint32_t> & realNumbers) {
	std::deque<uint8_t> data;
	UByteArrayAdapter adapter(&data);
	if (!fillVariableSizeNegativeStreaming(realNumbers, adapter))
		return false;
	adapter.resetPtrs();
	return testVariableSizeNegativeStreaming(realNumbers, adapter);
}

bool fillAndTestFixedSizeUtilityFuncs(std::deque<uint32_t> & realNumbers, UByteArrayAdapter adapter) {
	uint32_t curPos = 0;
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			adapter.putUint8(curPos, num);
			curPos += 1;
			break;
		case(2):
			adapter.putUint16(curPos, num);
			curPos += 2;
			break;
		case(3):
			adapter.putUint24(curPos, num);
			curPos += 3;
			break;
		case(4):
			adapter.putUint32(curPos, num);
			curPos += 4;
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
	}

	UByteArrayAdapter testAdapter(adapter);
	//Now test them
	curPos = 0;
	if (testAdapter.offset() != curPos) {
		std::cout << "Initial offsets are wrong" << std::endl;
	}
	for(size_t i = 0; i < realNumbers.size(); i++) {
		uint32_t num = realNumbers.at(i);
		uint32_t codedNum = 0;
		switch (CompactUintArray::minStorageBitsFullBytes(num)) {
		case(1):
			codedNum = testAdapter.getUint8(0);
			testAdapter++;
			curPos += 1;
			if (testAdapter.offset() != curPos) {
				std::cout << "Offset is wrong after ++! Should: " << curPos << " is: " << testAdapter.offset() << std::endl;
				return false;
			}
			break;
		case(2):
			codedNum = testAdapter.getUint16(0);
			testAdapter += 2;
			curPos += 2;
			if (testAdapter.offset() != curPos) {
				std::cout << "Offset is wrong after +=!" << std::endl;
				return false;
			}
			break;
		case(3):
			codedNum = testAdapter.getUint24(0);
			testAdapter = testAdapter + 3;
			curPos += 3;
			if (testAdapter.offset() != curPos) {
				std::cout << "Offset is wrong after operator+(): Should" << curPos << "; Is: " << testAdapter.offset() << "; len: " << testAdapter.size() << std::endl;
				return false;
			}
			break;
		case(4):
			codedNum = testAdapter.getUint32(0);
			testAdapter = UByteArrayAdapter(testAdapter, 4);
			curPos += 4;
			if (testAdapter.offset() != curPos) {
				std::cout << "Offset is wrong after UByteArrayAdapter(, 4)!" << std::endl;
				return false;
			}
			break;
		default:
			std::cout << "ERROR" << std::endl;
			break;
		}
		if (codedNum != num) {
// 			std::cout << "Failed with codedNum=" << codedNum << " and realNum=" << num << std::endl;
			return false;
		}
	}
	return true;
}

std::deque<uint32_t> createNumber(int testCount) {
	std::deque<uint32_t> realNumbers;
	//Fill the first
	uint32_t rndNum;
	uint32_t rndMask;
	uint32_t mask;
	for(int i = 0; i < testCount; i++) {
		rndNum = rand();
		rndMask = (double)rand()/RAND_MAX * 31; 
		mask = ((rndMask+1 == 32) ? 0xFFFFFFFF : ((static_cast<uint32_t>(1) << (rndMask+1)) - 1));
		realNumbers.push_back(rndNum & mask);
	}
	return realNumbers;
}

bool fillAndTestComparissonOperators() {
	std::deque<uint8_t> ad( createNumbers8(0xFFF));
	UByteArrayAdapter adap(&ad);
	if (!adap.equalContent(ad)) {
		return false;
	}
	else {
		return true;
	}
}


bool testPrivateIndependent(std::deque<uint32_t> & realNumbers) {
	bool passed = true;

	if (fillAndTestStreamingApi(realNumbers)) {
		std::cout << "Passed fixed size streaming deque test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed fixed size streaming deque test" << std::endl;
	}

	if (fillFixedSizeStreamingAndTestFixedSize(realNumbers)) {
		std::cout << "Passed fixed size streaming, test fixed size deque" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed fixed size streaming, test fixed size deque" << std::endl;
	}

	if (fillStreamingAndTestVariableSizeStreaming(realNumbers)) {
		std::cout << "Passed variable size streaming deque test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size streaming deque test" << std::endl;
	}

	if (fillStreamingAndTestVariableSizeNegativeStreaming(realNumbers)) {
		std::cout << "Passed variable size with negative number, streaming write/read deque test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size with negative number, streaming write/read deque test" << std::endl;
	}
	
	if (fillAndTestComparissonOperators()) {
		std::cout << "Passed comparisson operators test"  << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed comparisson operators test"  << std::endl;
	}

	
	return passed;
}

bool testStringFunctions(uint16_t maxStrLen, uint32_t strCount) {
	bool passed = true;

	if (fillAndTestStrings(maxStrLen, strCount)) {
		std::cout << "Passed put(pos,String) with get(pos) test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed put(pos,String) with get(pos) test" << std::endl;
	}

	if (fillAndTestStringsStreaming(maxStrLen, strCount)) {
		std::cout << "Passed put(pos,String) with >> test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed put(pos,String) with >> test" << std::endl;
	}

	if (fillAndTestStringsData(maxStrLen, strCount)) {
		std::cout << "Passed put(pos,String) with get(pos) test (strData)" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed put(pos,String) with get(pos) test (strData)" << std::endl;
	}

	if (fillAndTestStringsDataStreaming(maxStrLen, strCount)) {
		std::cout << "Passed put(pos,String) with >> test (strData)" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed put(pos,String) with >> test (strData)" << std::endl;
	}

	if (fillStreamingAndTestStrings(maxStrLen, strCount)) {
		std::cout << "Passed string streaming with get(pos) test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed string streaming with get(pos) test" << std::endl;
	}

	if (fillStreamAndTestStringsStreaming(maxStrLen, strCount)) {
		std::cout << "Passed string streaming test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed string streaming test" << std::endl;
	}
	
	return passed;
}

bool testPrivateDependent(std::deque<uint32_t> & realNumbers, UByteArrayAdapter destinationAdap) {

	bool passed = true;

	if (fillAndTestFixedSize(realNumbers, destinationAdap)) {
		std::cout << "Passed fixed size test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed fixed size test" << std::endl;
	}

	if (fillFixedSizeAndTestStreaming(realNumbers, destinationAdap)) {
		std::cout << "Passed fixed size fill test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed fixed size fill test" << std::endl;
	}

	if (fillAndTestVariableSize(realNumbers, destinationAdap)) {
		std::cout << "Passed variable size test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size test" << std::endl;
	}

	if (fillAndTestVariableSizeStreaming(realNumbers, destinationAdap)) {
		std::cout << "Passed variable size fixed put, streaming read test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size fixed put, streaming read test" << std::endl;
	}

	if (fillAndTestVariableSizeNegative(realNumbers, destinationAdap)) {
		std::cout << "Passed variable size with negative number test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size with negative number test" << std::endl;
	}

	if (fillAndTestVariableSizeNegativeStreaming(realNumbers, destinationAdap)) {
		std::cout << "Passed variable size with negative number, streaming read test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed variable size with negative number, streaming read test" << std::endl;
	}

	//Now test the utility functions +=, ++ and +;
	if (fillAndTestFixedSizeUtilityFuncs(realNumbers, destinationAdap)) {
		std::cout << "Passed utility functions test with fixed size number test" << std::endl;
	}
	else {
		passed = false;
		std::cout << "Failed utility functions test with fixed size number test" << std::endl;
	}
	
	return passed;
}

bool test(int testCount) {
	bool allOk = true;
	std::deque<uint32_t> realNumbers = createNumber(testCount);
	
	std::cout << "Testing streaming API" << std::endl;
	if (testPrivateIndependent(realNumbers)) {
		std::cout << "Passed streaming API test" << std::endl;
	}
	else {
		std::cout << "FAILED streaming API test" << std::endl;
		allOk = false;
	}

	std::cout << "Testing string API" << std::endl;
	if (testStringFunctions(32, 1024)) {
		std::cout << "Passed string API test" << std::endl;
	}
	else {
		std::cout << "FAILED string API test" << std::endl;
		allOk = false;
	}
	
	std::cout << "Testing string (large strings) API" << std::endl;
	if (testStringFunctions(0xFFFF, 0xF)) {
		std::cout << "Passed string (large strings) API test" << std::endl;
	}
	else {
		std::cout << "FAILED string (large strings) API test" << std::endl;
		allOk = false;
	}

	
	uint8_t * arrayData = new uint8_t[4*testCount];
	UByteArrayAdapter arrayAdap(arrayData, 0, 4*testCount);
	std::cout << "Testing UByteArrayAdapterPrivateArray" << std::endl;
	if (testPrivateDependent(realNumbers, arrayAdap)) {
		std::cout << "UByteArrayAdapterPrivateArray PASSED tests!" << std::endl;
	}
	else {
		std::cout << "UByteArrayAdapterPrivateArray FAILED tests!" << std::endl;
		allOk = false;
	}
	delete[] arrayData;
	
	UByteArrayAdapter dequeAdap(new std::deque<uint8_t>(), true);
	dequeAdap.growStorage(4*testCount);
	std::cout << "Testing UByteArrayAdapterPrivateDeque" << std::endl;
	if (testPrivateDependent(realNumbers, dequeAdap)) {
		std::cout << "UByteArrayAdapterPrivateDeque PASSED tests!" << std::endl;
	}
	else {
		std::cout << "UByteArrayAdapterPrivateDeque FAILED tests!" << std::endl;
		allOk = false;
	}

	UByteArrayAdapter mmappedFileAdap = UByteArrayAdapter::createCache(0, sserialize::MM_FILEBASED);
	if (!mmappedFileAdap.growStorage(4*testCount)) {
		std::cout << "Failed to grow storage" << std::endl;
	}
	std::cout << "Testing UByteArrayAdapterPrivateMmappedFile" << std::endl;
	if (testPrivateDependent(realNumbers, mmappedFileAdap)) {
		std::cout << "UByteArrayAdapterPrivateMmappedFile PASSED tests!" << std::endl;
	}
	else {
		std::cout << "UByteArrayAdapterPrivateMmappedFile FAILED tests!" << std::endl;
		allOk = false;
	}
	
	sserialize::MmappedFile::createFile("ubatestseekedfile.tmp", 10);
	UByteArrayAdapter seekedFileAdap = UByteArrayAdapter::open("ubatestseekedfile.tmp", true, 5, 0);
	if (!seekedFileAdap.growStorage(4*testCount)) {
		std::cout << "Failed to grow storage" << std::endl;
	}
	std::cout << "Testing UByteArrayAdapterPrivateSeekedFile" << std::endl;
	if (testPrivateDependent(realNumbers, seekedFileAdap)) {
		std::cout << "UByteArrayAdapterPrivateSeekedFile PASSED tests!" << std::endl;
	}
	else {
		std::cout << "UByteArrayAdapterPrivateSeekedFile FAILED tests!" << std::endl;
		allOk = false;
	}
	sserialize::MmappedFile::unlinkFile("ubatestseekedfile.tmp");
	
	return allOk;
}

bool testVeryLargeFileCreation() {
	uint64_t largeFileSize = static_cast<uint64_t>( std::numeric_limits<uint32_t>::max()) + 17*1024*1024;
	uint64_t testPos = largeFileSize-48;
	
	if (std::numeric_limits<size_t>::max() < largeFileSize)
		std::cout << "Unable to map files larger than 4GB";
	
	UByteArrayAdapter testFileAdap(UByteArrayAdapter::createFile(2, "uba_largefiletest.bin"));
	if (!testFileAdap.growStorage(largeFileSize-2)) {
		std::cout << "Failed to grow storage to " << largeFileSize << " Bytes" << std::endl;
		return false;
	}
	testFileAdap.putUint16(testPos, 0xFEFE);
	testFileAdap = UByteArrayAdapter();
	testFileAdap = UByteArrayAdapter::open("uba_largefiletest.bin");
	testFileAdap.setDeleteOnClose(true);
	if (testFileAdap.getUint16(testPos) != 0xFEFE) {
		std::cout << "Failed large file test" << std::endl;
		return false;
	}
	std::cout << "Very large file creation test PASSED" << std::endl;
	return true;
}

int main(int argc, char** argv) {

	srand( 0 );

	int maxRuns = 1;
	if (argc > 1) {
		maxRuns = atoi(argv[1]);
	}

	bool allOk = true;

	for(int i=0; i < maxRuns; i++) {
		if (test(0xFFFFF)) {
			std::cout << "Passed all tests in run " << i << std::endl;
		}
		else {
			allOk = false;
			std::cout << "Failed some tests in run " << i << std::endl;
		}
	}
	
#ifdef __LP64__
	if (! testVeryLargeFileCreation())
		allOk = false;
#endif
	std::cout <<  "SUMMARY: ";
	if (allOk) std::cout << "OK! All tests passed" << std::endl;
	else std::cout << "FAILED! Some tests failed" << std::endl;
	return 0;
}