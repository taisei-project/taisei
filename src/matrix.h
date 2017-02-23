/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MATRIX_H
#define MATRIX_H

typedef float Matrix[4][4];

typedef float Vector[3];

extern Matrix _identity;

void matcpy(Matrix dest, Matrix src);
void matmul(Matrix dest, Matrix a, Matrix b);

void matrotate(Matrix md, Matrix ms, float angle, float x, float y, float z);
void matrotatez(Matrix md, Matrix ms, float angle);
void mattranslate(Matrix md, Matrix ms, float x, float y, float z);
void matscale(Matrix md, Matrix ms, float x, float y, float z);

void matvec(Matrix m, Vector v);
void matvecv(Matrix m, Vector *vs, int size);

void normalize(Vector v);
float length(Vector v);

#endif
