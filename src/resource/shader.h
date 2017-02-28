/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SHADER_H
#define SHADER_H

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#define DELIM "%% -- FRAG"
#define DELIM_SIZE 10

struct Uniform;
typedef struct Uniform {
	char *name;
	GLint location;
} Uniform;

struct Shader;
typedef struct Shader {
	GLuint prog;
	int unicount;
	Uniform *uniforms;
} Shader;

void load_shader_snippets(const char *filename, const char *prefix);
void load_shader_file(const char *filename);
void load_shader(const char *vheader, const char *fheader, const char *vtext, const char *ftext, const char *name, int nsize);
Shader *get_shader(const char *name);
void delete_shaders(void);

void cache_uniforms(Shader *sha);
int uniloc(Shader *sha, const char *name);
#endif
