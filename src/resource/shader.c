/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "shader.h"
#include "resource.h"
#include "config.h"
#include "list.h"
#include <stdio.h>
#include "taisei_err.h"

void print_info_log(GLuint shader) {
	int len = 0, alen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	
	if(len > 1) {
		char *log = malloc(len);
		memset(log, 0, len);
		glGetShaderInfoLog(shader, 256, &alen, log);
		printf("%s\n", log);
		free(log);
	}
}

void load_shader(const char *filename) {
	if(!GLEW_ARB_vertex_program || !GLEW_ARB_fragment_program) {
		warnx("missing shader support; skipping.");
		tconfig.intval[NO_SHADER] = 1;
		return;
	}
	
	FILE *file = fopen(filename,"r");
	if(file == NULL)
		errx(-1, "Error opening '%s'", filename);
	
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	if(size == 0)
		errx(-1, "File empty!");
	
	char *text = malloc(size+1);
	fread(text, size, 1, file);
	text[size] = 0;
	
	char *vtext = text;
	char *delim = strstr(text, DELIM);
	if(delim == NULL)
		errx(-1, "Expected '%s' delimiter.", DELIM);	
	
	*delim = 0;
	char *ftext = delim + DELIM_SIZE;
	
	Shader *sha = create_element((void **)&resources.shaders, sizeof(Shader));
	GLuint vshaderobj;
	GLuint fshaderobj;
	
	sha->prog = glCreateProgram();
	vshaderobj = glCreateShader(GL_VERTEX_SHADER);
	fshaderobj = glCreateShader(GL_FRAGMENT_SHADER);
	
	int s = -1;
	
	glShaderSource(vshaderobj, 1, (const GLchar **)&vtext, &s);
	glShaderSource(fshaderobj, 1, (const GLchar **)&ftext, &s);
	
	glCompileShader(vshaderobj);
	glCompileShader(fshaderobj);
	
	print_info_log(vshaderobj);
	print_info_log(fshaderobj);
			
	glAttachShader(sha->prog, vshaderobj);
	glAttachShader(sha->prog, fshaderobj);
	
	glDeleteShader(vshaderobj);
	glDeleteShader(fshaderobj);
	
	glLinkProgram(sha->prog);
	
	print_info_log(sha->prog);
	
	free(text);
	
	char *beg = strstr(filename, "shader/") + 7;
	char *end = strrchr(filename, '.');
	
	sha->name = malloc(end-beg + 1);
	memset(sha->name, 0, end-beg + 1);
	strncpy(sha->name, beg, end-beg);
	
	printf("-- loaded '%s' as '%s'\n", filename, sha->name);
}
	
GLuint get_shader(const char *name) {
	Shader *s, *res = NULL;
	for(s = resources.shaders; s; s = s->next) {
		if(strcmp(s->name, name) == 0)
			res = s;
	}
	
	if(res == NULL)
		errx(-1,"get_shader():\n!- cannot load shader '%s'", name);
	
	return res->prog;
}

void delete_shader(void **shas, void *sha) {
	free(((Shader *)sha)->name);
	glDeleteShader(((Shader*)sha)->prog);
	
	delete_element(shas, sha);
}

void delete_shaders() {
	delete_all_elements((void **)&resources.shaders, delete_shader);
}