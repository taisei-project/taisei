/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

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
	struct Shader *next;
	struct Shader *prev;
	bool transient;

	char *name;
	GLuint prog;

	int unicount;
	Uniform *uniforms;
} Shader;

void load_shader_snippets(char *filename, char *prefix);
void load_shader_file(char *filename, bool transient);
void load_shader(char *vheader, char *fheader, char *vtext, char *ftext, char *name, int nsize, bool transient);
Shader *get_shader(const char *name);
void delete_shaders(bool transient);

void cache_uniforms(Shader *sha);
int uniloc(Shader *sha, char *name);
#endif
