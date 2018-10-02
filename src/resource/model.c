/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "model.h"
#include "list.h"
#include "resource.h"
#include "renderer/api.h"

// TODO: Rewrite all of this mess, maybe even consider a different format
// IQM for instance: http://sauerbraten.org/iqm/

ResourceHandler model_res_handler = {
	.type = RES_MODEL,
	.typename = "model",
	.subdir = MDL_PATH_PREFIX,

	.procs = {
		.find = model_path,
		.check = check_model_path,
		.begin_load = load_model_begin,
		.end_load = load_model_end,
		.unload = unload_model,
	},
};

static void parse_obj(const char *filename, ObjFileData *data);
static void free_obj(ObjFileData *data);

char* model_path(const char *name) {
	return strjoin(MDL_PATH_PREFIX, name, MDL_EXTENSION, NULL);
}

bool check_model_path(const char *path) {
	return strendswith(path, MDL_EXTENSION);
}

typedef struct ModelLoadData {
	GenericModelVertex *verts;
	uint *indices;
	uint icount;
} ModelLoadData;

void* load_model_begin(const char *path, uint flags) {
	ObjFileData *data = malloc(sizeof(ObjFileData));
	GenericModelVertex *verts;

	parse_obj(path, data);

	uint *indices = calloc(data->icount, sizeof(uint));
	uint icount = data->icount;

	verts = calloc(data->icount, sizeof(GenericModelVertex));

#define BADREF(filename,aux,n) { \
	log_warn("OBJ file '%s': Index %d: bad %s index reference\n", filename, n, aux); \
	goto fail; \
}

	memset(verts, 0, data->icount*sizeof(GenericModelVertex));

	for(uint i = 0; i < data->icount; i++) {
		int xi, ni, ti;

		xi = data->indices[i][0]-1;
		if(xi < 0 || xi >= data->xcount)
			BADREF(path, "vertex", i);

		memcpy(verts[i].position, data->xs[xi], sizeof(vec3_noalign));

		if(data->tcount) {
			ti = data->indices[i][1]-1;
			if(ti < 0 || ti >= data->tcount)
				BADREF(path, "texcoord", i);

			verts[i].uv.s = data->texcoords[ti][0];
			verts[i].uv.t = data->texcoords[ti][1];
		}

		if(data->ncount) {
			ni = data->indices[i][2]-1;
			if(ni < 0 || ni >= data->ncount)
				BADREF(path, "normal", ni);

			memcpy(verts[i].normal, data->normals[ni], sizeof(vec3_noalign));
		}

		indices[i] = i;
	}

	free_obj(data);
	free(data);

#undef BADREF

	ModelLoadData *ldata = malloc(sizeof(ModelLoadData));
	ldata->verts = verts;
	ldata->indices = indices;
	ldata->icount = icount;

	return ldata;

fail:
	free(indices);
	free(verts);
	free_obj(data);
	free(data);
	return NULL;
}

void* load_model_end(void *opaque, const char *path, uint flags) {
	ModelLoadData *ldata = opaque;

	if(!ldata) {
		return NULL;
	}

	Model *model = calloc(1, sizeof(Model));
	r_model_add_static(model, PRIM_TRIANGLES, ldata->icount, ldata->verts, ldata->indices);

	free(ldata->verts);
	free(ldata->indices);
	free(ldata);

	return model;
}

void unload_model(void *model) { // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space
	free(model);
}

static void free_obj(ObjFileData *data) {
	free(data->xs);
	free(data->normals);
	free(data->texcoords);
	free(data->indices);
}

static void parse_obj(const char *filename, ObjFileData *data) {
	SDL_RWops *rw = vfs_open(filename, VFS_MODE_READ);

	if(!rw) {
		log_warn("VFS error: %s", vfs_get_error());
		return;
	}

	char line[256], *save;
	vec3_noalign buf;
	char mode;
	int linen = 0;

	memset(data, 0, sizeof(ObjFileData));

	while(SDL_RWgets(rw, line, sizeof(line))) {
		linen++;

		char *first;
		first = strtok_r(line, " \n", &save);

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
			buf[0] = atof(strtok_r(NULL, " \n", &save));
			char *wtf = strtok_r(NULL, " \n", &save);
			buf[1] = atof(wtf);
			if(mode != 't')
				buf[2] = atof(strtok_r(NULL, " \n", &save));

			switch(mode) {
			case 'v':
				data->xs = realloc(data->xs, sizeof(vec3_noalign)*(++data->xcount));
				memcpy(data->xs[data->xcount-1], buf, sizeof(vec3_noalign));
				break;
			case 't':
				data->texcoords = realloc(data->texcoords, sizeof(vec3_noalign)*(++data->tcount));
				memcpy(data->texcoords[data->tcount-1], buf, sizeof(vec3_noalign));
				break;
			case 'n':
				data->normals = realloc(data->normals, sizeof(vec3_noalign)*(++data->ncount));
				memcpy(data->normals[data->ncount-1], buf, sizeof(vec3_noalign));
				break;
			}
		} else if(mode == 'f') {
			char *segment, *seg;
			int j = 0, jj;
			ivec3_noalign ibuf;
			memset(ibuf, 0, sizeof(ibuf));

			while((segment = strtok_r(NULL, " \n", &save))) {
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
					log_fatal("OBJ file '%s:%d': Parsing error: Corrupt face definition", filename,linen);

				data->indices = realloc(data->indices, sizeof(ivec3_noalign)*(++data->icount));
				memcpy(data->indices[data->icount-1], ibuf, sizeof(ivec3_noalign));
			}

			if(data->fverts == 0)
				data->fverts = j;

			if(data->fverts != j)
				log_fatal("OBJ file '%s:%d': Parsing error: face vertex count must stay the same in the whole file", filename, linen);

			if(data->fverts != 3)
				log_fatal("OBJ file '%s:%d': Parsing error: face vertex count must be 3", filename, linen);
		}
	}

	SDL_RWclose(rw);
}

Model* get_model(const char *name) {
	return get_resource(RES_MODEL, name, RESF_DEFAULT)->data;
}

