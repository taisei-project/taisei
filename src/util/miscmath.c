/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "miscmath.h"
#include "assert.h"

#include <stdlib.h>

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

void approach_p(double *v, double t, double d) {
	*v = approach(*v, t, d);
}

void fapproach_p(float *v, float t, float d) {
	*v = fapproach(*v, t, d);
}

double approach_asymptotic(double val, double target, double rate, double epsilon) {
	if(fabs(val - target) < epsilon || rate >= 1) {
		return target;
	}

	return val + (target - val) * rate;
}

float fapproach_asymptotic(float val, float target, float rate, float epsilon) {
	if(fabsf(val - target) < epsilon || rate >= 1) {
		return target;
	}

	return val + (target - val) * rate;
}

void approach_asymptotic_p(double *val, double target, double rate, double epsilon) {
	*val = approach_asymptotic(*val, target, rate, epsilon);
}

void fapproach_asymptotic_p(float *val, float target, float rate, float epsilon) {
	*val = fapproach_asymptotic(*val, target, rate, epsilon);
}

double psin(double x) {
	return 0.5 + 0.5 * sin(x);
}

double min(double a, double b) {
	return (a < b) ? a : b;
}

double max(double a, double b) {
	return (a > b) ? a : b;
}

intmax_t imin(intmax_t a, intmax_t b) {
	return (a < b) ? a : b;
}

intmax_t imax(intmax_t a, intmax_t b) {
	return (a > b) ? a : b;
}

double clamp(double f, double lower, double upper) {
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

float ftopow2(float x) {
	// NOTE: obviously this isn't the smallest possible power of two, but for our purposes, it could as well be.
	float y = 0.0625;
	while(y < x) y *= 2;
	return y;
}

float smooth(float x) {
	return 1.0 - (0.5 * cos(M_PI * x) + 0.5);
}

float smoothreclamp(float x, float old_min, float old_max, float new_min, float new_max) {
	x = (x - old_min) / (old_max - old_min);
	return new_min + (new_max - new_min) * smooth(x);
}

float sanitize_scale(float scale) {
	return max(0.1, scale);
}

float normpdf(float x, float sigma) {
    return 0.39894 * exp(-0.5 * pow(x, 2) / pow(sigma, 2)) / sigma;
}

void gaussian_kernel_1d(size_t size, float sigma, float kernel[size]) {
	assert(size & 1);

	double sum = 0.0;
	size_t halfsize = size / 2;

	kernel[halfsize] = normpdf(0, sigma);
	sum += kernel[halfsize];

	for(size_t i = 1; i <= halfsize; ++i) {
		float k = normpdf(i, sigma);
		kernel[halfsize + i] = kernel[halfsize - i] = k;
		sum += k * 2;

	}

	for(size_t i = 0; i < size; ++i) {
		kernel[i] /= sum;
	}
}

uint ipow10(uint n) {
	static uint pow10[] = {
		1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000,
	};

	assert(n < sizeof(pow10)/sizeof(*pow10));
	return pow10[n];
}

typedef struct int128_bits {
	uint64_t hi;
	uint64_t lo;
} int128_bits_t;

static inline attr_must_inline attr_unused
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

static inline attr_must_inline attr_unused
void umul_128_64(uint64_t multiplicant, uint64_t multiplier, int128_bits_t *result) {
#if (defined(__x86_64) || defined(__x86_64__))
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

static inline attr_must_inline attr_unused
uint64_t _umuldiv64_slow(uint64_t x, uint64_t multiplier, uint64_t divisor) {
	int128_bits_t intermediate;
	uint64_t result;
	umul_128_64(x, multiplier, &intermediate);
	udiv_128_64(intermediate, divisor, &result);
	return result;
}

#include "util.h"

static inline attr_must_inline
uint64_t _umuldiv64(uint64_t x, uint64_t multiplier, uint64_t divisor) {
#if defined(TAISEI_BUILDCONF_HAVE_INT128)
	__extension__ typedef unsigned __int128 uint128_t;
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
	static char sanity = -1;

	if(sanity < 0) {
		sanity = (_umuldiv64(UINT64_MAX, UINT64_MAX, UINT64_MAX) == UINT64_MAX);
	}

	return (sanity ? _umuldiv64 : _umuldiv64_slow)(x, multiplier, divisor);
#else
	return _umuldiv64(x, multiplier, divisor);
#endif
}
