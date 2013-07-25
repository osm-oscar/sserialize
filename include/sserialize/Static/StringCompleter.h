#ifndef SSERIALIZE_STATIC_STRING_COMPLETER_H
#define SSERIALIZE_STATIC_STRING_COMPLETER_H
#include <deque>
#include <sserialize/completers/StringCompleterPrivate.h>
#include <sserialize/Static/GeneralizedTrie.h>
#include <sserialize/Static/FlatGeneralizedTrie.h>
#include <sserialize/Static/FlatGSTStrIds.h>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/completers/StringCompleterPrivateStaticDB.h>
#include <sserialize/Static/ItemIndexStore.h>

#define STATIC_STRING_COMPLETER_HEADER_SIZE 2


/*
 * Completer Layout
 * -------------------------------
 * vvvvvvvv|tttttttt|CompleterData
 * -------------------------------
 *  1 byte | 1 byte |multiple bytes
 * 
 * v = version number as uint8
 * t = completer type
 */

namespace sserialize {
namespace Static {

struct StringCompleter {
	enum Type {
		T_NONE=0, T_TRIE, T_RECTRIE, T_FLAT_TRIE, T_FLAT_TRIE_STRIDS, T_TRIE_INDIRECT_INDEX
	};
	inline static bool addHeader(uint8_t type, std::deque<uint8_t> & destination) {
		destination.push_back(0);
		destination.push_back(type);
		return true;
	}
	
	template<typename DataBaseType>
	static sserialize::StringCompleter fromDB(const DataBaseType & db) {
		return sserialize::StringCompleter(new StringCompleterPrivateStaticDB<DataBaseType>(db));
	}
	
	template<typename DataBaseType>
	static sserialize::StringCompleter fromData(const UByteArrayAdapter & data, const sserialize::Static::ItemIndexStore & indexStore, const DataBaseType & db) {
		uint8_t type = data[1];
		switch (type) {
		case (Static::StringCompleter::T_TRIE):
			return sserialize::StringCompleter(new Static::GeneralizedTrie(data+2, indexStore));
		case (Static::StringCompleter::T_FLAT_TRIE):
			return sserialize::StringCompleter(new Static::FlatGST (data+2, indexStore));
		case (Static::StringCompleter::T_FLAT_TRIE_STRIDS):
			return sserialize::StringCompleter(new sserialize::Static::FlatGSTStrIds<DataBaseType>( data+2, indexStore, db) );
		default:
			return sserialize::StringCompleter();
		}
	}
};

}}//end namespace

#endif