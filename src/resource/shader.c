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

char* shader_path(const char *name) {
	return strjoin(SHA_PATH_PREFIX, name, SHA_EXTENSION, NULL);
}

bool check_shader_path(const char *path) {
	return strendswith(path, SHA_EXTENSION);
}

static Shader* load_shader(const char *vheader, const char *fheader, const char *vtext, const char *ftext);

typedef struct ShaderLoadData {
	char *text;
	char *vtext;
	char *ftext;
} ShaderLoadData;

void* load_shader_begin(const char *path, unsigned int flags) {
	char *text, *vtext, *ftext, *delim;

	text = read_all(path, NULL);

	if(!text) {
		return NULL;
	}

	vtext = text;
	delim = strstr(text, SHA_DELIM);

	if(delim == NULL) {
		log_warn("Expected '%s' delimiter.", SHA_DELIM);
		free(text);
		return NULL;
	}

	*delim = 0;
	ftext = delim + SHA_DELIM_SIZE;

	ShaderLoadData *data = malloc(sizeof(ShaderLoadData));
	data->text = text;
	data->vtext = vtext;
	data->ftext = ftext;

	return data;
}

void* load_shader_end(void *opaque, const char *path, unsigned int flags) {
	ShaderLoadData *data = opaque;

	if(!data) {
		return NULL;
	}

	Shader *sha = load_shader(NULL, NULL, data->vtext, data->ftext);

	free(data->text);
	free(data);

	return sha;
}

void unload_shader(void *vsha) {
	Shader *sha = vsha;
	glDeleteProgram((sha)->prog);
	hashtable_free(sha->uniforms);
	free(sha);
}

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

void load_shader_snippets(const char *filename, const char *prefix, unsigned int flags) {
	int size, vhsize = 0, vfsize = 0, fhsize = 0, ffsize = 0, ssize, prefixlen;
	char *text, *vhead, *vfoot, *fhead, *ffoot;
	char *sec, *name, *nend, *send;
	char *vtext = NULL, *ftext = NULL;
	char *nbuf;

	log_info("Loading snippet file '%s' with prefix '%s'", filename, prefix);

	prefixlen = strlen(prefix);

	text = read_all(filename, &size);

	if(!text) {
		return;
	}

	sec = text;

	vhead = copy_segment(text, "%%VSHADER-HEAD%%", &vhsize);
	vfoot = copy_segment(text, "%%VSHADER-FOOT%%", &vfsize);

	fhead = copy_segment(text, "%%FSHADER-HEAD%%", &fhsize);
	ffoot = copy_segment(text, "%%FSHADER-FOOT%%", &ffsize);

	if(!vhead || !fhead)
		log_fatal("Syntax Error: missing HEAD section(s)");
	if((vfoot == NULL) + (ffoot == NULL) != 1)
		log_fatal("Syntax Error: must contain exactly 1 FOOT section");

	while((sec = strstr(sec, "%%"))) {
		sec += 2;

		name = sec;
		nend = strstr(name, "%%");
		if(!nend)
			log_fatal("Syntax Error: expected '%%'");

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

		Shader *sha = load_shader(get_snippet_header(), NULL, vtext, ftext);
		insert_resource(RES_SHADER, nbuf, sha, flags, filename);

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

static void print_info_log(GLuint shader, tsglGetShaderiv_ptr lenfunc, tsglGetShaderInfoLog_ptr logfunc, const char *type) {
	int len = 0, alen = 0;
	lenfunc(shader, GL_INFO_LOG_LENGTH, &len);

	if(len > 1) {
		char log[len];
		memset(log, 0, len);
		logfunc(shader, len, &alen, log);

		log_warn("\n == GLSL %s info log (%u) ==\n%s\n == End of GLSL %s info log (%u) ==",
					type, shader, log, type, shader);
	}
}

static void cache_uniforms(Shader *sha) {
	int i, maxlen = 0;
	GLint tmpi;
	GLenum tmpt;
	GLint unicount;

	sha->uniforms = hashtable_new_stringkeys(13);

	glGetProgramiv(sha->prog, GL_ACTIVE_UNIFORMS, &unicount);
	glGetProgramiv(sha->prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);

	if(maxlen < 1) {
		return;
	}

	char name[maxlen];

	for(i = 0; i < unicount; i++) {
		glGetActiveUniform(sha->prog, i, maxlen, NULL, &tmpi, &tmpt, name);
		// This +1 increment kills two youkai with one spellcard.
		// 1. We can't store 0 in the hashtable, because that's the NULL/nonexistent value.
		//    But 0 is a valid uniform location, so we need to store that in some way.
		// 2. glGetUniformLocation returns -1 for builtin uniforms, which we don't want to cache anyway.
		hashtable_set_string(sha->uniforms, name, (void*)(intptr_t)(glGetUniformLocation(sha->prog, name) + 1));
	}

#ifdef DEBUG_GL
	// hashtable_print_stringkeys(sha->uniforms);
#endif
}

static Shader* load_shader(const char *vheader, const char *fheader, const char *vtext, const char *ftext) {
	Shader *sha = malloc(sizeof(Shader));
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

	const GLchar *v_sources[] = { vheader, vtext };
	const GLchar *f_sources[] = { fheader, ftext };
	GLint lengths[] = { -1, -1 };

	glShaderSource(vshaderobj, 2, (const GLchar**)v_sources, lengths);
	glShaderSource(fshaderobj, 2, (const GLchar**)f_sources, lengths);

	glCompileShader(vshaderobj);
	glCompileShader(fshaderobj);

	print_info_log(vshaderobj, glGetShaderiv, glGetShaderInfoLog, "Vertex Shader");
	print_info_log(fshaderobj, glGetShaderiv, glGetShaderInfoLog, "Fragment Shader");

	glAttachShader(sha->prog, vshaderobj);
	glAttachShader(sha->prog, fshaderobj);

	glDeleteShader(vshaderobj);
	glDeleteShader(fshaderobj);

	glLinkProgram(sha->prog);

	print_info_log(sha->prog, glGetProgramiv, glGetProgramInfoLog, "Program");

	cache_uniforms(sha);

	return sha;
}

int uniloc(Shader *sha, const char *name) {
	return (intptr_t)hashtable_get_string(sha->uniforms, name) - 1;
}

Shader* get_shader(const char *name) {
	return get_resource(RES_SHADER, name, RESF_DEFAULT)->shader;
}
