/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)
#define GOLDEN_RATIO 1.618033988749895
#define M_TAU (M_PI * 2)

#define re(z) (__real__ (z))
#define im(z) (__imag__ (z))

double (creal)(cmplx z) attr_deprecated("Use re() instead");
double (cimag)(cmplx z) attr_deprecated("Use im() instead");
float (crealf)(cmplxf z) attr_deprecated("Use re() instead");
float (cimagf)(cmplxf z) attr_deprecated("Use im() instead");

#define min(a, b) ({ \
	typeof((a)+(b)) _temp_a = (a); \
	typeof((a)+(b)) _temp_b = (b); \
	_temp_a > _temp_b ? _temp_b : _temp_a; \
})

#define max(a, b) ({ \
	typeof((a)+(b)) _temp_a = (a); \
	typeof((a)+(b)) _temp_b = (b); \
	_temp_a > _temp_b ? _temp_a : _temp_b; \
})

double lerp(double v0, double v1, double f) attr_const;
float lerpf(float v0, float v1, float f) attr_const;
cmplx clerp(cmplx v0, cmplx v1, double f) attr_const;
cmplxf clerpf(cmplxf v0, cmplxf v1, float f) attr_const;
intmax_t imin(intmax_t, intmax_t) attr_const;
intmax_t imax(intmax_t, intmax_t) attr_const;
uintmax_t umin(uintmax_t, uintmax_t) attr_const;
uintmax_t umax(uintmax_t, uintmax_t) attr_const;
intmax_t iclamp(intmax_t, intmax_t, intmax_t) attr_const;
double clamp(double, double, double) attr_const;
float clampf(float, float, float) attr_const;
double smoothstep(double edge0, double edge1, double x) attr_const;
double smoothmin(double a, double b, double k) attr_const;
double approach(double v, double t, double d) attr_const;
float fapproach(float v, float t, float d) attr_const;
double approach_p(double *v, double t, double d);
float fapproach_p(float *v, float t, float d);
double approach_asymptotic(double val, double target, double rate, double epsilon) attr_const;
float fapproach_asymptotic(float val, float target, float rate, float epsilon) attr_const;
cmplx capproach_asymptotic(cmplx val, cmplx target, double rate, double epsilon) attr_const;
double approach_asymptotic_p(double *val, double target, double rate, double epsilon);
float fapproach_asymptotic_p(float *val, float target, float rate, float epsilon);
cmplx capproach_asymptotic_p(cmplx *val, cmplx target, double rate, double epsilon);
double cabs2(cmplx c) attr_const;
float cabs2f(cmplxf c) attr_const;
cmplx cnormalize(cmplx c) attr_const;
cmplx cclampabs(cmplx c, double maxabs) attr_const;
cmplx cwclamp(cmplx c, cmplx cmin, cmplx cmax) attr_const;
cmplx cdir(double angle) attr_const;
cmplx cwmul(cmplx c0, cmplx c1) attr_const;
cmplxf cwmulf(cmplxf c0, cmplxf c1) attr_const;
double cdot(cmplx c0, cmplx c1) attr_const;
float cdotf(cmplxf c0, cmplxf c1) attr_const;
cmplx cswap(cmplx c) attr_const;
double ccross(cmplx a, cmplx b) attr_const;
float ccrossf(cmplxf a, cmplxf b) attr_const;
cmplx csort(cmplx z) attr_const;
cmplxf csortf(cmplxf z) attr_const;
double psin(double) attr_const;
double pcos(double) attr_const;
float psinf(float) attr_const;
float pcosf(float) attr_const;
int sign(double) attr_const;
double swing(double x, double s) attr_const;
double sawtooth(double x) attr_const;
double triangle(double x) attr_const;
uint32_t topow2_u32(uint32_t x) attr_const;
uint64_t topow2_u64(uint64_t x) attr_const;
float smooth(float x) attr_const;
double circle_angle(double index, double max_elements) attr_const;
cmplx circle_dir(double index, double max_elements) attr_const;
cmplx circle_dir_ofs(double index, double max_elements, double ofs) attr_const;
uint64_t upow10(uint n) attr_const;
uint digitcnt(uint64_t x) attr_const;
uint64_t uceildiv64(uint64_t x, uint64_t y) attr_const;
int popcnt32(uint32_t) attr_const;
int popcnt64(uint64_t) attr_const;

// Compute (a*b)/c with 128-bit intermediate precision.
// If the final result would not fit into 64 bits, the return value is undefined.
uint64_t umuldiv64(uint64_t x, uint64_t multiplier, uint64_t divisor);

#define topow2(x) (_Generic((x), \
	uint32_t: topow2_u32, \
	uint64_t: topow2_u64, \
	default: topow2_u64 \
)(x))

#include <cglm/types.h>

typedef float         vec2_noalign[2];
typedef int          ivec2_noalign[2];
typedef float         vec3_noalign[3];
typedef int          ivec3_noalign[3];
typedef float         vec4_noalign[4];
typedef int          ivec4_noalign[4];
typedef vec3_noalign  mat3_noalign[3];
typedef vec4_noalign  mat4_noalign[4];
