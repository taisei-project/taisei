/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef cglm_types_h
#define cglm_types_h

#if defined(_MSC_VER)
/* do not use alignment for older visual studio versions */
#if _MSC_VER < 1913 /*  Visual Studio 2017 version 15.6  */
#  define CGLM_ALL_UNALIGNED
#  define CGLM_ALIGN(X) /* no alignment */
#else
#  define CGLM_ALIGN(X) __declspec(align(X))
#endif
#else
#  define CGLM_ALIGN(X) __attribute((aligned(X)))
#endif

#ifndef CGLM_ALL_UNALIGNED
#  define CGLM_ALIGN_IF(X) CGLM_ALIGN(X)
#else
#  define CGLM_ALIGN_IF(X) /* no alignment */
#endif

#ifdef __GNUC__
#  define CGLM_ASSUME_ALIGNED(ptr, alignment) (__builtin_assume_aligned(ptr, alignment))
#else
#  define CGLM_ASSUME_ALIGNED(ptr, alignment) (ptr)
#endif

#if __STDC_VERSION__ >= 201112L
#  include <stdalign.h>
#  define CGLM_ALIGNOF(X) alignof(X)
#elif defined ( _MSC_VER )
#  define CGLM_ALIGNOF(X) __alignof(X)
#else // assume GCC-compatible
#  define CGLM_ALIGNOF(X) __alignof__(X)
#endif

typedef float                   vec2[2];
typedef CGLM_ALIGN_IF(8)  float vec3[3];
typedef int                    ivec3[3];
typedef CGLM_ALIGN_IF(16) float vec4[4];

typedef vec3                    mat3[3];
typedef CGLM_ALIGN_IF(16) vec4  mat4[4];

typedef vec4                    versor;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif

#define CGLM_PI    ((float)M_PI)
#define CGLM_PI_2  ((float)M_PI_2)
#define CGLM_PI_4  ((float)M_PI_4)

/*!
 * if you have axis order like vec3 orderVec = [0, 1, 2] or [0, 2, 1]...
 * vector then you can convert it to this enum by doing this:
 * @code
 * glm_euler_sq order;
 * order = orderVec[0] | orderVec[1] << 2 | orderVec[2] << 4;
 * @endcode
 * you may need to explicit cast if required
 */
typedef enum glm_euler_sq {
  GLM_EULER_XYZ = 0 << 0 | 1 << 2 | 2 << 4,
  GLM_EULER_XZY = 0 << 0 | 2 << 2 | 1 << 4,
  GLM_EULER_YZX = 1 << 0 | 2 << 2 | 0 << 4,
  GLM_EULER_YXZ = 1 << 0 | 0 << 2 | 2 << 4,
  GLM_EULER_ZXY = 2 << 0 | 0 << 2 | 1 << 4,
  GLM_EULER_ZYX = 2 << 0 | 1 << 2 | 0 << 4
} glm_euler_sq;

#endif /* cglm_types_h */
