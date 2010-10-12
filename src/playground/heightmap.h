#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "vector.h"

#define RESOLUTION 30

typedef struct {
	int w;
	int h;
	
	Vertex *verts;
	Vector *normals;

} Heightmap;

void load_heightmap(const char *path, Heightmap *hm);
void free_heightmap(Heightmap *hm);

void draw_heightmap(Heightmap *hm);

#endif