#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/utility/utilfuncs.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/containers/UDWIterator.h>
#include <sserialize/containers/UDWIteratorPrivateHD.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h>

namespace sserialize {
namespace Static {

ItemIndexStore::ItemIndexStore() :
m_type(ItemIndex::T_EMPTY),
m_compression(IC_NONE)
{}

ItemIndexStore::ItemIndexStore(UByteArrayAdapter data) :
m_version(data.at(0)),
m_type((ItemIndex::Types) data.at(1) ),
m_compression(IC_NONE)
{
	if (m_version == 1) {
		data += 2;
	}
	else if (m_version == 2) {
		m_compression = (IndexCompressionType) data.at(2);
		data += 3;
	}
	else {
		SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_ITEM_INDEX_STORE_VERSION, m_version, "Static::ItemIndexStore");
	}
	
	OffsetType off = data.getOffset();
	data.shrinkToGetPtr();
	
	m_data = UByteArrayAdapter(data, 0, off);
	m_index = SortedOffsetIndex(data + off);
	off += m_index.getSizeInBytes();
	if (m_version > 2)
		m_hd = HuffmanDecoder(data +  off);
}


ItemIndexStore::~ItemIndexStore() {}

uint32_t ItemIndexStore::size() const {
	return m_index.size();
}

uint32_t ItemIndexStore::getSizeInBytes() const {
	return 2 + (m_version == 2 ? 1 : 0) + UByteArrayAdapter::OffsetTypeSerializedLength() + m_index.getSizeInBytes() + m_data.size();
}

UByteArrayAdapter ItemIndexStore::dataAt(uint32_t pos) const {
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
	return UByteArrayAdapter(m_data, indexStart, indexLength);
}

ItemIndex ItemIndexStore::at(uint32_t pos) const {
	if (pos >= size())
		return ItemIndex();
	if (m_type == ItemIndex::T_WAH) {
		switch (m_compression) {
		case IC_NONE:
			return ItemIndex(dataAt(pos), m_type);
		case IC_VARUINT32:
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator( new UDWIteratorPrivateVarDirect(dataAt(pos))));
		case IC_HUFFMAN:
			return ItemIndex::createInstance<ItemIndexPrivateWAH>(UDWIterator(new UDWIteratorPrivateHD(MultiBitIterator(dataAt(pos)), m_hd)));
		case IC_ILLEGAL:
			return ItemIndex();
		};
	}
	else {
		return ItemIndex(dataAt(pos), m_type);
	}
	return ItemIndex();
}

ItemIndex ItemIndexStore::at(uint32_t pos, const ItemIndex& realIdIndex) const {
	if (pos >= size())
		return ItemIndex();
	return ItemIndex(dataAt(pos), realIdIndex, ItemIndex::T_REGLINE);
}

ItemIndex ItemIndexStore::hierachy(const std::deque< uint32_t >& offsets) const {
	if (offsets.size() == 0)
		return ItemIndex();
	ItemIndex idx(dataAt(offsets.front()), indexType());
	for(size_t i = 1; i < offsets.size(); i++) {
		idx = ItemIndex(dataAt(offsets.at(i)), idx, indexType());
	}
	return idx;
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

	std::unordered_set<uint32_t> wabSet;
	uint64_t sizeOfSelectedIndices = 0;
	uint64_t totalElementCount = 0;
	uint64_t wordAlignedBitSetSize = 0;
	std::unordered_map<uint32_t, uint32_t> idFreqs;
	long double meanBitsPerId = 0;
	for(std::unordered_set<uint32_t>::const_iterator idIt = indexIds.begin(); idIt != indexIds.end(); ++idIt) {
		size_t i = *idIt;
		ItemIndex idx(at(i));
		for(size_t j = 0; j < idx.size(); j++)  {
			uint32_t id = idx.at(j);
			
			wabSet.insert(id/8);
			
			if (idFreqs.count(id) == 0)
				idFreqs[id] = 0;
			idFreqs[id] += 1;
		}
		totalElementCount += idx.size();
		meanBitsPerId += idx.size()*idx.bpn();
		sizeOfSelectedIndices += idx.getSizeInBytes();
		wordAlignedBitSetSize += wabSet.size();
		wabSet.clear();
	}
	meanBitsPerId /= totalElementCount;
	
	
	long double idEntropy = 0;
	long double idDiscreteEntropy = 0;
	for(std::unordered_map<uint32_t, uint32_t>::iterator it = idFreqs.begin(); it != idFreqs.end(); ++it) {
		long double wn = (double)it->second/totalElementCount;
		long double log2res = logTo2(wn);
		idEntropy += wn * log2res;
		idDiscreteEntropy += wn * ceil( - log2res );
	}
	idEntropy = - idEntropy;
	out << "Size of selected indices: " << sizeOfSelectedIndices << std::endl;
	out << "Id entropy: " << idEntropy << std::endl;
	out << "Id discrete entropy: " << idDiscreteEntropy << std::endl;
	out << "Mean bits per id: " << meanBitsPerId << std::endl;
	out << "Mean bits per id (headers included): " << (long double)sizeOfSelectedIndices/totalElementCount*8 << std::endl;
	out << "Minimum storage need for word aligned bit set (compression-ratio 100%): " << wordAlignedBitSetSize << std::endl;
	out << "Static::ItemIndexStore::Stats->END" << std::endl;
	return out;
}


}}//end namespace