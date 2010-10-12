#ifndef VECTOR_H
#define VECTOR_H

#include <SDL/SDL_opengl.h>

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} Vertex, Vector;

void cross_product(Vector *res, Vector *a, Vector *b);
void make_vector(Vector *res, Vertex *a, Vertex *b);

void norm_vector(Vector *v);

#endif