#ifndef SSERIALIZE_STATIC_STRING_COMPLETER_H
#define SSERIALIZE_STATIC_STRING_COMPLETER_H
#include <deque>
#include <sserialize/completers/StringCompleterPrivate.h>

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
};

}}//end namespace

#endif