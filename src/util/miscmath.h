/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#define DEG2RAD (M_PI/180.0)
#define RAD2DEG (180.0/M_PI)

double min(double, double) attr_const;
double max(double, double) attr_const;
double clamp(double, double, double) attr_const;
double approach(double v, double t, double d) attr_const;
double psin(double) attr_const;
int sign(double) attr_const;
double swing(double x, double s) attr_const;
uint32_t topow2_u32(uint32_t  x) attr_const;
uint64_t topow2_u64(uint64_t x) attr_const;
float ftopow2(float x) attr_const;
float smooth(float x) attr_const;
float smoothreclamp(float x, float old_min, float old_max, float new_min, float new_max) attr_const;
float sanitize_scale(float scale) attr_const;
float normpdf(float x, float sigma) attr_const;
void gaussian_kernel_1d(size_t size, float sigma, float kernel[size]) attr_nonnull(3);

#define topow2(x) (_Generic((x), \
	uint32_t: topow2_u32, \
	uint64_t: topow2_u64, \
	default: topow2_u64 \
)(x))

#include <cglm/types.h>

typedef float vec2_noalign[2];
typedef float vec3_noalign[3];
typedef int ivec3_noalign[3];
typedef float vec4_noalign[4];
typedef vec3_noalign mat3_noalign[3];
typedef vec4_noalign mat4_noalign[4];
