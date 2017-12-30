#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/iterator/UDWIterator.h>
#include <sserialize/iterator/UDWIteratorPrivateHD.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h>
#include <minilzo/minilzo.h>

namespace sserialize {
namespace Static {

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::FixedLengthTag) :
m_d(d.getOffset(0))
{}

template<>
ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::VariableLengthTag) :
m_d(d.getVlPackedUint64(0))
{}

ItemIndexStoreId::ItemIndexStoreId(const UByteArrayAdapter& d, ItemIndexStoreId::SerializationType t) :
ItemIndexStoreId()
{
	switch(t) {
	case ST_FL:
		m_d = d.getOffset(0);
		break;
	case ST_VL:
		m_d = d.getVlPackedUint64(0);
		break;
	default:
		break;
	}
}

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::FixedLengthTag) const {
	return sserialize::UByteArrayAdapter::OffsetTypeSerializedLength();
}

template<>
sserialize::UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::VariableLengthTag) const {
	return sserialize::psize_vu64(m_d);
}

UByteArrayAdapter::OffsetType ItemIndexStoreId::getSizeInBytes(ItemIndexStoreId::SerializationType t) const {
	switch(t) {
	case ST_FL:
		return getSizeInBytes<ItemIndexStoreId::FixedLengthTag>();
	case ST_VL:
		return getSizeInBytes<ItemIndexStoreId::VariableLengthTag>();
	default:
		return 0;
	}
}

ItemIndexStore::ItemIndexStore() : m_priv(new detail::ItemIndexStore()) {}
ItemIndexStore::ItemIndexStore(const sserialize::UByteArrayAdapter & data) : m_priv(new detail::ItemIndexStore(data)) {}
ItemIndexStore::ItemIndexStore(interfaces::ItemIndexStore * base) : m_priv(base) {}

namespace detail {

ItemIndexStore::LZODecompressor::LZODecompressor() {}

ItemIndexStore::LZODecompressor::LZODecompressor(const sserialize::UByteArrayAdapter & data) :
m_data(data+1, data.getUint8(0))
{}

ItemIndexStore::LZODecompressor::~LZODecompressor() {}

OffsetType ItemIndexStore::LZODecompressor::getSizeInBytes() const {
	return SerializationInfo<uint8_t>::length + m_data.data().size();
}

UByteArrayAdapter ItemIndexStore::LZODecompressor::decompress(uint32_t id, const UByteArrayAdapter & src) const {
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

ItemIndexStore::ItemIndexStore() :
m_type(ItemIndex::T_EMPTY),
m_compression(sserialize::Static::ItemIndexStore::IC_NONE)
{}

ItemIndexStore::ItemIndexStore(UByteArrayAdapter data) :
m_version(data.getUint8(0)),
m_type((ItemIndex::Types) data.getUint8(1) ),
m_compression((IndexCompressionType) data.getUint8(2))
{
	data.resetGetPtr();
	data += 3;
// 	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION, m_version, "Static::ItemIndexStore");
	if (m_version != 3 && m_version != 4) {
		throw sserialize::VersionMissMatchException("sserialize::Static::ItemIndexStore", 4, m_version);
	}
	OffsetType dataLength = data.getOffset();
	data.shrinkToGetPtr();
	
	m_data = UByteArrayAdapter(data, 0, dataLength);
	data += dataLength;
	
	m_index = SortedOffsetIndex(data);
	data += m_index.getSizeInBytes();
	
	if (m_version == 4) {
		m_idxSizes = sserialize::Static::Array<uint32_t>(data);
		data += m_idxSizes.getSizeInBytes();
	}
	else if (m_version == 3) {
		std::cout << "sserialize::ItemIndexStore: version 3 detected, please upgrade to version 4" << std::endl;
	}
	
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		m_hd.reset(new HuffmanDecoder(data) );
		data += m_hd->getSizeInBytes();
	}
	
	if  (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		m_lzod.reset(new LZODecompressor(data));
	}
	
}


ItemIndexStore::~ItemIndexStore() {}

uint32_t ItemIndexStore::size() const {
	return m_index.size();
}

OffsetType ItemIndexStore::getSizeInBytes() const {
	OffsetType r = SerializationInfo<uint8_t>::length*3;
	r += UByteArrayAdapter::OffsetTypeSerializedLength();
	r += m_data.size();
	r += m_index.getSizeInBytes();
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		r += m_hd->getSizeInBytes();
	}
	if (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		r += m_lzod->getSizeInBytes();
	}
	return r;
}

UByteArrayAdapter::OffsetType ItemIndexStore::dataSize(uint32_t pos) const {
	if (pos >= size())
		return 0;
	UByteArrayAdapter::OffsetType indexStart = m_index.at(pos);
	UByteArrayAdapter::OffsetType indexLength;
	if (pos+1 == size()) {
		indexLength = m_data.size() - indexStart;
	}
	else {
		indexLength = m_index.at(pos+1) - indexStart;
	}
	return indexLength;
}

UByteArrayAdapter ItemIndexStore::rawDataAt(uint32_t pos) const {
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
	return UByteArrayAdapter::makeContigous(UByteArrayAdapter(m_data, indexStart, indexLength));
}

ItemIndex ItemIndexStore::at(uint32_t pos) const {
	if (pos >= size())
		return ItemIndex();
	
	UByteArrayAdapter idxData = rawDataAt(pos);
	if (m_compression & sserialize::Static::ItemIndexStore::IC_LZO) {
		idxData = m_lzod->decompress(pos, idxData);
	}
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		if (m_type == ItemIndex::T_WAH) {
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator(new UDWIteratorPrivateHD(MultiBitIterator(idxData), m_hd)));
		}
		else if (m_type == ItemIndex::T_RLE_DE) {
			return ItemIndex::createInstance<ItemIndexPrivateRleDE>(UDWIterator(new UDWIteratorPrivateHD(MultiBitIterator(idxData), m_hd)));
		}
	}
	else if (m_compression & sserialize::Static::ItemIndexStore::IC_VARUINT32) {
		if (m_type == ItemIndex::T_WAH) {
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator( new UDWIteratorPrivateVarDirect(idxData)));
		}
	}
	else { //no further compression
		return ItemIndex(idxData, m_type);
	}
	return ItemIndex();
}

UByteArrayAdapter ItemIndexStore::getHuffmanTreeData() const {
	if (m_compression & sserialize::Static::ItemIndexStore::IC_HUFFMAN) {
		UByteArrayAdapter data = getData();
		UByteArrayAdapter::OffsetType totalSize = data.size() + m_index.size();
		data.growStorage(m_index.getSizeInBytes() + m_hd->getSizeInBytes());
		data.incGetPtr(totalSize);
		data.shrinkToGetPtr();
		return data;
	}
	return UByteArrayAdapter();
}

std::ostream& ItemIndexStore::printStats(std::ostream& out) const {
	std::unordered_set<uint32_t> indexIds;
	for(uint32_t i = 0; i < size(); i++)
		indexIds.insert(i);
	return printStats(out, indexIds);
}

std::ostream& ItemIndexStore::printStats(std::ostream& out, const std::unordered_set<uint32_t> & indexIds) const {
	out << "Static::ItemIndexStore::Stats->BEGIN (info depends on selected indices)" << std::endl;
	out << "Storage size  of ItemIndexStore: " << getSizeInBytes() << std::endl;
	out << "size: " << indexIds.size() << std::endl;

	uint32_t largestIndex = 0;
	sserialize::UByteArrayAdapter::SizeType largestSpaceUsageIndex = 0;
	std::array<uint32_t, 32> sizeHisto;
	sizeHisto.fill(0);
	for(uint32_t i(0), s(size()); i < s; ++i) {
		sserialize::ItemIndex idx(at(i));
		sizeHisto.at( sserialize::msb(idx.size()) ) += 1;
		largestIndex = std::max<uint32_t>(largestIndex, idx.size());
		largestSpaceUsageIndex = std::max(largestSpaceUsageIndex, idx.getSizeInBytes());
	}
	out << "Largest index size: " << largestIndex << std::endl;
	out << "Largest space usage: " << largestSpaceUsageIndex << std::endl;
	out << "Size historgram [i>v]: ";
	for(uint32_t i(0); i < sizeHisto.size(); ++i) {
		if (sizeHisto.at(i)) {
			if (i<15) {
				out << (uint32_t(1) << (i+1));
			}
			else {
				out << "2^" << i+1;
			}
			out << ": " << double(100*sizeHisto.at(i))/size() << ", ";
		}
	}
	out << std::endl;

	uint64_t totalElementCount = 0;
	uint64_t sizeOfSelectedIndices = 0;
	long double meanBitsPerId = 0;
	for(std::unordered_set<uint32_t>::const_iterator idIt = indexIds.begin(); idIt != indexIds.end(); ++idIt) {
		uint32_t i = *idIt;
		ItemIndex idx(at(i));
		totalElementCount += idx.size();
		meanBitsPerId += idx.size()*idx.bpn();
		sizeOfSelectedIndices += idx.getSizeInBytes();
	}
	meanBitsPerId /= totalElementCount;
	
	out << "Total element count (sum idx.size()):" << totalElementCount << std::endl;
	out << "Size of selected indices: " << sizeOfSelectedIndices << std::endl;
	out << "Mean bits per id: " << meanBitsPerId << std::endl;
	out << "Mean bits per id (headers included): " << (long double)sizeOfSelectedIndices/totalElementCount*8 << std::endl;

	uint64_t wordAlignedBitSetSize = 0;
	std::unordered_set<uint32_t> wabSet;
	std::unordered_map<uint32_t, uint32_t> idFreqs;
	for(std::unordered_set<uint32_t>::const_iterator idIt = indexIds.begin(); idIt != indexIds.end(); ++idIt) {
		uint32_t i = *idIt;
		ItemIndex idx(at(i));
		for(uint32_t j(0), s(idx.size()); j < s; j++)  {
			uint32_t id = idx.at(j);
			
			wabSet.insert(id/8);
			
			if (idFreqs.count(id) == 0) {
				idFreqs[id] = 0;
			}
			idFreqs[id] += 1;
		}
		wordAlignedBitSetSize += wabSet.size();
		wabSet.clear();
	}
	out << "Minimum storage need for word aligned bit set (compression-ratio 100%): " << wordAlignedBitSetSize << std::endl;
	
	long double idEntropy = 0;
	long double idDiscreteEntropy = 0;
	for(std::unordered_map<uint32_t, uint32_t>::iterator it = idFreqs.begin(); it != idFreqs.end(); ++it) {
		long double wn = (double)it->second/totalElementCount;
		long double log2res = logTo2(wn);
		idEntropy += wn * log2res;
		idDiscreteEntropy += wn * ceil( - log2res );
	}
	idEntropy = - idEntropy;
	out << "Id entropy: " << idEntropy << std::endl;
	out << "Id discrete entropy: " << idDiscreteEntropy << std::endl;
	{ //index specific stats
		if (indexType() == ItemIndex::T_PFOR) {
			std::array<uint32_t, ItemIndexPrivatePFoR::BlockSizes.size()> bsOcc;
			std::map<uint32_t, uint32_t> bcOcc;
			bsOcc.fill(0);
			for(uint32_t i(0), s(size()); i < s; ++i) {
				sserialize::ItemIndexPrivatePFoR idx( rawDataAt(i) );
				bsOcc.at(idx.blockSizeOffset()) += 1;
			}
			out << "ItemIndexPFoR::BlockSize histogram: ";
			for(uint32_t i(0); i < bsOcc.size(); ++i) {
				if (bsOcc.at(i)) {
					out << ItemIndexPrivatePFoR::BlockSizes.at(i) << ": " << double(100*bsOcc.at(i))/size() << ", ";
				}
			}
		}
		out << std::endl;
	}
	
	out << "Static::ItemIndexStore::Stats->END" << std::endl;
	return out;
}


}}}//end namespace