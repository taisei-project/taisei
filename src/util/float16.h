/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

/*
 * NOTE: This is a storage-only format. You must not directly initialize it or perform math
 * operations on it.
 */
typedef struct float16_storage {
	TAISEI_BUILDCONF_F16_STORAGE_TYPE _storage;
} float16_storage_t;

#if defined(TAISEI_BUILDCONF_F16_CVT_TYPE)

// Compiler has native support for float16 conversions through a special type.
// Note that it might not be possible to return that type from functions or pass it as arguments
// directly.

typedef TAISEI_BUILDCONF_F16_CVT_TYPE float16_cvt_t;

union f16_cvt {
	float16_cvt_t as_cvt;
	float16_storage_t as_storage;
};

attr_const
INLINE float16_storage_t f32_to_f16(float x) {
	assert(isfinite(x));
	return ((union f16_cvt) { .as_cvt = x }).as_storage;
}

attr_const
INLINE float f16_to_f32(float16_storage_t x) {
	return ((union f16_cvt) { .as_storage = x }).as_cvt;
}

#elif \
	defined(TAISEI_BUILDCONF_F16_RT_ABI_TYPE) && \
	defined(TAISEI_BUILDCONF_F16_RT_FUNC_F2H)	 && \
	defined(TAISEI_BUILDCONF_F16_RT_FUNC_H2F)

// Conversion functions are available as part of the runtime library

typedef TAISEI_BUILDCONF_F16_RT_ABI_TYPE float16_rtabi_t;

float TAISEI_BUILDCONF_F16_RT_FUNC_H2F(float16_rtabi_t);
float16_rtabi_t TAISEI_BUILDCONF_F16_RT_FUNC_F2H(float);

union f16_rtabi_cvt {
	float16_rtabi_t as_rtabi;
	float16_storage_t as_storage;
};

attr_const
INLINE float16_storage_t f32_to_f16(float x) {
	assert(isfinite(x));
	return ((union f16_rtabi_cvt) {
		.as_rtabi = TAISEI_BUILDCONF_F16_RT_FUNC_F2H(x)
	}).as_storage;
}

attr_const
INLINE float f16_to_f32(float16_storage_t x) {
	return TAISEI_BUILDCONF_F16_RT_FUNC_H2F(
		((union f16_rtabi_cvt) { .as_storage = x }).as_rtabi
	);
}

#else

// Resort to vendored fallbacks

float f16_to_f32(float16_storage_t x) attr_const;
float16_storage_t f32_to_f16(float x) attr_const;

#endif
