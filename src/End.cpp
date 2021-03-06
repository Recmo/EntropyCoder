#include "End.h"
#include "CodeInterval.h"
#include <stdexcept>
namespace EntropyCoder {

End::End(End::State _state) throw(invalid_state)
: state{_state}
{
	if(state == sS) {
		ending = 0;
	} else if(state == s0) {
		ending = 0;
	} else if(state == s1) {
		ending = msb;
	} else {
		throw invalid_state("Invalid state.");
	}
}

End::End(End::State _state, uint64_t _ending) throw(invalid_state)
: ending{_ending}
, state{_state}
{
	if(state == sS && ending >= msb) {
		throw invalid_state("Invalid state.");
	}
}

std::ostream& operator<<(std::ostream& out, const End& end)
{
	switch(end.state) {
		case End::sS: out << "S:"; break;
		case End::s0: out << "0:"; break;
		case End::s1: out << "1:"; break;
	}
	std::uint64_t bits = end.ending;
	out << (bits >= End::msb ? "1." : "0.");
	bits <<= 1;
	while(bits != 0) {
		out << (bits >= End::msb ? "1" : "0");
		bits <<= 1;
	}
	return out;
}

bool End::operator==(const End& other) const noexcept
{
	return state == other.state && ending == other.ending;
}

void End::first(const CodeInterval& interval) throw(carry_inconsistent, overflow_error)
{
	if(state == sS && interval.wraps()) {
		throw carry_inconsistent("End is in carry-free state, but interval allows carry.");
	}
	while(!is_valid(interval)) {
		next();
	}
}

void End::next() throw(overflow_error)
{
	if(state == sS && ending == 0) {
		ending = msb >> 1;
	} else if(state == s0 && ending == 0) {
		ending = msb;
	} else if(state == s1 && ending == msb) {
		ending = 0;
	} else if(state == s1 && ending == 0) {
		ending = msb >> 1;
	} else {
		const std::uint64_t last_bit = ending & ~(ending - 1);
		const std::uint64_t max = last_bit | (state == sS ? msb : 0);
		ending += last_bit << 1;
		if(ending == max) {
			if(last_bit == 1) {
				throw overflow_error("Overflow.");
			}
			ending = last_bit >> 1;
		}
	}
}

void End::carry() throw(invalid_carry)
{
	const std::uint64_t last_bit = ending & ~(ending - 1);
	if(state == sS) {
		throw invalid_carry("Can not add carry in state S.");
	} else if(ending == 0) {
		ending = state == s1 ? msb >> 1 : 0;
	} else if(ending >= msb) {
		ending ^= msb;
	} else {
		ending = last_bit;
	}
	state = sS;
}

void End::zero() noexcept
{
	const std::uint64_t last_bit = ending & ~(ending - 1);
	if(state == s1 && ending == msb) {
		ending = 0;
	} else if (ending >= msb) {
		ending = last_bit;
	} else {
		ending <<= 1;
	}
	state = s0;
}

void End::one() noexcept
{
	const std::uint64_t last_bit = ending & ~(ending - 1);
	if(ending == 0) {
		ending = state == s0 ? msb : 0;
	} else if(ending < (msb >> 1)) {
		ending = last_bit << 1;
	} else if(ending < (msb | msb >> 1)) {
		ending <<= 1;
		ending ^= msb;
	} else {
		ending = last_bit;
	}
	state = state == sS ? sS : s1;
}

bool End::is_valid(const CodeInterval& interval) noexcept
{
	const bool carry = ending >= msb;
	const std::uint64_t value = ending << 1;
	const bool value_valid = interval.includes(value);
	const bool carry_valid = !carry || interval.wraps();
	const bool wrap_valid = carry == (value < interval.base);
	return value_valid && carry_valid && wrap_valid;
}

} // namespace EntropyCoder
