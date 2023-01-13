/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "float16.h"

// Evil bit hackery stolen from stack overflow: https://stackoverflow.com/a/60047308

float16_storage_t f32_to_f16(float x) {
	assert(isfinite(x));
	// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
	uint32_t b = UNION_CAST(float, uint32_t, x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
	uint32_t e = (b & 0x7F800000) >> 23; // exponent
	uint32_t m = (b & 0x007FFFFF); // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
	return UNION_CAST(uint16_t, float16_storage_t,
		// sign : normalized : denormalized : saturate
		(b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
		((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
		(e > 143) * 0x7FFF
	);
}

float f16_to_f32(float16_storage_t f16) {
	// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
	uint16_t x = UNION_CAST(float16_storage_t, uint16_t, f16);
	uint32_t e = (x & 0x7C00) >> 10; // exponent
	uint32_t m = (x & 0x03FF) << 13; // mantissa
	uint32_t v = UNION_CAST(float, uint32_t, m) >> 23; // evil log2 bit hack to count leading zeros in denormalized format
	return UNION_CAST(uint32_t, float,
		// sign : normalized : denormalized
		(x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) |
		((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000))
	);
}
