/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#undef HAS_BUILTIN_COMPLEX

#if defined __has_builtin
	#if __has_builtin(__builtin_complex)
		#define HAS_BUILTIN_COMPLEX
	#endif
#else
	#if defined __GNUC__ && defined __GNUC_MINOR__
		#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
			#define HAS_BUILTIN_COMPLEX
		#endif
	#endif
#endif

#if defined __clang__
	// On these platforms CMPLX is defined without __extension__, causing warning spam
	#if defined __EMSCRIPTEN__ || defined __APPLE__
		#undef CMPLX
		#undef CMPLXF
	#endif
#endif

// In case the C11 CMPLX macro is not present, try our best to provide a substitute

#if !defined CMPLX
	#if defined HAS_BUILTIN_COMPLEX
		#define CMPLX(re,im) __builtin_complex((double)(re), (double)(im))
	#elif defined __clang__
		#define CMPLX(re,im) (__extension__ (_Complex double){ (double)(re), (double)(im) })
	#elif defined _Imaginary_I
		#define CMPLX(re,im) (_Complex double)((double)(re) + _Imaginary_I * (double)(im))
	#else
		#define CMPLX(re,im) (_Complex double)((double)(re) + _Complex_I * (double)(im))
	#endif
#endif

// same for CMPLXF

#if !defined CMPLXF
	#if defined HAS_BUILTIN_COMPLEX
		#define CMPLXF(re,im) __builtin_complex((float)(re), (float)(im))
	#elif defined __clang__
		#define CMPLXF(re,im) (__extension__ (_Complex float){ (float)(re), (float)(im) })
	#elif defined _Imaginary_I
		#define CMPLXF(re,im) (_Complex float)((float)(re) + _Imaginary_I * (float)(im))
	#else
		#define CMPLXF(re,im) (_Complex float)((float)(re) + _Complex_I * (float)(im))
	#endif
#endif
