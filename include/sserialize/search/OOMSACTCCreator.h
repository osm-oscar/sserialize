#ifndef SSERIALIZE_OOM_SA_CTC_CREATOR_H
#define SSERIALIZE_OOM_SA_CTC_CREATOR_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/search/OOMCTCValueStore.h>

namespace sserialize {
namespace detail {
namespace OOMSACTCCreator {

class Creator {
public:
	Creator();
private:
	sserialize::HashBasedFlatTrie<uint32_t> m_t;
	
};

}}//end namespace detail::OOMSACTCCreator

template<typename TInputIterator, typename TTraits>
void createSACTC(TInputIterator begin, TInputIterator end, sserialize::UByteArrayAdapter & dest,
TTraits traits = TTraits(), uint64_t maxMemoryUsage = static_cast<uint64_t>(1) << 32) {
	detail::OOMSACTCCreator::Creator creator;
	
}

}//end namespace

#endif