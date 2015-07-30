#include <sserialize/Static/StringCompleter.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>

namespace sserialize {
namespace Static {

ItemIndexIterator StringCompleterPrivate::partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	return ItemIndexIterator( complete(str, qtype) );
}

std::map<uint16_t, ItemIndex> StringCompleterPrivate::getNextCharacters(const std::string& /*str*/, sserialize::StringCompleter::QuerryType /*qtype*/, bool /*withIndex*/) const {
	return std::map<uint16_t, ItemIndex>();
}

StringCompleter::StringCompleter() {}

StringCompleter::StringCompleter(UByteArrayAdapter data, const sserialize::Static::ItemIndexStore & indexStore) {
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_STRING_COMPLETER_HEADER_VERSION, data.getUint8(), "sserialize::Static::StringCompleter");
	m_t = (Type) data.getUint32();
	UByteArrayAdapter::OffsetType dataSize = data.getOffset();
	data.shrinkToGetPtr();
	SSERIALIZE_LENGTH_CHECK(dataSize, data.size(), "sserialize::Static::StringCompleter");
	data = UByteArrayAdapter(data, 0, dataSize);
	
	switch (m_t) {
	case (Static::StringCompleter::T_TRIE):
		m_priv.reset(new Static::GeneralizedTrie(data, indexStore));
		break;
	case (Static::StringCompleter::T_FLAT_TRIE):
		m_priv.reset(new Static::FlatGST (data, indexStore));
		break;
	default:
		throw sserialize::CorruptDataException("sserialize::Static::StringCompleter: Unknown data");
	}
}

StringCompleter::~StringCompleter() {}

OffsetType StringCompleter::getSizeInBytes() const {
	return SerializationInfo< sserialize::Static::StringCompleter::HeaderInfo >::length + priv()->getSizeInBytes();
}

ItemIndex StringCompleter::complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	return priv()->complete(str, qtype);
}

ItemIndexIterator StringCompleter::partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const {
	return priv()->partialComplete(str, qtype);
}

std::map<uint16_t, ItemIndex> StringCompleter::getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const {
	return priv()->getNextCharacters(str, qtype, withIndex);
}

sserialize::StringCompleter::SupportedQuerries StringCompleter::getSupportedQuerries() const {
	return priv()->getSupportedQuerries();
}

ItemIndex StringCompleter::indexFromId(uint32_t idxId) const {
	return priv()->indexFromId(idxId);
}

std::ostream& StringCompleter::printStats(std::ostream& out) const {
	return priv()->printStats(out);
}

std::string StringCompleter::getName() const {
	return priv()->getName();
}

StringCompleter::ForwardIterator * StringCompleter::forwardIterator() const {
	return priv()->forwardIterator();
}

}}