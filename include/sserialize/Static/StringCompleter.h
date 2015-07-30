#ifndef SSERIALIZE_STATIC_STRING_COMPLETER_H
#define SSERIALIZE_STATIC_STRING_COMPLETER_H
#include <deque>
#include <sserialize/search/StringCompleterPrivate.h>
#include <sserialize/Static/ItemIndexStore.h>

#define SSERIALIZE_STATIC_STRING_COMPLETER_HEADER_SIZE 7
#define SSERIALIZE_STATIC_STRING_COMPLETER_HEADER_VERSION 1


/*
 * Completer Layout
 * ----------------------------------------------------------------
 * vvvvvvvv|tttttttt|datasize                      |CompleterData
 * ----------------------------------------------------------------
 *  1 byte | 1 byte |UBA::OffsetTypeSerializedLength|multiple bytes
 * 
 * v = version number as uint8
 * t = completer type
 */

namespace sserialize {
namespace Static {

class StringCompleterPrivate: public sserialize::RefCountObject {
public:
	typedef sserialize::StringCompleterPrivate::ForwardIterator ForwardIterator;
public:
	StringCompleterPrivate() {}
	virtual ~StringCompleterPrivate() {}
	virtual OffsetType getSizeInBytes() const = 0;
	virtual ItemIndex complete(const std::string & str, StringCompleter::QuerryType qtype) const = 0;
	
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	/** @return returns pairs of char->ItemIndex **/
	virtual std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const;

	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const = 0;

	virtual ItemIndex indexFromId(uint32_t idxId) const = 0;

	virtual std::ostream& printStats(std::ostream& out) const = 0;
	
	virtual std::string getName() const = 0;
	
	virtual sserialize::StringCompleterPrivate::ForwardIterator * forwardIterator() const = 0;
};

class StringCompleter: public sserialize::StringCompleterPrivate {
public:
	enum Type {
		T_NONE=0, T_TRIE=1, T_RECTRIE=2, T_FLAT_TRIE=3, T_FLAT_TRIE_STRIDS=4, T_TRIE_INDIRECT_INDEX=5
	};
	struct HeaderInfo {
		HeaderInfo(uint32_t type, OffsetType dataSize) : type(type), dataSize(dataSize) {}
		uint32_t type;
		OffsetType dataSize;
	};
private:
	Type m_t;
	sserialize::RCPtrWrapper<Static::StringCompleterPrivate> m_priv;
public:
	StringCompleter();
	StringCompleter(sserialize::UByteArrayAdapter data, const sserialize::Static::ItemIndexStore & indexStore);
	virtual ~StringCompleter();
	
	inline const sserialize::RCPtrWrapper<Static::StringCompleterPrivate> & priv() const { return m_priv; }
	inline sserialize::RCPtrWrapper<Static::StringCompleterPrivate> & priv() { return m_priv; }
	
	virtual OffsetType getSizeInBytes() const;
	
	virtual ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	/** @return returns pairs of char->ItemIndex **/
	virtual std::map<uint16_t, ItemIndex> getNextCharacters(const std::string& str, sserialize::StringCompleter::QuerryType qtype, bool withIndex) const;

	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual ItemIndex indexFromId(uint32_t idxId) const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
	virtual ForwardIterator * forwardIterator() const;

};

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & d, const StringCompleter::HeaderInfo & src) {
	d.putUint8(SSERIALIZE_STATIC_STRING_COMPLETER_HEADER_VERSION);
	d.putUint32(src.type);
	d.putOffset(src.dataSize);
	return d;
}

}

template<>
struct SerializationInfo< sserialize::Static::StringCompleter::HeaderInfo > {
	static const bool is_fixed_length = true;
	static const OffsetType length = SerializationInfo<uint8_t>::length + SerializationInfo<uint32_t>::length + sserialize::UByteArrayAdapter::S_OffsetTypeSerializedLength;
	static const OffsetType max_length = length;
	static const OffsetType min_length = length;
	static OffsetType sizeInBytes(const sserialize::Static::StringCompleter::HeaderInfo & /*value*/) {
		return length;
	}
};

}//end namespace

#endif