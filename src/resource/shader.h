/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

#define DELIM "%% -- FRAG"
#define DELIM_SIZE 10

struct Shader;

typedef struct Shader {
	struct Shader *next;
	struct Shader *prev;
	
	char *name;
	GLuint prog;
} Shader;

void load_shader(const char *filename);
GLuint get_shader(const char *name);
void delete_shaders();

#endif