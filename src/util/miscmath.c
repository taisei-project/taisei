/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "miscmath.h"

double lerp(double v0, double v1, double f) {
	return f * (v1 - v0) + v0;
}

float lerpf(float v0, float v1, float f) {
	return f * (v1 - v0) + v0;
}

cmplx clerp(cmplx v0, cmplx v1, double f) {
	return f * (v1 - v0) + v0;
}

cmplxf clerpf(cmplxf v0, cmplxf v1, float f) {
	return f * (v1 - v0) + v0;
}

double approach(double v, double t, double d) {
	if(v < t) {
		v += d;
		if(v > t)
			return t;
	} else if(v > t) {
		v -= d;
		if(v < t)
			return t;
	}

	return v;
}

float fapproach(float v, float t, float d) {
	if(v < t) {
		v += d;
		if(v > t)
			return t;
	} else if(v > t) {
		v -= d;
		if(v < t)
			return t;
	}

	return v;
}

double approach_p(double *v, double t, double d) {
	return *v = approach(*v, t, d);
}

float fapproach_p(float *v, float t, float d) {
	return *v = fapproach(*v, t, d);
}

double approach_asymptotic(double val, double target, double rate, double epsilon) {
	if(fabs(val - target) < epsilon || UNLIKELY(rate >= 1.0)) {
		return target;
	}

	return val + (target - val) * rate;
}

float fapproach_asymptotic(float val, float target, float rate, float epsilon) {
	if(fabsf(val - target) < epsilon || UNLIKELY(rate >= 1.0f)) {
		return target;
	}

	return val + (target - val) * rate;
}

cmplx capproach_asymptotic(cmplx val, cmplx target, double rate, double epsilon) {
	if(cabs(val - target) < epsilon || UNLIKELY(rate >= 1.0)) {
		return target;
	}

	return val + (target - val) * rate;
}

double approach_asymptotic_p(double *val, double target, double rate, double epsilon) {
	return *val = approach_asymptotic(*val, target, rate, epsilon);
}

float fapproach_asymptotic_p(float *val, float target, float rate, float epsilon) {
	return *val = fapproach_asymptotic(*val, target, rate, epsilon);
}

cmplx capproach_asymptotic_p(cmplx *val, cmplx target, double rate, double epsilon) {
	return *val = capproach_asymptotic(*val, target, rate, epsilon);
}

cmplx cnormalize(cmplx c) {
	cmplx r = c / sqrt(re(c) * re(c) + im(c) * im(c));
	// NOTE: clang generates a function call for isnan()...
	return LIKELY(r == r) ? r : 0;
}

cmplx cclampabs(cmplx c, double maxabs) {
	double a = cabs(c);

	if(a > maxabs) {
		return maxabs * c / a;
	}

	return c;
}

cmplx cwclamp(cmplx c, cmplx cmin, cmplx cmax) {
	return CMPLX(
		clamp(re(c), re(cmin), re(cmax)),
		clamp(im(c), im(cmin), im(cmax))
	);
}

cmplx cdir(double angle) {
	// this is faster than cexp(I*angle)

	#ifdef TAISEI_BUILDCONF_HAVE_SINCOS
	double s, c;
	sincos(angle, &s, &c);
	return CMPLX(c, s);
	#else
	return CMPLX(cos(angle), sin(angle));
	#endif
}

cmplxf cdirf(float angle) {
	// this is faster than cexp(I*angle)

	#ifdef TAISEI_BUILDCONF_HAVE_SINCOSF
	float s, c;
	sincosf(angle, &s, &c);
	return CMPLXF(c, s);
	#else
	return CMPLXF(cosf(angle), sinf(angle));
	#endif
}

cmplx cwmul(cmplx c0, cmplx c1) {
	return CMPLX(re(c0) * re(c1), im(c0) * im(c1));
}

cmplxf cwmulf(cmplxf c0, cmplxf c1) {
	return CMPLXF(re(c0) * re(c1), im(c0) * im(c1));
}

cmplx cwdiv(cmplx c0, cmplx c1) {
	return CMPLX(re(c0) / re(c1), im(c0) / im(c1));
}

cmplxf cwdivf(cmplxf c0, cmplxf c1) {
	return CMPLXF(re(c0) / re(c1), im(c0) / im(c1));
}

cmplx cswap(cmplx c) {
	return CMPLX(im(c), re(c));
}

cmplxf cswapf(cmplxf c) {
	return CMPLXF(im(c), re(c));
}

double ccross(cmplx a, cmplx b) {
	return re(a) * im(b) - im(a) * re(b);
}

float ccrossf(cmplxf a, cmplxf b) {
	return re(a) * im(b) - im(a) * re(b);
}

cmplx csort(cmplx z) {
	double a = re(z);
	double b = im(z);
	return b > a ? CMPLX(a, b) : CMPLX(b, a);
}

cmplxf csortf(cmplxf z) {
	float a = re(z);
	float b = im(z);
	return b > a ? CMPLXF(a, b) : CMPLXF(b, a);
}

double psin(double x) {
	return 0.5 + 0.5 * sin(x);
}

double pcos(double x) {
	return 0.5 + 0.5 * cos(x);
}

float psinf(float x) {
	return 0.5 + 0.5 * sinf(x);
}

float pcosf(float x) {
	return 0.5 + 0.5 * cosf(x);
}

intmax_t imin(intmax_t a, intmax_t b) {
	return (a < b) ? a : b;
}

intmax_t imax(intmax_t a, intmax_t b) {
	return (a > b) ? a : b;
}

uintmax_t umin(uintmax_t a, uintmax_t b) {
	return (a < b) ? a : b;
}

uintmax_t umax(uintmax_t a, uintmax_t b) {
	return (a > b) ? a : b;
}

float clampf(float f, float lower, float upper) {
	assert(lower <= upper);

	if(f < lower) {
		return lower;
	}

	if(f > upper) {
		return upper;
	}

	return f;
}

intmax_t iclamp(intmax_t f, intmax_t lower, intmax_t upper) {
	assert(lower <= upper);

	if(f < lower) {
		return lower;
	}

	if(f > upper) {
		return upper;
	}

	return f;
}

double smoothstep(double edge0, double edge1, double x) {
	x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return x * x * (3 - 2 * x);
}

double smoothmin(double a, double b, double k) {
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return lerp(b, a, h) - k * h * (1.0 - h);
}

int sign(double x) {
	return (x > 0) - (x < 0);
}

double swing(double x, double s) {
	if(x <= 0.5) {
		x *= 2;
		return x * x * ((s + 1) * x - s) / 2;
	}

	x--;
	x *= 2;

	return x * x * ((s + 1) * x + s) / 2 + 1;
}

double sawtooth(double x) {
	return 2 * (x - floor(x + 0.5));
}

double triangle(double x) {
	return 2 * fabs(sawtooth(x)) - 1;
}

uint32_t topow2_u32(uint32_t x) {
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

uint64_t topow2_u64(uint64_t x) {
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return x + 1;
}

float smooth(float x) {
	return 1.0 - (0.5 * cos(M_PI * x) + 0.5);
}

double circle_angle(double index, double max_elements) {
	return (index * (M_PI * 2.0)) / max_elements;
}

cmplx circle_dir(double index, double max_elements) {
	return cdir(circle_angle(index, max_elements));
}

cmplx circle_dir_ofs(double index, double max_elements, double ofs) {
	return cdir(circle_angle(index, max_elements) + ofs);
}

static const uint64_t upow10_table[] = {
	1ull,
	10ull,
	100ull,
	1000ull,
	10000ull,
	100000ull,
	1000000ull,
	10000000ull,
	100000000ull,
	1000000000ull,
	10000000000ull,
	100000000000ull,
	1000000000000ull,
	10000000000000ull,
	100000000000000ull,
	1000000000000000ull,
	10000000000000000ull,
	100000000000000000ull,
	1000000000000000000ull,
	10000000000000000000ull,
};

uint64_t upow10(uint n) {
	assert(n < sizeof(upow10_table)/sizeof(*upow10_table));
	return upow10_table[n];
}

uint digitcnt(uint64_t x) {
	if(x == 0) {
		return 1;
	}

	uint low = 0;
	uint high = sizeof(upow10_table)/sizeof(*upow10_table) - 1;

	for(;;) {
		uint mid = (low + high) / 2;

		if(x >= upow10_table[mid]) {
			if(low == mid) {
				return mid + 1;
			}

			low = mid;
		} else {
			high = mid;
		}
	}

	UNREACHABLE;
}

typedef struct int128_bits {
	uint64_t hi;
	uint64_t lo;
} int128_bits_t;

INLINE attr_unused
void udiv_128_64(int128_bits_t divident, uint64_t divisor, uint64_t *out_quotient) {
	/*
	if(!divident.hi) {
		*out_quotient = divident.lo / divisor;
		return;
	}
	*/

	uint64_t quotient = divident.lo << 1;
	uint64_t remainder = divident.hi;
	uint64_t carry = divident.lo >> 63;
	uint64_t temp_carry = 0;

	for(int i = 0; i < 64; i++) {
		temp_carry = remainder >> 63;
		remainder <<= 1;
		remainder |= carry;
		carry = temp_carry;

		if(carry == 0) {
			if(remainder >= divisor) {
				carry = 1;
			} else {
				temp_carry = quotient >> 63;
				quotient <<= 1;
				quotient |= carry;
				carry = temp_carry;
				continue;
			}
		}

		remainder -= divisor;
		remainder -= (1 - carry);
		carry = 1;
		temp_carry = quotient >> 63;
		quotient <<= 1;
		quotient |= carry;
		carry = temp_carry;
	}

	*out_quotient = quotient;
}

INLINE attr_unused
void umul_128_64(uint64_t multiplicant, uint64_t multiplier, int128_bits_t *result) {
#if defined(__x86_64) || defined(__x86_64__)
    __asm__ (
        "mulq %3"
        : "=a,a" (result->lo),   "=d,d" (result->hi)
        : "%0,0" (multiplicant), "r,m"  (multiplier)
        : "cc"
    );
#else
	uint64_t u1 = (multiplicant & 0xffffffff);
	uint64_t v1 = (multiplier & 0xffffffff);
	uint64_t t = (u1 * v1);
	uint64_t w3 = (t & 0xffffffff);
	uint64_t k = (t >> 32);

	multiplicant >>= 32;
	t = (multiplicant * v1) + k;
	k = (t & 0xffffffff);
	uint64_t w1 = (t >> 32);

	multiplier >>= 32;
	t = (u1 * multiplier) + k;
	k = (t >> 32);

	result->hi = (multiplicant * multiplier) + w1 + k;
	result->lo = (t << 32) + w3;
#endif
}

INLINE attr_unused
uint64_t _umuldiv64_slow(uint64_t x, uint64_t multiplier, uint64_t divisor) {
	int128_bits_t intermediate;
	uint64_t result;
	umul_128_64(x, multiplier, &intermediate);
	udiv_128_64(intermediate, divisor, &result);
	return result;
}

#include "util.h"

INLINE
uint64_t _umuldiv64(uint64_t x, uint64_t multiplier, uint64_t divisor) {
#if defined(TAISEI_BUILDCONF_HAVE_INT128)
	typedef unsigned __int128 uint128_t;
	return ((uint128_t)x * (uint128_t)multiplier) / divisor;
#elif defined(TAISEI_BUILDCONF_HAVE_LONG_DOUBLE)
	#define UMULDIV64_SANITY_CHECK
	return ((long double)x * (long double)multiplier) / (long double)divisor;
#else
	return _umuldiv64_slow(x, multiplier, divisor);
#endif
}

uint64_t umuldiv64(uint64_t x, uint64_t multiplier, uint64_t divisor) {
#ifdef UMULDIV64_SANITY_CHECK
	static signed char sanity = -1;

	if(sanity < 0) {
		sanity = (_umuldiv64(UINT64_MAX, UINT64_MAX, UINT64_MAX) == UINT64_MAX);
	}

	return (sanity ? _umuldiv64 : _umuldiv64_slow)(x, multiplier, divisor);
#else
	return _umuldiv64(x, multiplier, divisor);
#endif
}

uint64_t uceildiv64(uint64_t x, uint64_t y) {
	return x / y + (x % y != 0);
}

int popcnt32(uint32_t x) {
	#ifdef TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNT
	return __builtin_popcount(x);
	#else
	return popcnt64(x);
	#endif
}

int popcnt64(uint64_t x) {
	#ifdef TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNTLL
	return __builtin_popcountll(x);
	#else
	x -= (x >> 1) & 0x5555555555555555;
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (x * 0x0101010101010101) >> 56;
	#endif
}
