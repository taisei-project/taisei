#include "vector.h"
#include "math.h"

void cross_product(Vector *res, Vector *a, Vector *b) {
	res->x = a->y*b->z - a->z*b->y;
	res->y = a->z*b->x - a->x*b->z;
	res->z = a->x*b->y - a->y*b->x;
}

void make_vector(Vector *res, Vertex *a, Vertex *b) {
	res->x = a->x-b->x;
	res->y = a->y-b->y;
	res->z = a->z-b->z;
}
	
void norm_vector(Vector *v) {
	GLfloat len = sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
	
	v->x = v->x/len;
	v->y = v->y/len;
	v->z = v->z/len;
}