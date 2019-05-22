#pragma once
#ifndef ITEM_INDEX_PRIVATE_STL_RANGE_GENERATOR_H
#define ITEM_INDEX_PRIVATE_STL_RANGE_GENERATOR_H
#include "ItemIndexPrivate.h"
#include <sserialize/iterator/RangeGenerator.h>

namespace sserialize {

class ItemIndexPrivateRangeGenerator: public ItemIndexPrivate {
private:
	RangeGenerator<uint32_t> m_data;
public:
	ItemIndexPrivateRangeGenerator();
	ItemIndexPrivateRangeGenerator(const RangeGenerator<uint32_t> & data);
	virtual ~ItemIndexPrivateRangeGenerator();
	virtual ItemIndex::Types type() const override;
public:
	virtual uint32_t at(uint32_t pos) const override;
	virtual uint32_t first() const override;
	virtual uint32_t last() const override;
	
	virtual uint32_t size() const override;

	virtual uint8_t bpn() const override;

	virtual sserialize::UByteArrayAdapter::SizeType getSizeInBytes() const override;
};

}//end namespace

#endif
