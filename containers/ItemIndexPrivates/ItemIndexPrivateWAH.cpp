#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h>
#include <sserialize/containers/UDWIterator.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/pack_unpack_functions.h>
#include <iostream>

namespace sserialize {

ItemIndexPrivateWAH::ItemIndexPrivateWAH(const UByteArrayAdapter & data) :
m_data(UByteArrayAdapter(data, 8, data.getUint32(0))),
m_size(data.getUint32(4)),
m_dataOffset(0),
m_curId(0),
m_cache(UByteArrayAdapter::createCache(std::min<uint32_t>(1024, m_size*4), false) )
{}

ItemIndexPrivateWAH::ItemIndexPrivateWAH() : m_size(0),  m_dataOffset(0), m_curId(0){}

ItemIndexPrivateWAH::~ItemIndexPrivateWAH() {}

ItemIndex::Types ItemIndexPrivateWAH::type() const {
	return ItemIndex::T_WAH;
}

int ItemIndexPrivateWAH::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

uint32_t ItemIndexPrivateWAH::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	for(;m_cache.tellPutPtr()/4 <= pos;) {
		uint32_t val = m_data.getUint32(m_dataOffset);
		m_dataOffset += 4;
		if (val & 0x1) { //rle encoded
			val >>= 1;//move out indicator bit
			if (val & 0x1) {//all ones, push corresponding ids
				uint32_t count = (val >> 1)*31;
				uint32_t curId = m_curId;
				for(std::size_t i = 0; i < count; ++i) {
					m_cache.putUint32(curId);
					++curId;
				}
			}
			val >>= 1;
			m_curId += val*31;
		}
		else {//no rle encoding, full 31 bits are use to encode values
			val >>= 1; //move out the indicator bit
			uint32_t curId = m_curId;
			while (val) {
				if (val & 0x1) {
					m_cache.putUint32(curId);
				}
				++curId;
				val >>= 1;
			}
			m_curId += 31;
		}
	}
	return m_cache.getUint32(pos*4);
}

uint32_t ItemIndexPrivateWAH::first() const {
	if (size())
		return at(0);
	return 0;
}

uint32_t ItemIndexPrivateWAH::last() const {
	if (size())
		return at(size()-1);
	return 0;
}

uint32_t ItemIndexPrivateWAH::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateWAH::bpn() const { return m_data.size()*8/m_size; }


uint32_t ItemIndexPrivateWAH::getSizeInBytes() const { return m_data.size() + 8; }

void ItemIndexPrivateWAH::putInto(DynamicBitSet & bitSet) const {
	if (!size())
		return;
	UByteArrayAdapter & destData = bitSet.data();
	uint32_t destDataSize = destData.size();
	uint32_t bitCount = 0;
	
	uint32_t mySize = m_data.size();
	for(uint32_t dataOffset = 0; dataOffset < mySize; dataOffset += 4) {
		uint32_t val = m_data.getUint32(dataOffset);
		m_dataOffset += 4;
		if (val & 0x1) { //rle encoded
			val >>= 1;//move out indicator bit
			if (val & 0x1) {//all ones, push corresponding ids
				uint32_t count = (val >> 1)*31;
				uint32_t destDataOffset = (bitCount+31*count) / 8;
				if (destDataSize < destDataOffset+5) {
					destData.growStorage(destDataOffset+5 - destDataSize);
					destDataSize += destDataOffset+5 - destDataSize;
				}
				for(std::size_t i = 0; i < count; ++i) {
					destDataOffset = bitCount / 8;
					uint8_t shift = bitCount % 8;
					
					uint64_t myVal = 0x7FFFFFFF; //move out the indicator bit
					myVal <<= shift;
					for(uint32_t myDestDataOffset = 0; myVal; ++myDestDataOffset) {
						destData[destDataOffset+myDestDataOffset] |= myVal;
						myVal >>= 8;
					}
					bitCount += 31;
				}
			}
			val >>= 1; //move out sign bit
			bitCount += 31*val;
		}
		else {//no rle encoding, full 31 bits are use to encode values
			uint32_t destDataOffset = bitCount / 8;
			uint8_t shift = bitCount % 8;
			
			if (destDataSize < destDataOffset+5) {
				destData.growStorage(destDataOffset+5 - destDataSize);
				destDataSize += destDataOffset+5 - destDataSize;
			}
			uint64_t myVal = val >> 1; //move out the indicator bit
			myVal <<= shift;
			for(uint32_t myDestDataOffset = 0; myVal; ++myDestDataOffset) {
				destData[destDataOffset+myDestDataOffset] |= myVal;
				myVal >>= 8;
			}
			bitCount += 31;
		}
	}
}

ItemIndexPrivate * ItemIndexPrivateWAH::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData = bitSet.data();
	UByteArrayAdapter tmpData(UByteArrayAdapter::createCache(bitSet.data().size(), false));
	tmpData.putUint32(0); //dummy data size
	tmpData.putUint32(0); //dummy count
	
	uint32_t count = 0;
	uint32_t initShift = 0;
	
	uint32_t curEncWord = 0;
	uint32_t bitSetDataOffset = 0;
	uint32_t bitSetDataSize = bitSetData.size();
	for(;bitSetDataOffset < bitSetDataSize;) {
		uint32_t val = 0;
		{
			val = static_cast<uint32_t>(bitSetData.at(bitSetDataOffset)) >> initShift;
			uint8_t curShift = 8 - initShift;
			++bitSetDataOffset;
			uint8_t bitsRead = 8 - initShift;
			while (bitSetDataOffset < bitSetDataSize) {
				uint32_t newByte = bitSetData.at(bitSetDataOffset);
				newByte = (newByte << (curShift));
				val |= newByte;
				bitsRead += 8;
				if (bitsRead < 31) {
					++bitSetDataOffset;
					curShift += 8;
				}
				else {
					if (bitsRead == 31)
						++bitSetDataOffset;
					break;
				}
			}
			initShift = (initShift + 31) % 8;
		}
		val <<= 1; //moves out a possible false most-signifcant bit
		uint8_t bitCount = popCount(val);
		count += bitCount;
		if (bitCount == 31) {
			if ((curEncWord & 0x1) && !(curEncWord & 0x2)) { //this means there's an one-encoding rle ahead
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			curEncWord >>= 2;
			++curEncWord;
			curEncWord <<= 2;
			curEncWord  |= 0x3;
		}
		else if (bitCount == 0) {
			if ((curEncWord & 0x1) && (curEncWord & 0x2)) { //this means there's an one-encoding rle ahead
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			curEncWord >>= 2;
			++curEncWord;
			curEncWord <<= 2;
			curEncWord  |= 0x1;
		}
		else {
			if (curEncWord) { //there is a rle word before
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			tmpData.putUint32(val); //val is already shifted one to the left
		}
	}
	if (curEncWord) {
		tmpData.putUint32(curEncWord);
		curEncWord = 0;
	}
	tmpData.putUint32(0, tmpData.tellPutPtr()-8);
	tmpData.putUint32(4, count);
	
	return new ItemIndexPrivateWAH(tmpData);
}


ItemIndexPrivate * ItemIndexPrivateWAH::intersect(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateWAH * cother = dynamic_cast<const ItemIndexPrivateWAH*>(other);
	if (!cother)
		return ItemIndexPrivate::doIntersect(other);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	dest.putUint32(0); //dummy data size
	dest.putUint32(0); //dummy count
	uint32_t resSize = 0;
	
	//Preconditions: both have a size larger than 0 but it should work without it as well, as UBA returns zero on overflow
	uint32_t myDataSize = m_data.size()/4;
	uint32_t oDataSize = cother->m_data.size()/4;
	uint32_t myDataOffset = 1;
	uint32_t oDataOffset = 1;
	uint32_t myEncWord = m_data.getUint32(0);
	uint32_t oEncWord = cother->m_data.getUint32(0);
	uint32_t prevDest = 0;
	do {
		if ((myEncWord & 0x1) && (oEncWord & 0x1)) {
			//get the types
			uint32_t myType = myEncWord & 0x3;
			uint32_t oType = oEncWord & 0x3;
			uint32_t destType = myType & oType;
			
			//prepare to check the sizes
			myEncWord >>= 2;
			oEncWord >>= 2;
			uint32_t destCount = std::min(myEncWord, oEncWord);
			
			//andjust in words
			myEncWord -= destCount;
			oEncWord -= destCount;
			
			if (destType & 0x2)//rled ones
				resSize += destCount*31;
			destCount <<= 2;
			destCount |= destType;
			prevDest = destCount;
			dest.putUint32(prevDest);
			
			if (myEncWord) {
				myEncWord <<= 2;
				myEncWord |= myType;
			}
			else {
				myEncWord = m_data.getUint32(myDataOffset*4);
				++myDataOffset;
			}
			
			if (oEncWord) {
				oEncWord <<= 2;
				oEncWord |= oType;
			}
			else {
				oEncWord = cother->m_data.getUint32(oDataOffset*4);
				++oDataOffset;
			}
		}
		else if ((myEncWord & 0x1) && !(oEncWord & 0x1)) {
			uint32_t myType = myEncWord & 0x3;
			myEncWord >>= 2;
			--myEncWord;
			if (myType & 0x2) {
				prevDest = oEncWord;
				dest.putUint32(prevDest);
				resSize += popCount(oEncWord);
			}
			else {
				if (prevDest & 0x1 && !(prevDest & 0x2)) {//update rle
					prevDest >>= 2;
					++prevDest;
					prevDest <<= 2;
					prevDest |= 1;
					dest.putUint32(dest.tellPutPtr()-4, prevDest);
				}
				else {
					dest.putUint32(0);
				}
			}
			
			if (myEncWord) {
				myEncWord <<= 2;
				myEncWord |= myType;
			}
			else {
				myEncWord = m_data.getUint32(myDataOffset*4);
				++myDataOffset;
			}

			
			oEncWord = cother->m_data.getUint32(oDataOffset*4);
			++oDataOffset;
		}
		else if (!(myEncWord & 0x1) && (oEncWord & 0x1)) {
			uint32_t oType = oEncWord & 0x3;
			
			oEncWord >>=2;
			oEncWord--;
			
			if (oType & 0x2) {
				prevDest = myEncWord;
				dest.putUint32(prevDest);
				resSize += popCount(myEncWord);
			}
			else {
				if (prevDest & 0x1 && !(prevDest & 0x2)) {//update rle
					prevDest >>= 2;
					++prevDest;
					prevDest <<= 2;
					prevDest |= 1;
					dest.putUint32(dest.tellPutPtr()-4, prevDest);
				}
				else {
					dest.putUint32(0);
				}
			}
			
			if (oEncWord) {
				oEncWord <<= 2;
				oEncWord |= oType;
			}
			else {
				oEncWord = cother->m_data.getUint32(oDataOffset*4);
				++oDataOffset;
			}
			
			myEncWord = m_data.getUint32(myDataOffset*4);
			++myDataOffset;
		}
		else {
			prevDest = myEncWord & oEncWord;
			resSize += popCount(prevDest);
			dest.putUint32(prevDest);
			myEncWord = m_data.getUint32(myDataOffset*4);
			oEncWord = cother->m_data.getUint32(oDataOffset*4);
			++myDataOffset;
			++oDataOffset;
		}
	} while (myDataOffset <= myDataSize && oDataOffset <= oDataSize);
	//no need to check for anything else as those will all be zeros
	
	dest.putUint32(4, resSize);
	dest.putUint32(0, dest.tellPutPtr()-8);
	dest.resetGetPtr();
	return new ItemIndexPrivateWAH(dest);
}

ItemIndexPrivate * ItemIndexPrivateWAH::unite(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateWAH * cother = dynamic_cast<const ItemIndexPrivateWAH*>(other);
	if (!cother)
		return ItemIndexPrivate::doUnite(other);
	UByteArrayAdapter dest( UByteArrayAdapter::createCache(8, false));
	dest.putUint32(0); //dummy data size
	dest.putUint32(0); //dummy count
	uint32_t resSize = 0;
	
	//Preconditions: both have a size larger than 0 but it should work without it as well, as UBA returns zero on overflow
	uint32_t myDataSize = m_data.size()/4;
	uint32_t oDataSize = cother->m_data.size()/4;
	uint32_t myDataOffset = 1;
	uint32_t oDataOffset = 1;
	uint32_t myEncWord = m_data.getUint32(0);
	uint32_t oEncWord = cother->m_data.getUint32(0);
	uint32_t prevDest = 0;
	do {
		if (!(myEncWord & 0x1) && !(oEncWord & 0x1)) {
			prevDest = myEncWord | oEncWord;
			resSize += popCount(prevDest);
			dest.putUint32(prevDest);
			myEncWord = m_data.getUint32(myDataOffset*4);
			oEncWord = cother->m_data.getUint32(oDataOffset*4);
			++myDataOffset;
			++oDataOffset;
		}
		else if ((myEncWord & 0x1) && !(oEncWord & 0x1)) {
			uint32_t myType = myEncWord & 0x3;
			myEncWord >>= 2;
			--myEncWord;
			if (myType & 0x2) { //all ones
				if (prevDest & 0x1 && (prevDest & 0x2)) {//update rle
					prevDest >>= 2;
					++prevDest;
					prevDest <<= 2;
					prevDest |= 1;
					dest.putUint32(dest.tellPutPtr()-4, prevDest);
					resSize += 31;
				}
				else {
					prevDest = 0x7;
					dest.putUint32(prevDest); //rle one 31
					resSize += 31;
				}
			}
			else {//all zeros
				prevDest = oEncWord;
				dest.putUint32(prevDest);
				resSize += popCount(prevDest);
			}
			
			if (myEncWord) {
				myEncWord <<= 2;
				myEncWord |= myType;
			}
			else {
				myEncWord = m_data.getUint32(myDataOffset*4);
				++myDataOffset;
			}

			
			oEncWord = cother->m_data.getUint32(oDataOffset*4);
			++oDataOffset;
		}
		else if (!(myEncWord & 0x1) && (oEncWord & 0x1)) {
			uint32_t oType = oEncWord & 0x3;
			
			oEncWord >>=2;
			oEncWord--;
			
			if (oType & 0x2) {
				if (prevDest & 0x1 && !(prevDest & 0x2)) {//update rle
					prevDest >>= 2;
					++prevDest;
					prevDest <<= 2;
					prevDest |= 1;
					dest.putUint32(dest.tellPutPtr()-4, prevDest);
					resSize += 31;
				}
				else {
					prevDest = 0x7;
					dest.putUint32(prevDest);
					resSize += 31;
				}
			}
			else {
				prevDest = myEncWord;
				dest.putUint32(prevDest);
				resSize += popCount(prevDest);
			}
			
			if (oEncWord) {
				oEncWord <<= 2;
				oEncWord |= oType;
			}
			else {
				oEncWord = cother->m_data.getUint32(oDataOffset*4);
				++oDataOffset;
			}
			
			myEncWord = m_data.getUint32(myDataOffset*4);
			++myDataOffset;
		}
		else {
			//get the types
			uint32_t myType = myEncWord & 0x3;
			uint32_t oType = oEncWord & 0x3;
			uint32_t destType = myType | oType;
			
			//prepare to check the sizes
			myEncWord >>= 2;
			oEncWord >>= 2;
			uint32_t destCount = std::min(myEncWord, oEncWord);
			
			//andjust in words
			myEncWord -= destCount;
			oEncWord -= destCount;
			
			if (destType & 0x2)//rled ones
				resSize += destCount*31;
			destCount <<= 2;
			destCount |= destType;
			prevDest = destCount;
			dest.putUint32(prevDest);
			
			if (myEncWord) {
				myEncWord <<= 2;
				myEncWord |= myType;
			}
			else {
				myEncWord = m_data.getUint32(myDataOffset*4);
				++myDataOffset;
			}
			
			if (oEncWord) {
				oEncWord <<= 2;
				oEncWord |= oType;
			}
			else {
				oEncWord = cother->m_data.getUint32(oDataOffset*4);
				++oDataOffset;
			}
		}

	} while (myDataOffset <= myDataSize && oDataOffset <= oDataSize);
	
	if (myDataOffset <= myDataSize) {
		//check for remaining merges, oData is zerod
		if ((prevDest & 0x1) && (prevDest & 0x2) && (myEncWord & 0x1) && (myEncWord & 0x2)) {
			resSize += (myEncWord >> 2);
			prevDest >>= 2;
			prevDest += (myEncWord >> 2);
			prevDest <<= 2;
			prevDest |= 0x3;
			dest.putUint32(dest.tellPutPtr()-4, prevDest);
			myEncWord = m_data.getUint32(myDataOffset*4);
			++myDataOffset;
		}
		if (myDataOffset <= myDataSize) {
			do {
				dest.putUint32(myEncWord);
				if ((myEncWord & 0x1)) {
					if (myEncWord & 0x2)
						resSize += (myEncWord >>  2);
				}
				else {
					resSize += popCount(myEncWord);
				}
				myEncWord = m_data.getUint32(myDataOffset*4);
				++myDataOffset;
			} while( myDataOffset <= myDataSize);
		}
	}
	else if (oDataOffset <= oDataSize) {
		//check for remaining merges, oData is zerod
		if ((prevDest & 0x1) && (prevDest & 0x2) && (oEncWord & 0x1) && (oEncWord & 0x2)) {
			resSize += (oEncWord >> 2);
			prevDest >>= 2;
			prevDest += (oEncWord >> 2);
			prevDest <<= 2;
			prevDest |= 0x3;
			dest.putUint32(dest.tellPutPtr()-4, prevDest);
			oEncWord = cother->m_data.getUint32(oDataOffset*4);
			++oDataOffset;
		}
		if (oDataOffset <= oDataSize) {
			do {
				dest.putUint32(oEncWord);
				if ((oEncWord & 0x1)) {
					if (oEncWord & 0x2)
						resSize += (oEncWord >>  2);
				}
				else {
					resSize += popCount(oEncWord);
				}
				oEncWord = cother->m_data.getUint32(oDataOffset*4);
				++oDataOffset;
			} while( oDataOffset <= oDataSize);
		}
	}

	dest.putUint32(4, resSize);
	dest.putUint32(0, dest.tellPutPtr()-8);
	dest.resetGetPtr();
	return new ItemIndexPrivateWAH(dest);
}

struct OutPutHandler {
	UByteArrayAdapter & data;
	uint32_t outPutWord;
	uint32_t idCount;
	OutPutHandler(UByteArrayAdapter & data) : data(data), outPutWord(0), idCount(0) {}
	void push() {
		if (outPutWord & 0x1) {
			if (outPutWord & 0x2) {
				idCount += 31*(outPutWord >> 2);
			}
		}
		else {
			idCount += popCount(outPutWord >> 1);
		}
		data.putUint32(outPutWord);
	}
	void append(uint32_t nextWord) {
		if ((outPutWord & 0x1) && (nextWord & 0x1)) { //both are run length encodings, check if we can merge them
			if ((nextWord & 0x2) == (outPutWord & 0x2)) { //equal rle type
				//add the lengths
				outPutWord = (outPutWord + (nextWord & (~static_cast<uint32_t>(0x3))));
			}
			else {
				push();
				outPutWord = nextWord;
			}
		}
		else {
			if (outPutWord)
				push();
			outPutWord = nextWord;
		}
	}
	
	///@return idCount
	uint32_t flush() {
		if (outPutWord)
			push();
		return idCount;
	}

};

ItemIndexPrivate * ItemIndexPrivateWAH::difference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateWAH * cother = dynamic_cast<const ItemIndexPrivateWAH*>(other);
	if (!cother)
		return ItemIndexPrivate::doDifference(other);

	UDWIterator myIt(m_data);
	UDWIterator oIt(cother->m_data);
	myIt.reset();
	oIt.reset();
	
	UByteArrayAdapter oData = UByteArrayAdapter::createCache(8, false);
	oData.putUint32(0); //dummy size
	oData.putUint32(0); //dummy count
	OutPutHandler oHandler(oData);
	
	uint32_t myVal = 0;
	uint32_t oVal = 0;
	while(myIt.hasNext() && oIt.hasNext()) {
		if ( ! (myVal >> 2) )
			myVal = myIt.next();
		if ( !(oVal >> 2) )
			oVal = oIt.next();
		
		uint32_t pushType = myVal & 0x3; //default to myVal
		uint8_t types = ((myVal & 0x3) << 2) | (oVal & 0x3);
		switch (types) {
		//myval with zero-rle
		case ((0x4 | 0x8) | 0x3): //myval is 1-rle and oVal is one-rle => pushType = zero-rle
			//only set pushType here and fall through to handling of the rest
			pushType = 0x1;
		case (0x4 | 0x1) ://myVal is zero-rle and oVal is zero-rle => pushType = myVal
		case (0x4 | 0x3) ://myVal is zero-rle and oVal is one-rle => pushType = myVal
		case ((0x4 | 0x8) | 0x1): //myval is 1-rle and oVal is zero-rle => pushType = myVal
		{
			if ((oVal & (~static_cast<uint32_t>(0x3))) <= (myVal & (~static_cast<uint32_t>(0x3)))) {
				oHandler.append((oVal & ~static_cast<uint32_t>(0x3)) | pushType);
				myVal -= (oVal & ~static_cast<uint32_t>(0x3));
				oVal = 0;
			}
			else {
				oHandler.append((myVal & ~static_cast<uint32_t>(0x3)) | pushType);
				oVal -= (myVal & ~static_cast<uint32_t>(0x3));
				myVal = 0;
			}
			break;
		}
		case (0x4 | 0x0): //myval is zero-rle and oval is no rle
		case (0x4 | 0x2):
		{
			oVal = 0;
			myVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append((0x4 | 0x1)); //1-rle zero
			break;
		}
		case ((0x4 | 0x8) | 0x0): //myval is 1-rle and oVal is no rle
		case ((0x4 | 0x8) | 0x2):
		{
			oHandler.append(~oVal & (~static_cast<uint32_t>(0x1)));
			myVal -= static_cast<uint32_t>(0x1) << 2;
			oVal = 0;
			break;
		}
		//myval no rle
		case (0x8 | 0x1): //myval no rle, oVal zero-rle
		case (0x0 | 0x1):
		{
			oVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(myVal);
			myVal = 0;
			break;
		}
		case(0x0 | 0x3): //myval no rle, oVal one-rle
		case(0x8 | 0x3):
		{
			oHandler.append((0x4 | 0x1)); //one zero-rle
			myVal = 0;
			oVal -= static_cast<uint32_t>(0x1) << 2;
			break;
		}
		case(0x0 | 0x0): //myval no-rle oval no-rle
		case(0x0 | 0x2):
		case(0x8 | 0x0):
		case(0x8 | 0x2):
		{
			uint32_t pushVal = myVal & (~oVal);
			pushVal = (pushVal ? pushVal : (0x4 | 0x1));
			oHandler.append(pushVal); //push either a 1-rle-zero or a non-rle
			myVal = 0;
			oVal = 0;
			break;
		}
		default:
			std::cerr << "BUG!!!!" << static_cast<uint32_t>(types) << std::endl;
			break;
		};
	}
	if (myVal >> 2) {
		oHandler.append(myVal);
	}
	while(myIt.hasNext()) {
		myVal = myIt.next();
		oHandler.append(myVal);
	}
	
	uint32_t idCount = oHandler.flush();
	oData.putUint32(0, oData.tellPutPtr()-8);
	oData.putUint32(4, idCount);
	oData.resetPtrs();
	return new ItemIndexPrivateWAH(oData);
}

ItemIndexPrivate * ItemIndexPrivateWAH::symmetricDifference(const sserialize::ItemIndexPrivate * other) const {
	const ItemIndexPrivateWAH * cother = dynamic_cast<const ItemIndexPrivateWAH*>(other);
	if (!cother)
		return ItemIndexPrivate::doSymmetricDifference(other);
	std::cerr << "UNIMPLEMENTED" << std::endl;
	return new ItemIndexPrivateEmpty();
}

//Optimilize:
//if an subtract index is at the end, remove it

// #define FF_SUPPORT

ItemIndex ItemIndexPrivateWAH::fusedIntersectDifference(const std::vector< ItemIndexPrivateWAH * >& intersect, const std::vector< ItemIndexPrivateWAH * >& subtract, uint32_t count) {
	std::vector<uint32_t> resultIds;
	resultIds.reserve(count);
	
	uint32_t intersectSize = intersect.size();
	uint32_t subtractSize = subtract.size();
	
	std::vector<uint32_t> intersectPositions(intersectSize, 0), intersectEncWords(intersectSize, 0);
	std::vector<uint32_t> subtractPositions(subtractSize, 0), subtractEncWords(subtractSize, 0);
	uint64_t bitOffset = 0;
	uint32_t currentWorkingWord = 0;
	
	//initialize variables
	for(uint32_t i = 0; i < intersectSize; ++i) {
		intersectEncWords[i] = intersect[i]->m_data.getUint32(0);
	}
	for(uint32_t i = 0; i < subtractSize; ++i) {
		subtractEncWords[i] = subtract[i]->m_data.getUint32(0);
	}

	bool allIntersectsHaveNext = true;
	while (resultIds.size() < count && allIntersectsHaveNext) {
		//process 31 Bits in  each round until either one intersect index is at the end or we have count elements in our result set
		currentWorkingWord = std::numeric_limits<uint64_t>::max();
#ifdef FF_SUPPORT
		//fast-forward support if all indices have rle encoding
		//take care of different possiblities:
		//intersect zeros => ff both
		//intersect ones && diffs ones => ff both
		//intersect ones && intersect zeros: no ff but insert into result set
		bool allIntsZeroRle = true;
		for(uint32_t i = 0; i < intersectSize; ++i) {
			if ( !(intersectEncWords[i] & 0x1) || intersectEncWords[i] & 0x2)
				allIntsZeroRle = false;
		}
		if (allIntsZeroRle) {
			uint32_t amount = std::numeric_limits<uint32_t>::max();
			for(uint32_t i = 0; i < intersectSize; ++i) {
				amount = std::min<uint32_t>(amount, (intersectEncWords[i] >> 2));
			}
			//adjust bit offset
			bitOffset += 31*amount;
			
			//now do the ff, we know that all are zeros rle
			for(uint32_t i = 0; i < intersectSize; ++i) {
				intersectEncWords[i] = (intersectEncWords[i] >> 2) - amount;
				if (!intersectEncWords[i]) { //load next word?
					intersectPositions[i] += 4;
					if (intersectPositions[i] < intersect[i]->m_data.size()) {
						intersectEncWords[i] = intersect[i]->m_data.getUint32(intersectPositions[i]);
					}
					else { //this should never happen as there has to be at least a single one after zero rle
						allIntersectsHaveNext = false;
						continue; //no need to check currentWorkingWord as we haven't touched it yet
					}
				}
				else { //word is still active
					intersectEncWords[i] = (intersectEncWords[i] << 2) | 0x1;
				}
			}
			
			//ff the diffs now
			for (uint32_t i = 0; i < subtractSize; ++i) {
				uint32_t myAmount = amount;
				while (myAmount) {
					bool loadNext = false;
					switch (subtractEncWords[i] & 0x3) {
					case 0x1:
					case 0x3:
						if ((subtractEncWords[i] >> 2) > myAmount) {
							myAmount = 0;
							subtractEncWords[i] = ((((subtractEncWords[i] >> 2) - myAmount) << 2) | (subtractEncWords[i] & 0x3));
						}
						else {
							myAmount -= subtractEncWords[i] >> 2;
							loadNext = true;
						}
						break;
					default:
						--myAmount;
						loadNext = true;
						break;
					}
					if (loadNext) {
						subtractPositions[i] += 4;
						if (subtractPositions[i] < subtract[i]->m_data.size()) {
							subtractEncWords[i] = subtract[i]->m_data.getUint32(subtractPositions[i]);
						}
						else {
							subtractEncWords[i] = 0;
						}
					}
				}
			}
		}
#endif
		//do the intersects
		for(uint32_t i = 0; i < intersectSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = intersectEncWords[i] & 0x3;
			switch (rleFlag) {
			case 0x1: //0s rle encoded
				currentWorkingWord = 0;
				//fall through to check for word
			case 0x3: //1s rle encoded, currentWorkingWord stays the smae
				intersectEncWords[i] = ((intersectEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !intersectEncWords[i]; //check if no rle anymore
				intersectEncWords[i] = (intersectEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= (intersectEncWords[i] >> 1);
				loadNext = true;
				break;
			}
			if (loadNext) {
				intersectPositions[i] += 4;
				if (intersectPositions[i] < intersect[i]->m_data.size()) {
					intersectEncWords[i] = intersect[i]->m_data.getUint32(intersectPositions[i]);
				}
				else {
					allIntersectsHaveNext = false;
				}
			}
		}
		
		//do the substracts
		for(uint32_t i = 0; i < subtractSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = subtractEncWords [i] & 0x3;
			switch (rleFlag) {
			case 0x3: //1s rle encoded, zero word
				currentWorkingWord = 0;
				//fall through to check for word
			case 0x1: //0s rle encoded, currentWorkingWord stays the same
				subtractEncWords[i] = ((subtractEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !intersectEncWords[i]; //check if no rle anymore
				subtractEncWords[i] = (subtractEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= ~ (subtractEncWords[i] >> 1);
				loadNext = true;
				break;
			}
			if (loadNext) {
				subtractPositions[i] += 4;
				if (subtractPositions[i] < subtract[i]->m_data.size()) {
					subtractEncWords[i] = subtract[i]->m_data.getUint32(subtractPositions[i]);
				}
				else {
					subtractEncWords[i] = 0;
				}
			}
		}
		
		for(uint32_t i = bitOffset; currentWorkingWord && i < bitOffset+31; ++i) {
			if (currentWorkingWord & 0x1)
				resultIds.push_back(i);
			currentWorkingWord >>= 1;
		}
		bitOffset += 31;
	}
	
	return ItemIndex::absorb(resultIds);
}
///@return returns if allIntersectsHaveNext
bool ItemIndexPrivateWAH::fastForwardIntersectZero(const std::vector< ItemIndexPrivateWAH * > & intersect, std::vector<uint32_t> & intersectPositions, std::vector<uint32_t> & intersectEncWords, uint64_t & bitOffset) {
	uint32_t intersectSize = intersect.size();
	//find the word with the largest zero
	uint32_t amount = std::numeric_limits<uint32_t>::min();
	for(uint32_t i = 0; i < intersectSize; ++i) {
		if ( (intersectEncWords[i] & 0x3) == 0x1)
			amount = std::max<uint32_t>(amount, (intersectEncWords[i] >> 2));
	}

	for (uint32_t i = 0; i < intersectSize; ++i) {
		uint32_t myAmount = amount;
		while (myAmount) {
			bool loadNext = false;
			if (intersectEncWords[i] & 0x1) {
				if ((intersectEncWords[i] >> 2) > myAmount) {
					myAmount = 0;
					intersectEncWords[i] = ((((intersectEncWords[i] >> 2) - myAmount) << 2) | (intersectEncWords[i] & 0x3));
				}
				else {
					myAmount -= intersectEncWords[i] >> 2;
					loadNext = true;
				}
			}
			else {
				--myAmount;
				loadNext = true;
			}
			if (loadNext) {
				intersectPositions[i] += 4;
				if (intersectPositions[i] < intersect[i]->m_data.size()) {
					intersectEncWords[i] = intersect[i]->m_data.getUint32(intersectPositions[i]);
				}
				else { //one intersect is at the end, no need to check for the others
					intersectEncWords[i] = 0;
					return false;
				}
			}
		}
	}
	
	bitOffset += amount*31;
	
	return true;
}

///@return returns true if allIntersectsHaveNext and resultIds.size() < count
bool ItemIndexPrivateWAH::fastForwardIntersectOne(
const std::vector< ItemIndexPrivateWAH * > & intersect, std::vector<uint32_t> & intersectPositions, 
std::vector<uint32_t> & intersectEncWords, std::vector<uint32_t> & resultIds, uint32_t count, uint64_t & bitOffset) {
	uint32_t intersectSize = intersect.size();
	bool allIntersectsHaveNext = true;

	uint32_t amount = std::numeric_limits<uint32_t>::max();
	for(uint32_t i = 0; i < intersectSize; ++i) {
		amount = std::min<uint32_t>(amount, (intersectEncWords[i] >> 2));
	}

	uint32_t curId = bitOffset;
	bitOffset += amount*31;
	for(; curId < bitOffset && resultIds.size() < count; ++curId) {
		resultIds.push_back(curId);
	}
	if (resultIds.size() >= count)
		return false;

	for(uint32_t i = 0; i < intersectSize; ++i) {
		intersectEncWords[i] = (intersectEncWords[i] >> 2) - amount;
		if (!intersectEncWords[i]) { //load next word?
			intersectPositions[i] += 4;
			if (intersectPositions[i] < intersect[i]->m_data.size()) {
				intersectEncWords[i] = intersect[i]->m_data.getUint32(intersectPositions[i]);
			}
			else {
				allIntersectsHaveNext = false;
				intersectEncWords[i] = 0;
			}
		}
		else { //word is still active, restore type
			intersectEncWords[i] = (intersectEncWords[i] << 2) | 0x3;
		}
	}
	return allIntersectsHaveNext;
}

// #define FF2_SUPPORT
// #define FF3_SUPPORT
ItemIndex ItemIndexPrivateWAH::constrainedIntersect(const std::vector< ItemIndexPrivateWAH * > & intersect, uint32_t count) {
	std::vector<uint32_t> resultIds;
	resultIds.reserve(count);
	
	const uint32_t intersectSize = intersect.size();
	
	std::vector<uint32_t> intersectPositions(intersectSize, 0), intersectEncWords(intersectSize, 0);
	uint64_t bitOffset = 0;
	uint32_t currentWorkingWord = 0;
	
	//initialize variables
	for(uint32_t i = 0; i < intersectSize; ++i) {
		intersectEncWords[i] = intersect[i]->m_data.getUint32(0);
	}

	bool allIntersectsHaveNext = true;
	while (resultIds.size() < count && allIntersectsHaveNext) {
		//process 31 Bits in  each round until either one intersect index is at the end or we have count elements in our result set
		currentWorkingWord = std::numeric_limits<uint64_t>::max();

		//fast-forward support if all indices have rle encoding
#ifdef FF2_SUPPORT
		bool allIntsOneRle = true;
#endif
#ifdef FF3_SUPPORT
		bool oneIntsZeroRle = false;
#endif
#if defined(FF2_SUPPORT) || defined(FF3_SUPPORT)
		for(uint32_t i = 0; i < intersectSize; ++i) {
#endif
#ifdef FF2_SUPPORT
			allIntsOneRle = allIntsOneRle && (intersectEncWords[i] & 0x1) && (intersectEncWords[i] & 0x2);
#endif
#ifdef FF3_SUPPORT
			oneIntsZeroRle = oneIntsZeroRle || ((intersectEncWords[i] & 0x1) && !(intersectEncWords[i] & 0x2));
#endif
#if defined(FF2_SUPPORT) || defined(FF3_SUPPORT)
		}
#endif
#ifdef FF2_SUPPORT
		if (allIntsOneRle) {
			if (! fastForwardIntersectOne(intersect, intersectPositions, intersectEncWords, resultIds, count, bitOffset) )
				return ItemIndex::absorb(resultIds);
		}
#endif
#if defined(FF2_SUPPORT) && defined(FF3_SUPPORT)
		else
#endif
#ifdef FF3_SUPPORT
		if (oneIntsZeroRle) {
			if ( !fastForwardIntersectZero(intersect, intersectPositions, intersectEncWords, bitOffset))
				return ItemIndex::absorb(resultIds);
			continue;
		}
#endif
		
		//do the intersects
		for(uint32_t i = 0; i < intersectSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = intersectEncWords[i] & 0x3;
			switch (rleFlag) {
			case 0x1: //0s rle encoded
				currentWorkingWord = 0;
				//fall through to check for word
			case 0x3: //1s rle encoded, currentWorkingWord stays the smae
				intersectEncWords[i] = ((intersectEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !(intersectEncWords[i]); //check if no rle anymore
				intersectEncWords[i] = (intersectEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= (intersectEncWords[i] >> 1);
				loadNext = true;
				break;
			}
			if (loadNext) {
				intersectPositions[i] += 4;
				if (intersectPositions[i] < intersect[i]->m_data.size()) {
					intersectEncWords[i] = intersect[i]->m_data.getUint32(intersectPositions[i]);
				}
				else {
					allIntersectsHaveNext = false;
				}
			}
		}
		
		for(uint32_t i = bitOffset; currentWorkingWord && i < bitOffset+31; ++i) {
			if (currentWorkingWord & 0x1)
				resultIds.push_back(i);
			currentWorkingWord >>= 1;
		}
		bitOffset += 31;
	}
	
	return ItemIndex::absorb(resultIds);
}



}//end namespace