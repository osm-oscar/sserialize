#ifndef SSERIALIZE_ITERATOR_H
#define SSERIALIZE_ITERATOR_H
#include <iterator>

namespace sserialize {

template<typename TCategory, typename TValueType, typename TDistanceType = std::ptrdiff_t>
struct StaticIterator: public std::iterator<TCategory, TValueType, TDistanceType, TValueType*, TValueType> {

};


}//end namespace


#endif