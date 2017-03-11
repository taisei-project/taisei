/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>
#include "taiseigl.h"
#include "hashtable.h"

typedef struct Shader {
	GLuint prog;
	Hashtable *uniforms;
} Shader;

char* shader_path(const char *name);
bool check_shader_path(const char *path);
void* load_shader_file(const char *path);
void unload_shader(void *vsha);

void load_shader_snippets(const char *filename, const char *prefix);
Shader* get_shader(const char *name);

int uniloc(Shader *sha, const char *name);

#define SHA_PATH_PREFIX "shader/"
#define SHA_EXTENSION ".sha"

#define SHA_DELIM "%% -- FRAG"
#define SHA_DELIM_SIZE (sizeof(SHA_DELIM) - 1)

#endif
