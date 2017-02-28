/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <stdio.h>

#include "taiseigl.h"
#include "shader.h"
#include "resource.h"
#include "config.h"
#include "list.h"
#include "taisei_err.h"

static char snippet_header_EXT_draw_instanced[] =
	"#version 120\n"
	"#extension GL_EXT_draw_instanced : enable\n"
;

static char snippet_header_ARB_draw_instanced[] =
	"#version 120\n"
	"#extension GL_ARB_draw_instanced : enable\n"
	"#define gl_InstanceID gl_InstanceIDARB"
;

static char snippet_header_gl31[] =
	"#version 140\n"
;

static char* get_snippet_header(void) {
	if(glext.EXT_draw_instanced) {
		return snippet_header_EXT_draw_instanced;
	} else if(glext.ARB_draw_instanced) {
		return snippet_header_ARB_draw_instanced;
	} else {
		// probably won't work
		return snippet_header_gl31;
	}
}

void print_info_log(GLuint shader) {
	int len = 0, alen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

	if(len > 1) {
		printf(" == GLSL Shader info log (shader %u) ==\n", shader);
		char *log = malloc(len);
		memset(log, 0, len);
		glGetShaderInfoLog(shader, len, &alen, log);
		printf("%s\n", log);
		free(log);
		printf("\n == End of GLSL Shader info log (shader %u) ==\n", shader);
	}
}

void print_program_info_log(GLuint program) {
	int len = 0, alen = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

	if(len > 1) {
		printf(" == GLSL Program info log (program %u) ==\n", program);
		char *log = malloc(len);
		memset(log, 0, len);
		glGetProgramInfoLog(program, len, &alen, log);
		printf("%s\n", log);
		free(log);
		printf("\n == End of GLSL Program info log (program %u) ==\n", program);
	}
}

char *read_all(char *filename, int *size) {
	char *text;
	FILE *file = fopen(filename, "r");
	if(file == NULL)
		errx(-1, "Error opening '%s'", filename);

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if(*size == 0)
		errx(-1, "File empty!");

	text = malloc(*size+1);
	fread(text, *size, 1, file);
	text[*size] = 0;

	fclose(file);

	return text;
}

char *copy_segment(char *text, char *delim, int *size) {
	char *seg, *beg, *end;

	beg = strstr(text, delim);
	if(!beg)
		return NULL;
	beg += strlen(delim);

	end = strstr(beg, "%%");
	if(!end)
		return NULL;

	*size = end-beg;
	seg = malloc(*size+1);
	strlcpy(seg, beg, *size+1);

	return seg;
}

void load_shader_snippets(char *filename, char *prefix) {
	int size, vhsize = 0, vfsize = 0, fhsize = 0, ffsize = 0, ssize, prefixlen;
	char *text, *vhead, *vfoot, *fhead, *ffoot;
	char *sec, *name, *nend, *send;
	char *vtext = NULL, *ftext = NULL;
	char *nbuf;

	printf("-- loading snippet file '%s'\n", filename);

	prefixlen = strlen(prefix);

	text = read_all(filename, &size);

	sec = text;

	vhead = copy_segment(text, "%%VSHADER-HEAD%%", &vhsize);
	vfoot = copy_segment(text, "%%VSHADER-FOOT%%", &vfsize);

	fhead = copy_segment(text, "%%FSHADER-HEAD%%", &fhsize);
	ffoot = copy_segment(text, "%%FSHADER-FOOT%%", &ffsize);

	if(!vhead || !fhead)
		errx(-1, "Syntax Error: missing HEAD section(s).");
	if((vfoot == NULL) + (ffoot == NULL) != 1)
		errx(-1, "Syntax Error: must contain exactly 1 FOOT section");

	while((sec = strstr(sec, "%%"))) {
		sec += 2;

		name = sec;
		nend = strstr(name, "%%");
		if(!nend)
			errx(-1, "Syntax Error: expected '%%'");

		sec = nend + 2;

		if(strncmp(name+1, "SHADER", 6) == 0)
			continue;

		send = strstr(sec, "%%");
		if(!send)
			send = text + size + 1;
		send--;

		ssize = send-sec;

		if(vfoot) {
			vtext = malloc(vhsize + ssize + vfsize + 1);
			ftext = malloc(fhsize + 1);

			memset(vtext, 0, vhsize + ssize + vfsize + 1);
			memset(ftext, 0, fhsize + 1);

			strlcpy(vtext, vhead, vhsize+1);
			strlcpy(ftext, fhead, fhsize+1);

			strlcpy(vtext+vhsize, sec, ssize+1);
			strlcpy(vtext+vhsize+ssize, vfoot, vfsize+1);

		} else if(ffoot) {
			ftext = malloc(fhsize + ssize + ffsize + 1);
			vtext = malloc(vhsize + 1);

			memset(ftext, 0, fhsize + ssize + ffsize + 1);
			memset(vtext, 0, vhsize + 1);

			strlcpy(ftext, fhead, fhsize+1);
			strlcpy(vtext, vhead, vhsize+1);

			strlcpy(ftext+fhsize, sec, ssize+1);
			strlcpy(ftext+fhsize+ssize, ffoot, ffsize+1);
		}

		nbuf = malloc(nend-name+prefixlen+1);
		strcpy(nbuf, prefix);
		strlcat(nbuf+prefixlen, name, nend-name+1);
		nbuf[nend-name+prefixlen] = 0;

		load_shader(get_snippet_header(), NULL, vtext, ftext, nbuf, nend-name+prefixlen+1, false);
		printf("--- loaded snippet as shader '%s'\n", nbuf);

		free(nbuf);
		free(vtext);
		free(ftext);
	}

	free(vhead);
	free(fhead);
	free(vfoot);
	free(ffoot);
	free(text);
}

void load_shader_file(char *filename, bool transient) {
	int size;
	char *text, *vtext, *ftext, *delim;
	char *name = determine_name(filename);
	if(name == NULL)
		errx(-1, "load_shader_file(): unable to determine name of shader '%s'.", filename);

	text = read_all(filename, &size);

	vtext = text;
	delim = strstr(text, DELIM);
	if(delim == NULL)
		errx(-1, "load_shader_file(): Expected '%s' delimiter.", DELIM);

	*delim = 0;
	ftext = delim + DELIM_SIZE;

	
	load_shader(NULL, NULL, vtext, ftext, name, strlen(name) + 1, transient);

	printf("-- loaded '%s' as '%s'\n", filename, name);

	free(name);
	free(text);
}

void load_shader(char *vheader, char *fheader, char *vtext, char *ftext, char *name, int nsize, bool transient) {
	Shader *sha = get_shader(name);
	if (sha != NULL)
	{
		warnx("load_shader(): trying to load already loaded shader '%s' -> skipped.", name);
		return;
	}

	sha = create_element((void **)&resources.shaders, sizeof(Shader));
	GLuint vshaderobj;
	GLuint fshaderobj;

	sha->prog = glCreateProgram();
	vshaderobj = glCreateShader(GL_VERTEX_SHADER);
	fshaderobj = glCreateShader(GL_FRAGMENT_SHADER);

	if(!vheader) {
		vheader = "";
	}

	if(!fheader) {
		fheader = "";
	}

	GLchar *v_sources[] = { vheader, vtext };
	GLchar *f_sources[] = { fheader, ftext };
	GLint lengths[] = { -1, -1 };

	glShaderSource(vshaderobj, 2, (const GLchar**)v_sources, lengths);
	glShaderSource(fshaderobj, 2, (const GLchar**)f_sources, lengths);

	glCompileShader(vshaderobj);
	glCompileShader(fshaderobj);

	print_info_log(vshaderobj);
	print_info_log(fshaderobj);

	glAttachShader(sha->prog, vshaderobj);
	glAttachShader(sha->prog, fshaderobj);

	glDeleteShader(vshaderobj);
	glDeleteShader(fshaderobj);

	glLinkProgram(sha->prog);

	print_program_info_log(sha->prog);

	sha->name = malloc(nsize);
	sha->transient = transient;
	strlcpy(sha->name, name, nsize);

	cache_uniforms(sha);
}

void cache_uniforms(Shader *sha) {
	int i, maxlen = 0;
	GLint tmpi;
	GLenum tmpt;

	glGetProgramiv(sha->prog, GL_ACTIVE_UNIFORMS, &sha->unicount);
	glGetProgramiv(sha->prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);

	sha->uniforms = calloc(sizeof(Uniform), sha->unicount);

	for(i = 0; i < sha->unicount; i++) {
		sha->uniforms[i].name = malloc(maxlen);
		memset(sha->uniforms[i].name, 0, maxlen);

		glGetActiveUniform(sha->prog, i, maxlen, NULL, &tmpi, &tmpt, sha->uniforms[i].name);
		sha->uniforms[i].location = glGetUniformLocation(sha->prog, sha->uniforms[i].name);
	}
}

int uniloc(Shader *sha, char *name) {
	int i;
	for(i = 0; i < sha->unicount; i++)
		if(strcmp(sha->uniforms[i].name, name) == 0)
			return sha->uniforms[i].location;

	return -1;
}

Shader *get_shader(const char *name) {
	Shader *s, *res = NULL;

	if(config_get_int(CONFIG_NO_SHADER))
		return NULL;

	for(s = resources.shaders; s; s = s->next) {
		if(strcmp(s->name, name) == 0)
			res = s;
	}

	return res;
}

void delete_shader(void **shas, void *sha) {
	free(((Shader *)sha)->name);
	glDeleteProgram(((Shader*)sha)->prog);

	int i;
	for(i = 0; i < ((Shader *)sha)->unicount; i++)
		free(((Shader *)sha)->uniforms[i].name);

	if(((Shader *)sha)->uniforms)
		free(((Shader *)sha)->uniforms);

	delete_element(shas, sha);
}

void delete_shaders(bool transient) {
	delete_all_or_transient_elements((void **)&resources.shaders, delete_shader, transient);
}
