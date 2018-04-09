#ifndef SSERIALIZE_OOM_FLAT_TRIE_H
#define SSERIALIZE_OOM_FLAT_TRIE_H
#include <sserialize/utility/constants.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/containers/OOMArray.h>


namespace SSERIALIZE_NAMESPACE {
	
namespace detail {
namespace OOMFlatTrie {
	
}} //end namespace detail::OOMFlatTrie
	
class OOMFlatTrie {
public:
	OOMFlatTrie();
public:
	void finalize();
	void serialize(sserialize::UByteArrayAdapter & dest);
};
	
} //end namespace

#endif
