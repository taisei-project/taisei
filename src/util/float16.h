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

float16_storage_t f32_to_f16(float x) attr_const;
float f16_to_f32(float16_storage_t x) attr_const;

#endif

// Vector operations

#define F16_DEFINE_VECTOR_CONVERSION_SCALAR(vecsize) \
	INLINE void f32v##vecsize##_to_f16v##vecsize(float16_storage_t dst[vecsize], const float src[vecsize]) { \
		for(int i = 0; i < vecsize; ++i) { \
			dst[i] = f32_to_f16(src[i]); \
		} \
	} \
	\
	INLINE void f16v##vecsize##_to_f32v##vecsize(float dst[vecsize], const float16_storage_t src[vecsize]) { \
		for(int i = 0; i < vecsize; ++i) { \
			dst[i] = f16_to_f32(src[i]); \
		} \
	}

#ifdef TAISEI_BUILDCONF_F16_SIMD_TYPE

typedef TAISEI_BUILDCONF_F16_SIMD_TYPE f16_simd_t;

// NOTE: Sadly GCC 12 still can't vectorize this, but clang can.

#define F16_DEFINE_VECTOR_CONVERSION(vecsize) \
	typedef float      f32v##vecsize##simd __attribute__((vector_size(vecsize * sizeof(float)))); \
	typedef f16_simd_t f16v##vecsize##simd __attribute__((vector_size(vecsize * sizeof(f16_simd_t)))); \
	\
	INLINE void f32v##vecsize##_to_f16v##vecsize(float16_storage_t dst[vecsize], const float src[vecsize]) { \
		f32v##vecsize##simd v32_simd; \
		memcpy(&v32_simd, src, sizeof(v32_simd)); \
		auto v16_simd = __builtin_convertvector(v32_simd, f16v##vecsize##simd); \
		memcpy(dst, &v16_simd, sizeof(v16_simd)); \
	} \
	\
	INLINE void f16v##vecsize##_to_f32v##vecsize(float dst[vecsize], const float16_storage_t src[vecsize]) { \
		f16v##vecsize##simd v16_simd; \
		memcpy(&v16_simd, src, sizeof(v16_simd)); \
		auto v32_simd = __builtin_convertvector(v16_simd, f32v##vecsize##simd); \
		memcpy(dst, &v32_simd, sizeof(v32_simd)); \
	}

#else

#define F16_DEFINE_VECTOR_CONVERSION(vecsize) \
	F16_DEFINE_VECTOR_CONVERSION_SCALAR(vecsize)

#endif

/*
 * Defines functions:
 *
 *		void f16vX_to_f32vX(float dst[X], const float16_storage_t src[X]);
 *		void f32vX_to_f16vX(float16_storage_t dst[X], const float src[X]);
 *
 * Where X is the vector size.
 */

F16_DEFINE_VECTOR_CONVERSION(4)

#ifdef __clang__
	F16_DEFINE_VECTOR_CONVERSION(3)
#else
	F16_DEFINE_VECTOR_CONVERSION_SCALAR(3)
#endif

F16_DEFINE_VECTOR_CONVERSION(2)
