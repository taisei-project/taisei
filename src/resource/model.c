/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "model.h"
#include "list.h"
#include "resource.h"
#include "taisei_err.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void parse_obj(const char *filename, ObjFileData *data) {
	FILE *fp = fopen(filename, "rb");

	char line[256];
	Vector buf;
	char mode;
	int linen = 0;

	memset(data, 0, sizeof(ObjFileData));

	while(fgets(line, sizeof(line), fp)) {
		linen++;

		char *first;
		first = strtok(line, " \n");

		if(strcmp(first, "v") == 0)
			mode = 'v';
		else if(strcmp(first, "vt") == 0)
			mode = 't';
		else if(strcmp(first, "vn") == 0)
			mode = 'n';
		else if(strcmp(first, "f") == 0)
			mode = 'f';
		else
			mode = 0;

		if(mode != 0 && mode != 'f') {
			buf[0] = atof(strtok(NULL, " \n"));
			buf[1] = atof(strtok(NULL, " \n"));
			if(mode != 't')
				buf[2] = atof(strtok(NULL, " \n"));

			switch(mode) {
			case 'v':
				data->xs = realloc(data->xs, sizeof(Vector)*(++data->xcount));
				memcpy(data->xs[data->xcount-1], buf, sizeof(Vector));
				break;
			case 't':
				data->texcoords = realloc(data->texcoords, sizeof(Vector)*(++data->tcount));
				memcpy(data->texcoords[data->tcount-1], buf, sizeof(Vector));
				break;
			case 'n':
				data->normals = realloc(data->normals, sizeof(Vector)*(++data->ncount));
				memcpy(data->normals[data->ncount-1], buf, sizeof(Vector));
				break;
			}
		} else if(mode == 'f') {
			char *segment, *seg;
			int j = 0, jj;
			IVector ibuf;
			memset(ibuf, 0, sizeof(ibuf));

			while((segment = strtok(NULL, " \n"))) {
				seg = segment;
				j++;

				jj = 0;
				while(jj < 3) {
					ibuf[jj] = atoi(seg);
					jj++;

					while(*seg != '\0' && *(++seg) != '/');

					if(*seg == '\0')
						break;
					else
						seg++;
				}

				if(strstr(segment, "//")) {
					ibuf[2] = ibuf[1];
					ibuf[1] = 0;
				}

				if(jj == 0 || jj > 3 || segment[0] == '/')
					errx(-1, "parse_obj():\n!- OBJ file '%s:%d': Parsing error: Corrupt face definition\n", filename,linen);

				data->indices = realloc(data->indices, sizeof(IVector)*(++data->icount));
				memcpy(data->indices[data->icount-1], ibuf, sizeof(IVector));
			}

			if(data->fverts == 0)
				data->fverts = j;

			if(data->fverts != j)
				errx(-1, "parse_obj():\n!- OBJ file '%s:%d': Parsing error: face vertex count must stay the same in the whole file\n", filename, linen);

			if(data->fverts != 3 && data->fverts != 4)
				errx(-1, "parse_obj():\n!- OBJ file '%s:%d': Parsing error: face vertex count must be either 3 or 4\n", filename, linen);
		}
	}

	fclose(fp);
}

static void bad_reference_error(const char *filename, const char *aux, int n) {
	errx(-1, "load_model():\n!- OBJ file '%s': Index %d: bad %s index reference\n", filename, n, aux);
}

Model *load_model(const char *filename) {
	Model *m = malloc(sizeof(Model));

	ObjFileData data;
	unsigned int i;

	Vertex *verts;
	unsigned int ioffset = _vbo.offset;

	char *beg = strstr(filename, "models/") + 7;
	char *end = strrchr(filename, '.');

	int sz = end - beg + 1;
	char name[sz];
	strlcpy(name, beg, sz);

	parse_obj(filename, &data);

	m->fverts = data.fverts;
	m->indices = calloc(data.icount, sizeof(unsigned int));
	m->icount = data.icount;

	verts = calloc(data.icount, sizeof(Vertex));


	memset(verts, 0, data.icount*sizeof(Vertex));
	for(i = 0; i < data.icount; i++) {
		int xi, ni, ti;

		xi = data.indices[i][0]-1;
		if(xi < 0 || xi >= data.xcount)
			bad_reference_error(filename, "vertex", i);

		memcpy(verts[i].x, data.xs[xi], sizeof(Vector));

		if(data.tcount) {
			ti = data.indices[i][1]-1;
			if(ti < 0 || ti >= data.tcount)
				bad_reference_error(filename, "texcoord", i);

			verts[i].s = data.texcoords[ti][0];
			verts[i].t = data.texcoords[ti][1];
		}

		if(data.ncount) {
			ni = data.indices[i][2]-1;
			if(ni < 0 || ni >= data.ncount)
				bad_reference_error(filename, "normal", ni);

			memcpy(verts[i].n, data.normals[ni], sizeof(Vector));
		}

		m->indices[i] = i+ioffset;
	}

	vbo_add_verts(&_vbo, verts, data.icount);

	hashtable_set_string(resources.models, name, m);

	printf("-- loaded '%s' as '%s'\n", filename, name);

	free(verts);
	free(data.xs);
	free(data.normals);
	free(data.texcoords);
	free(data.indices);
	return m;
}

Model *get_model(const char *name) {
	Model *res = hashtable_get_string(resources.models, name);

	if(res == NULL)
		errx(-1,"get_model():\n!- cannot load model '%s'", name);

	return res;
}

void draw_model_p(Model *model) {
	GLenum flag = GL_NONE;
	switch(model->fverts) {
	case 3:
		flag = GL_TRIANGLES;
		break;
	case 4:
		flag = GL_QUADS;
		break;
	default:
		errx(-1, "draw_model_p():\n!- Model '%s': invalid face vertex count");
	}

	glMatrixMode(GL_TEXTURE);
	glScalef(1,-1,1); // every texture in taisei is actually read vertically mirrored. and I noticed that just now.

	glDrawElements(flag, model->icount, GL_UNSIGNED_INT, model->indices);

	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void draw_model(const char *name) {
	draw_model_p(get_model(name));
}

void* delete_model(void *name, void *model, void *arg) {
	free(((Model *)model)->indices);
	free(model);
	return NULL;
}

void delete_models(void) { // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space
	resources_delete_and_unset_all(resources.models, delete_model, NULL);
}
