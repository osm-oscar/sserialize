#include <sserialize/Static/Fraction.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace Static {

Fraction::Fraction(const sserialize::Fraction & f) :
sserialize::Fraction(f)
{}

Fraction::Fraction(const Fraction & f) :
sserialize::Fraction(f)
{}

Fraction::Fraction() {}

Fraction::Fraction(int64_t numerator, uint64_t denominator) :
sserialize::Fraction(numerator, denominator)
{}

Fraction::Fraction(const UByteArrayAdapter& src) {
	int len = 0;
	numerator() = src.getVlPackedUint64(0, &len);
	if (len < 0) {
		throw sserialize::CorruptDataException("Fraction: Could not parse numerator from data stream");
	}
	denominator() = src.getVlPackedUint64((UByteArrayAdapter::SizeType) len, &len);
	if (len < 0) {
		throw sserialize::CorruptDataException("Fraction: Could not parse denominator from data stream");
	}
}

Fraction::~Fraction() {}

}} //end namespace sserialize::Static