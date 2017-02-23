/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <string.h>

#include "global.h"
#include "matrix.h"

Matrix _identity = {
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1}
};

void matcpy(Matrix dest, Matrix src) {
	memcpy(dest, src, sizeof(Matrix));
}

void matmul(Matrix dest, Matrix a, Matrix b) {
	int i,j,k;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			dest[i][j] = 0;
			for(k = 0; k < 4; k++) {
				dest[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

void matrotate(Matrix md, Matrix ms, float angle, float x, float y, float z) {
	float c = cos(angle);
	float s = sin(angle);

	Matrix a = {
		{x*x*(1-c)+c, x*y*(1-c)-z*s, x*z*(1-c)+y*s, 0},
		{y*x*(1-c)+z*s, y*y*(1-c)+c, y*z*(1-c)-x*s, 0},
		{x*z*(1-c)-y*s, y*z*(1-c)+x*s, z*z*(1-c)+c, 0},
		{0, 0, 0, 1}
	};

	matmul(md, ms, a);
}

void matrotatez(Matrix md, Matrix ms, float angle) {
	float c = cos(angle);
	float s = sin(angle);

	Matrix a = {
		{c, -s, 0, 0},
		{s,  c, 0, 0},
		{0,  0, 1, 0},
		{0,  0, 0, 1}
	};

	matmul(md, ms, a);
}

void mattranslate(Matrix md, Matrix ms, float x, float y, float z) {
	Matrix a = {
		{1, 0, 0, x},
		{0, 1, 0, y},
		{0, 0, 1, z},
		{0, 0, 0, 1}
	};

	matmul(md, ms, a);
}

void matscale(Matrix md, Matrix ms, float x, float y, float z) {
	Matrix a = {
		{x, 0, 0, 0},
		{0, y, 0, 0},
		{0, 0, z, 0},
		{0, 0, 0, 1}
	};

	matmul(md, ms, a);
}

void matvec(Matrix m, Vector v) {
	Vector tmp;
	memcpy(tmp, v, sizeof(Vector));

	int i, k;
	for(i = 0; i < 3; i++) {
		v[i] = 0;
		for(k = 0; k < 4; k++) {
			float t = 1;
			if(k < 3)
				t = tmp[k];

			v[i] += m[i][k] * t;
		}
	}
}

void matvecv(Matrix m, Vector *v, int n) {
	int i;
	for(i = 0; i < n; i++) {
		matvec(m, v[i]);
	}
}

void normalize(Vector v) {
	int i;

	float len = length(v);

	for(i = 0; i < 3; i++)
		v[i] /= len;
}

float length(Vector v) {
	return sqrt(pow(v[0],2) + pow(v[1],2) + pow(v[2],2));
}
