/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MODEL_H
#define MODEL_H

#include "matrix.h"

typedef int IVector[3];

typedef struct ObjFileData ObjFileData;
struct ObjFileData {
	Vector *xs;
	int xcount;
	
	Vector *normals;
	int ncount;
	
	Vector *texcoords;
	int tcount;
	
	IVector *indices;
	int icount;
	
	int fverts;
};

typedef struct Model Model;
struct Model {
	struct Model *next;
	struct Model *prev;
	
	char *name;
	
	unsigned int *indices;
	int icount;
	
	int fverts;
};

Model *load_model(char *filename);

Model *get_model(char *name);

void draw_model_p(Model *model);
void draw_model(char *name);
void delete_models(void); // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space

#endif
