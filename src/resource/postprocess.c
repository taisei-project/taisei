/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <SDL.h>
#include "util.h"
#include "postprocess.h"
#include "resource.h"

static PostprocessShaderUniformFuncPtr get_uniform_func(PostprocessShaderUniformType type, int size) {
    tsglUniform1fv_ptr   f_funcs[] = { glUniform1fv,  glUniform2fv,  glUniform3fv,  glUniform4fv };
    tsglUniform1iv_ptr   i_funcs[] = { glUniform1iv,  glUniform2iv,  glUniform3iv,  glUniform4iv };
    tsglUniform1uiv_ptr ui_funcs[] = { glUniform1uiv, glUniform2uiv, glUniform3uiv, glUniform4uiv };

    PostprocessShaderUniformFuncPtr *lists[] = {
        (PostprocessShaderUniformFuncPtr*)  f_funcs,
        (PostprocessShaderUniformFuncPtr*)  i_funcs,
        (PostprocessShaderUniformFuncPtr*) ui_funcs,
    };

    return lists[type][size - 1];
}

static void postprocess_load_callback(const char *key, const char *value, void *data) {
    PostprocessShader **slist = data;
    PostprocessShader *current = *slist;

    if(!strcmp(key, "@shader")) {
        current = create_element((void**)slist, sizeof(PostprocessShader));
        current->uniforms = NULL;
        current->shader = get_resource(RES_SHADER, value, RESF_PRELOAD | RESF_PERMANENT)->shader;
        log_debug("Shader added: %s (prog: %u)", value, current->shader->prog);
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

    PostprocessShaderUniform *uni = create_element((void**)&current->uniforms, sizeof(PostprocessShaderUniform));
    uni->loc = uniloc(current->shader, name);
    uni->type = utype;
    uni->size = usize;
    uni->amount = asize;
    uni->values.v = vbuf.v;
    uni->func = get_uniform_func(utype, usize);

    log_debug("Uniform added: (name: %s; loc: %i; prog: %u; type: %i; size: %i; num: %i)", name, uni->loc, current->shader->prog, uni->type, uni->size, uni->amount);

    for(int i = 0; i < uni->size * uni->amount; ++i) {
        log_debug("u[%i] = (f: %f; i: %i; u: %u)", i, uni->values.f[i], uni->values.i[i], uni->values.u[i]);
    }
}

PostprocessShader* postprocess_load(const char *path) {
    PostprocessShader *list = NULL;
    parse_keyvalue_file_cb(path, postprocess_load_callback, &list);
    return list;
}

static void delete_uniform(void **dest, void *data) {
    PostprocessShaderUniform *uni = data;
    free(uni->values.v);
    delete_element(dest, data);
}

static void delete_shader(void **dest, void *data) {
    PostprocessShader *ps = data;
    delete_all_elements((void**)&ps->uniforms, delete_uniform);
    delete_element(dest, data);
}

void postprocess_unload(PostprocessShader **list) {
    delete_all_elements((void**)list, delete_shader);
}

FBO* postprocess(PostprocessShader *ppshaders, FBO *primfbo, FBO *auxfbo, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw) {
    FBO *fbos[] = { primfbo, auxfbo };

    if(!ppshaders) {
        return primfbo;
    }

    int fbonum = 1;
    for(PostprocessShader *pps = ppshaders; pps; pps = pps->next) {
        Shader *s = pps->shader;

        glBindFramebuffer(GL_FRAMEBUFFER, fbos[fbonum]->fbo);
        glUseProgram(s->prog);
        prepare(fbos[fbonum], s);

        for(PostprocessShaderUniform *u = pps->uniforms; u; u = u->next) {
            u->func(u->loc, u->amount, u->values.v);
        }

        fbonum = !fbonum;
        draw(fbos[fbonum]);
        glUseProgram(0);
    }

    fbonum = !fbonum;
    return fbos[fbonum];
}
