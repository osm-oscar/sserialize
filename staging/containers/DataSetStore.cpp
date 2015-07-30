#include <sserialize/Static/DataSetStore.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/exceptions.h>
#include <vendor/libs/minilzo/minilzo.h>

namespace sserialize {
namespace Static {
namespace detail {

DataSetStore::LZODecompressor::LZODecompressor() {}

DataSetStore::LZODecompressor::LZODecompressor(const sserialize::UByteArrayAdapter & data) :
m_data(data)
{}

DataSetStore::LZODecompressor::~LZODecompressor() {}

OffsetType DataSetStore::LZODecompressor::getSizeInBytes() const {
	return m_data.getSizeInBytes();
}

UByteArrayAdapter DataSetStore::LZODecompressor::decompress(uint32_t id, const UByteArrayAdapter & src) const {
	if (m_data.maxCount() >= id) {
		uint32_t chunkLength = m_data.at(id);
		//Get a memory view, but we don't write to it, so it's ok
		const UByteArrayAdapter::MemoryView srcD = src.getMemView(0, src.size());
		uint8_t * dest = new uint8_t[chunkLength];

		lzo_uint destLen = chunkLength;
		int ok = ::lzo1x_decompress(srcD.get(), src.size(), dest, &destLen, 0);
		if (ok != LZO_E_OK) {
			return UByteArrayAdapter();
		}
		else {
			UByteArrayAdapter ret(dest, 0, destLen);
			ret.setDeleteOnClose(true);
			return ret;
		}
	}
	return UByteArrayAdapter();
}

DataSetStore::DataSetStore() {}

DataSetStore::DataSetStore(UByteArrayAdapter data) :
m_version(data.getUint8(0))
{
	data.resetGetPtr();
	data += 3;
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_DATA_SET_STORE_VERSION, m_version, "Static::DataSetStore");
	OffsetType dataLength = data.getOffset();
	data.shrinkToGetPtr();
	
	m_data = UByteArrayAdapter(data, 0, dataLength);
	data += dataLength;
	
	m_index = SortedOffsetIndex(data);
	data += m_index.getSizeInBytes();
	
	if (data.size()) {
		m_lzod.reset(new LZODecompressor(data));
	}
	
}

DataSetStore::~DataSetStore() {}

uint32_t DataSetStore::size() const {
	return m_index.size();
}

OffsetType DataSetStore::getSizeInBytes() const {
	OffsetType r = SerializationInfo<uint8_t>::length*3;
	r += UByteArrayAdapter::OffsetTypeSerializedLength();
	r += m_data.size();
	r += m_index.getSizeInBytes();
	if (m_lzod.get()) {
		r += m_lzod->getSizeInBytes();
	}
	return r;
}

UByteArrayAdapter DataSetStore::at(uint32_t pos) const {
	if (pos >= size())
		return UByteArrayAdapter();
	UByteArrayAdapter::OffsetType indexStart = m_index.at(pos);
	UByteArrayAdapter::OffsetType indexLength;
	if (pos+1 == size()) {
		indexLength = m_data.size() - indexStart;
	}
	else {
		indexLength = m_index.at(pos+1) - indexStart;
	}
	if (m_lzod.get()) {
		return m_lzod->decompress(pos, UByteArrayAdapter(m_data, indexStart, indexLength));
	}
	else {
		return UByteArrayAdapter(m_data, indexStart, indexLength);
	}
}

std::ostream& DataSetStore::printStats(std::ostream& out) const {
	out << "Static::DataSetStore::Stats->BEGIN" << std::endl;
	out << "Storage size: " << getSizeInBytes() << std::endl;
	out << "size: " << size() << std::endl;

	uint32_t largestIndex = 0;
	uint32_t largestSpaceUsageIndex = 0;
	for(uint32_t i(0), s(size()); i < s; ++i) {
		largestSpaceUsageIndex = std::max<uint32_t>(largestSpaceUsageIndex, this->at(i).size());
	}
	out << "Largest space usage: " << largestSpaceUsageIndex << std::endl;
	out << "Static::DataSetStore::Stats->END" << std::endl;
	return out;
}

}}}//end namespace