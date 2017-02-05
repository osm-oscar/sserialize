#ifndef SSERIALIZE_STATIC_FRACTION_H
#define SSERIALIZE_STATIC_FRACTION_H
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/utility/Fraction.h>

namespace sserialize {
namespace Static {

/** This is a small wrapper class to represent fractions with up to 64 bits in the numerator/denominator
  * File format is as follow:
  * vs64|vu64 for the default ctor
  * s8|u8[tnum-1]|u8[tdenom] where tnum and tdenom define the number of bytes for the numerator and denominator
  *
  *
  */

class Fraction: public sserialize::Fraction {
public:
	Fraction();
	Fraction(int64_t numerator, uint64_t denominator);
	Fraction(const sserialize::Fraction & f);
	Fraction(const Fraction & f);
	Fraction(const sserialize::UByteArrayAdapter & src);
	template<uint8_t T_DENOMINATOR_BYTES, uint8_t T_NUMERATOR_BYTES = T_DENOMINATOR_BYTES>
	Fraction(const sserialize::UByteArrayAdapter & src);
	virtual ~Fraction();
private:
	
};


sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Fraction & src);
sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & dest, Fraction & src);


}}//end namespace sserialize::Static

//implementation

namespace sserialize {
namespace Static {


}} //end namespae sserialize::Static

#endif