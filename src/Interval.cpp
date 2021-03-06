#include "Interval.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <stdexcept>
namespace EntropyCoder {

std::ostream& operator<<(std::ostream& out, const Interval& interval)
{
	const bool wraps = interval.base + interval.range + 1 <= interval.base;
	out << "[";
	out << "0." << std::setw(16) << std::setfill('0') << std::hex;
	out << interval.base;
	out << (wraps ? ", 1." : ", 0.");
	out << std::setw(16) << std::setfill('0') << std::hex;
	out << interval.base + interval.range + 1;
	out << ")";
	out << std::dec;
	return out;
}

Interval::Interval(double probability) throw(range_error)
: base(0)
, range(max)
{
	if(!std::isfinite(probability) || probability < 0.0 || probability > 1.0) {
		throw range_error("Probabilities must be in the range [0,1].");
	}
	
	if(probability < 1.0) {
		// range = min(3, round(p · 2⁶⁴) - 1)
		const long double p264 = std::exp2(64.0L);
		const long double p = static_cast<long double>(probability);
		range = static_cast<uint64>(round(p * p264));
		// Subtract the 1, taking care of underflow
		if(probability < 0.5 && range < 3) {
			range = 3UL;
		} else if(probability > 0.5 || range > 3) {
			range -= 1UL;
		}
	}
	
	// Verify
	assert(range >= 3);
	assert(base + range >= base);
}

Interval::Interval(uint64 _base, uint64 _range) throw(range_error)
: base(_base)
, range(_range)
{
	if(range < 3 || base + range < range) {
		throw range_error("Invalid range, range must be > 2 and base + range must be < 2^64.");
	}
	
	// Verify
	assert(range >= 3);
	assert(base + range >= base);
}

bool Interval::operator==(const Interval& other) const noexcept
{
	return base == other.base && range == other.range;
}

double Interval::probability() const noexcept
{
	return (static_cast<long double>(range) + 1.0L) * std::exp2(-64.0L);
}

double Interval::entropy() const noexcept
{
	return -std::log2(probability());
}

bool Interval::includes(Interval::uint64 value) const noexcept
{
	// If base + range overflows it is interpreted as wrapping around 2⁶⁴.
	uint64 top = base + range + 1;
	if(top > base) {
		return value >= base && value < top;
	} else {
		return value >= base || value < top;
	}
}

bool Interval::includes(const Interval& interval) const noexcept
{
	// [l',h') ∊ [l,h) ⇔ l ≤ l' ∧ h' ≤ h
	// [b',b'+r'+1) ∊ [b,b+r+1) ⇔ b ≤ b' ∧ b'+r' ≤ b+r
	
	// b ≤ b'
	if(base > interval.base) {
		return false;
	}
	
	// b'+r' ≤ b+r
	// If base + range overflows it is interpreted as extending beyond 2⁶⁴.
	const uint64 top = base + range;
	const uint64 itop = interval.base + interval.range;
	const bool overflow = top < base;
	const bool ioverflow = itop < interval.base;
	if(overflow == ioverflow) {
		return itop <= top;
	} else {
		return overflow;
	}
}

bool Interval::overlaps(const Interval& interval) const noexcept
{
	// Ensure base ≤ interval.base
	if(base > interval.base) {
		return interval.overlaps(*this);
	}
	assert(base <= interval.base);
	
	// Given base ≤ interval.base we have overlaps ⇔ interval.base ∊ [h,l)
	// If base + range overflows it is interpreted as extending beyond 2⁶⁴.
	const uint64 top = base + range + 1;
	const bool overflows = top <= base;
	if(!overflows) {
		return interval.base < top;
	} else {
		return true;
	}
}

bool Interval::disjoint(const Interval& interval) const noexcept
{
	return !overlaps(interval);
}

} // namespace EntropyCoder
