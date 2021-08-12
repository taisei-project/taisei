/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_compat_h
#define IGUARD_util_compat_h

#include "taisei.h"

// Common standard library headers
#include <complex.h>
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include "util/assert.h"

#ifdef __FAST_MATH__
	#error -ffast-math is prohibited
#endif

#ifdef _WIN32
	// Include the god-awful windows.h header here so that we can fight the obnoxious namespace pollution it introduces.
	// We obviously don't need it in every source file, but it's easier to do it globally and early than to try to infer
	// just where down the maze of includes some of our dependencies happens to smuggle it in.
	//
	// *sigh*
	//
	// Goddamn it.

	// Make sure we get the "unicode" (actually UTF-16) versions of win32 APIs; it defaults to legacy crippled ones.
	#ifndef UNICODE
		#define UNICODE
	#endif
	#ifndef _UNICODE
		#define _UNICODE
	#endif

	// Ask windows.h to include a little bit less of the stupid crap we'll never use.
	// Some of it actually clashes with our names.
	#define WIN32_LEAN_AND_MEAN
	#define NOGDI
	#define NOMINMAX

	#include <windows.h>

	// far/near pointers are obviously very relevant for modern CPUs and totally deserve their very own, unprefixed keywords!
	#undef near
	#undef far

	// fixup other random name clashes
	#define mouse_event _taisei_mouse_event
#endif

// This macro should be provided by stddef.h, but in practice it sometimes is not.
#ifndef offsetof
	#ifdef __GNUC__
		#define offsetof(type, field) __builtin_offsetof(type, field)
	#else
		#define offsetof(type, field) ((size_t)&(((type *)0)->field))
	#endif
#endif

#define PRAGMA(p) _Pragma(#p)

#ifndef __GNUC__ // clang defines this too
	#pragma Unsupported compiler, expect nothing to work

	#define __attribute__(...)
	#define __extension__
	#define UNREACHABLE
	#define DIAGNOSTIC(x)
	#define DIAGNOSTIC_GCC(x)
	#define DIAGNOSTIC_CLANG(x)
	#define LIKELY(x) (bool)(x)
	#define UNLIKELY(x) (bool)(x)
#else
	#define UNREACHABLE __builtin_unreachable()

	#define DIAGNOSTIC(x) PRAGMA(GCC diagnostic x)

	#if defined(__clang__)
		#define DIAGNOSTIC_GCC(x)
		#define DIAGNOSTIC_CLANG(x) PRAGMA(clang diagnostic x)
	#else
		#define DIAGNOSTIC_GCC(x) PRAGMA(GCC diagnostic x)
		#define DIAGNOSTIC_CLANG(x)
	#endif

	#define LIKELY(x) __builtin_expect((bool)(x), 1)
	#define UNLIKELY(x) __builtin_expect((bool)(x), 0)
#endif

#ifndef __has_attribute
	#ifdef __GNUC__
		#define __has_attribute(attr) 1
	#else
		#define __has_attribute(attr) 0
	#endif
#endif

#ifndef __has_feature
	#define __has_feature(feature) 0
#endif

#undef ASSUME

#ifdef __has_builtin
	#if __has_builtin(__builtin_assume)
		#define ASSUME(x) __builtin_assume(x)
	#endif

	#if __has_builtin(__builtin_popcount)
		#undef TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNT
		#define TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNT 1
	#endif

	#if __has_builtin(__builtin_popcountll)
		#undef TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNTLL
		#define TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNTLL 1
	#endif
#endif

#if !defined(ASSUME) && defined(__GNUC__)
	#define ASSUME(x) do { if(!(x)) { UNREACHABLE; } } while(0)
#endif

#ifndef ASSUME
	#define ASSUME(x)
#endif

// On windows, use the MinGW implementations of printf and friends instead of the crippled mscrt ones.
#ifdef __USE_MINGW_ANSI_STDIO
	#define FORMAT_ATTR __MINGW_PRINTF_FORMAT
#else
	#define FORMAT_ATTR printf
#endif

#undef uint
typedef unsigned int uint;

#undef ushort
typedef unsigned short ushort;

#undef ulong
typedef unsigned long ulong;

#undef uchar
typedef unsigned char uchar;

#undef schar
typedef signed char schar;

#undef real
typedef double real;

#undef cmplxf
typedef _Complex float cmplxf;

#undef cmplx
typedef _Complex double cmplx;

// These definitions are common but non-standard, so we provide our own
#undef M_PI
#undef M_PI_2
#undef M_PI_4
#undef M_E
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#define M_E 2.7182818284590452354

#ifndef TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T
	#if TAISEI_BUILDCONF_MALLOC_ALIGNMENT <= 0
		#warning malloc alignment is unknown, assuming 8
		#undef TAISEI_BUILDCONF_MALLOC_ALIGNMENT
		#define TAISEI_BUILDCONF_MALLOC_ALIGNMENT 8
	#endif

	#undef max_align_t
	#define max_align_t _fake_max_align_t
	typedef struct { alignas(TAISEI_BUILDCONF_MALLOC_ALIGNMENT) char a; } max_align_t;
#endif

// polyfill CMPLX macros
#include "compat_cmplx.h"

/*
 * Abstract away the nasty GNU attribute syntax.
 */

// Function is a hot spot.
#define attr_hot \
	__attribute__ ((hot))

// Function has no side-effects.
#define attr_pure \
	__attribute__ ((pure))

// Function has no side-effects, return value depends on arguments only.
// Must not take pointer parameters, must not return void.
#define attr_const \
	__attribute__ ((const))

// Function never returns NULL.
#define attr_returns_nonnull \
	__attribute__ ((returns_nonnull))

// Function must be called with NULL as the last argument (for varargs functions).
#define attr_sentinel \
	__attribute__ ((sentinel))

// Symbol is meant to be possibly unused.
#define attr_unused \
	__attribute__ ((unused))

// Symbol should be emitted even if it appears to be unused.
#define attr_used \
	__attribute__ ((used))

// Function or type is deprecated and should not be used.
#define attr_deprecated(msg) \
	__attribute__ ((deprecated(msg)))

// Function parameters at specified positions must not be NULL.
#define attr_nonnull(...) \
	__attribute__ ((nonnull(__VA_ARGS__)))

// All pointer parameters must not be NULL.
#define attr_nonnull_all \
	__attribute__ ((nonnull))

// The return value of this function must not be ignored.
#define attr_nodiscard \
	__attribute__ ((warn_unused_result))

// Function takes a printf-style format string and variadic arguments.
#define attr_printf(fmt_index, firstarg_index) \
	__attribute__ ((format(FORMAT_ATTR, fmt_index, firstarg_index)))

// Function must be inlined regardless of optimization settings.
#define attr_must_inline \
	__attribute__ ((always_inline))

// Function returns a pointer aligned to x bytes
#define attr_returns_aligned(x) \
	__attribute__ ((assume_aligned(x)))

// Function returns a pointer aligned the same as max_align_t
#define attr_returns_max_aligned \
	attr_returns_aligned(alignof(max_align_t))

// Shorthand: always returns non-null pointer aligned to max_align_t; no discard.
#define attr_returns_allocated \
	attr_returns_nonnull attr_returns_max_aligned attr_nodiscard

// Structure must not be initialized with an implicit (non-designated) initializer.
#if __has_attribute(designated_init) && defined(TAISEI_BUILDCONF_USE_DESIGNATED_INIT)
	#define attr_designated_init \
		__attribute__ ((designated_init))
#else
	#define attr_designated_init
#endif

// Function returns a pointer that can't alias any other pointer when the function returns.
// Storage pointed at doesn't contain pointers to any valid objects.
#define attr_malloc \
	__attribute__ ((malloc))

// Function returns a pointer to object whose size is specified by the 'size_arg_index'th function argument.
#define attr_alloc_size(size_arg_index) \
	__attribute__ ((alloc_size(size_arg_index)))

#define INLINE static inline attr_must_inline __attribute__((gnu_inline))

#define ASSUME_ALIGNED(expr, alignment) ({ \
	static_assert(__builtin_constant_p(alignment), ""); \
	__auto_type _assume_aligned_ptr = (expr); \
	assert(((uintptr_t)_assume_aligned_ptr & ((alignment) - 1)) == 0); \
	__builtin_assume_aligned(_assume_aligned_ptr, (alignment)); \
})

#define UNION_CAST(_from_type, _to_type, _expr) \
	((union { _from_type f; _to_type t; }) { .f = (_expr) }).t

#define CASTPTR_ASSUME_ALIGNED(expr, type) ((type*)ASSUME_ALIGNED((expr), alignof(type)))

#define NOT_NULL(expr) ({ \
	__auto_type _assume_not_null_ptr = (expr); \
	assume(_assume_not_null_ptr != NULL); \
	_assume_not_null_ptr; \
})

#ifdef __SWITCH__
	#include "../arch_switch.h"
	#define atexit nxAtExit
	#define exit nxExit
	#define abort nxAbort
#endif

#ifdef RNG_API_CHECK
	#define _Generic(ignore, ...) _Generic(0, __VA_ARGS__)
#endif

#if defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
	#define ADDRESS_SANITIZER
#endif

#endif // IGUARD_util_compat_h
