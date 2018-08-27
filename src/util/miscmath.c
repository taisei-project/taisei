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

double psin(double x) {
	return 0.5 + 0.5 * sin(x);
}

double max(double a, double b) {
	return (a > b)? a : b;
}

double min(double a, double b) {
	return (a < b)? a : b;
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
