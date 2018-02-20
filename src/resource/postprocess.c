/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL.h>
#include "util.h"
#include "postprocess.h"
#include "resource.h"
#include "renderer.h"

ResourceHandler postprocess_res_handler = {
	.type = RES_POSTPROCESS,
	.typename = "postprocessing pipeline",
	.subdir = PP_PATH_PREFIX,

	.procs = {
		.find = postprocess_path,
		.check = check_postprocess_path,
		.begin_load = load_postprocess_begin,
		.end_load = load_postprocess_end,
		.unload = unload_postprocess,
	},
};

static PostprocessShaderUniformFuncPtr get_uniform_func(PostprocessShaderUniformType type, int size) {
	tsglUniform1fv_ptr   f_funcs[] = { glUniform1fv,  glUniform2fv,  glUniform3fv,  glUniform4fv  };
	tsglUniform1iv_ptr   i_funcs[] = { glUniform1iv,  glUniform2iv,  glUniform3iv,  glUniform4iv  };
	tsglUniform1uiv_ptr ui_funcs[] = { glUniform1uiv, glUniform2uiv, glUniform3uiv, glUniform4uiv };

	PostprocessShaderUniformFuncPtr *lists[] = {
		(PostprocessShaderUniformFuncPtr*)  f_funcs,
		(PostprocessShaderUniformFuncPtr*)  i_funcs,
		(PostprocessShaderUniformFuncPtr*) ui_funcs,
	};

	return lists[type][size - 1];
}

typedef struct PostprocessLoadData {
	PostprocessShader *list;
	ResourceFlags resflags;
} PostprocessLoadData;

static void postprocess_load_callback(const char *key, const char *value, void *data) {
	PostprocessLoadData *ldata = data;
	PostprocessShader **slist = &ldata->list;
	PostprocessShader *current = *slist;

	if(!strcmp(key, "@shader")) {
		current = malloc(sizeof(PostprocessShader));
		current->uniforms = NULL;
		current->shader = get_resource(RES_SHADER_PROGRAM, value, ldata->resflags)->data;
		list_append(slist, current);
		log_debug("Shader added: %s (prog: %u)", value, current->shader->gl_handle);
		return;
	}

	for(PostprocessShader *c = current; c; c = c->next) {
		current = c;
	}

	if(!current) {
		log_warn("Must define a @shader before key '%s'", key);
		return;
	}

	char buf[strlen(key) + 1];
	strcpy(buf, key);

	char *type = buf;
	char *name;

	int utype;
	int usize;
	int asize;
	int typesize;

	switch(*type) {
		case 'f': utype = PSU_FLOAT; typesize = sizeof(GLfloat); break;
		case 'i': utype = PSU_INT;   typesize = sizeof(GLint);   break;
		case 'u': utype = PSU_UINT;  typesize = sizeof(GLuint);  break;
		default:
			log_warn("Invalid type in key '%s'", key);
			return;
	}

	++type;
	usize = strtol(type, &type, 10);

	if(usize < 1 || usize > 4) {
		log_warn("Invalid uniform size %i in key '%s' (must be in range [1, 4])", usize, key);
		return;
	}

	asize = strtol(type, &type, 10);

	if(asize < 1) {
		log_warn("Negative uniform amount in key '%s'", key);
		return;
	}

	name = type;
	while(isspace(*name))
		++name;

	PostprocessShaderUniformValuePtr vbuf;
	vbuf.v = malloc(usize * asize * typesize);

	for(int i = 0; i < usize * asize; ++i)  {
		switch(utype) {
			case PSU_FLOAT: vbuf.f[i] = strtof  (value, (char**)&value    ); break;
			case PSU_INT:   vbuf.i[i] = strtol  (value, (char**)&value, 10); break;
			case PSU_UINT:  vbuf.u[i] = strtoul (value, (char**)&value, 10); break;
		}
	}

	PostprocessShaderUniform *uni = malloc(sizeof(PostprocessShaderUniform));
	uni->loc = uniloc(current->shader, name);
	uni->type = utype;
	uni->size = usize;
	uni->amount = asize;
	uni->values.v = vbuf.v;
	uni->func = get_uniform_func(utype, usize);
	list_append(&current->uniforms, uni);

	log_debug(
		"Uniform added: (name: %s; loc: %i; prog: %u; type: %i; size: %i; num: %i)",
		name,
		uni->loc,
		current->shader->gl_handle,
		uni->type,
		uni->size,
		uni->amount
	);

	for(int i = 0; i < uni->size * uni->amount; ++i) {
		log_debug("u[%i] = (f: %f; i: %i; u: %u)", i, uni->values.f[i], uni->values.i[i], uni->values.u[i]);
	}
}

PostprocessShader* postprocess_load(const char *path, ResourceFlags flags) {
	flags &= ~RESF_OPTIONAL;
	PostprocessLoadData *ldata = calloc(1, sizeof(PostprocessLoadData));
	ldata->resflags = flags;
	parse_keyvalue_file_cb(path, postprocess_load_callback, ldata);
	PostprocessShader *list = ldata->list;
	free(ldata);
	return list;
}

static void* delete_uniform(List **dest, List *data, void *arg) {
	PostprocessShaderUniform *uni = (PostprocessShaderUniform*)data;
	free(uni->values.v);
	free(list_unlink(dest, data));
	return NULL;
}

static void* delete_shader(List **dest, List *data, void *arg) {
	PostprocessShader *ps = (PostprocessShader*)data;
	list_foreach(&ps->uniforms, delete_uniform, NULL);
	free(list_unlink(dest, data));
	return NULL;
}

void postprocess_unload(PostprocessShader **list) {
	list_foreach(list, delete_shader, NULL);
}

void postprocess(PostprocessShader *ppshaders, FBOPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw) {
	if(!ppshaders) {
		return;
	}

	for(PostprocessShader *pps = ppshaders; pps; pps = pps->next) {
		ShaderProgram *s = pps->shader;

		glBindFramebuffer(GL_FRAMEBUFFER, fbos->back->fbo);
		glUseProgram(s->gl_handle);

		if(prepare) {
			prepare(fbos->back, s);
		}

		for(PostprocessShaderUniform *u = pps->uniforms; u; u = u->next) {
			u->func(u->loc, u->amount, u->values.v);
		}

		draw(fbos->front);
		swap_fbo_pair(fbos);
	}

	r_shader_standard();
}

/*
 *  Glue for resources api
 */

char* postprocess_path(const char *name) {
	return strjoin(PP_PATH_PREFIX, name, PP_EXTENSION, NULL);
}

bool check_postprocess_path(const char *path) {
	return strendswith(path, PP_EXTENSION);
}

void* load_postprocess_begin(const char *path, unsigned int flags) {
	return (void*)true;
}

void* load_postprocess_end(void *opaque, const char *path, unsigned int flags) {
	return postprocess_load(path, flags);
}

void unload_postprocess(void *vlist) {
	postprocess_unload((PostprocessShader**)&vlist);
}
