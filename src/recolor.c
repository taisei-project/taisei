/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "recolor.h"
#include "resource/resource.h"

ColorTransform colortransform_identity = {
    .R[1] = RGBA(1, 0, 0, 0),
    .G[1] = RGBA(0, 1, 0, 0),
    .B[1] = RGBA(0, 0, 1, 0),
    .A[1] = RGBA(0, 0, 0, 1),

    .min = RGBA(0, 0, 0, 0),
    .max = RGBA(1, 1, 1, 1),
};

struct recolor_varcache {
    Color prev;
    int loc;
};

static struct recolor_vars_s {
    Shader *shader;
    struct recolor_varcache R[2];
    struct recolor_varcache G[2];
    struct recolor_varcache B[2];
    struct recolor_varcache A[2];
    // struct recolor_varcache min;
    // struct recolor_varcache max;
    bool loaded;
} recolor_vars;

static inline void recolor_set_uniform(struct recolor_varcache *vc, Color clr) {
    static float clrarr[4];

    if(vc->prev != clr) {
        parse_color_array(clr, clrarr);
        glUniform4fv(vc->loc, 1, clrarr);
        vc->prev = clr;
    }
}

void recolor_init(void) {
    if(recolor_vars.loaded) {
        return;
    }

    preload_resource(RES_SHADER, "recolor", RESF_PERMANENT);

    recolor_vars.shader = get_shader("recolor");
    recolor_vars.R[0].loc = uniloc(recolor_vars.shader, "R0");
    recolor_vars.G[0].loc = uniloc(recolor_vars.shader, "G0");
    recolor_vars.B[0].loc = uniloc(recolor_vars.shader, "B0");
    recolor_vars.A[0].loc = uniloc(recolor_vars.shader, "A0");
    recolor_vars.R[1].loc = uniloc(recolor_vars.shader, "R1");
    recolor_vars.G[1].loc = uniloc(recolor_vars.shader, "G1");
    recolor_vars.B[1].loc = uniloc(recolor_vars.shader, "B1");
    recolor_vars.A[1].loc = uniloc(recolor_vars.shader, "A1");
    // recolor_vars.min.loc = uniloc(recolor_vars.shader, "Cmin");
    // recolor_vars.max.loc = uniloc(recolor_vars.shader, "Cmax");

    int prev_prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_prog);
    glUseProgram(recolor_vars.shader->prog);
    recolor_apply_transform(&colortransform_identity);
    glUseProgram(prev_prog);
}

void recolor_reinit(void) {
    recolor_vars.loaded = false;
    recolor_init();
}

Shader* recolor_get_shader(void) {
    return recolor_vars.shader;
}

void recolor_apply_transform(ColorTransform *ct) {
    for(int i = 0; i < 2; ++i) {
        recolor_set_uniform(&recolor_vars.R[i], ct->R[i]);
        recolor_set_uniform(&recolor_vars.G[i], ct->G[i]);
        recolor_set_uniform(&recolor_vars.B[i], ct->B[i]);
        recolor_set_uniform(&recolor_vars.A[i], ct->A[i]);
    }

    // recolor_set_uniform(&recolor_vars.min, ct->min);
    // recolor_set_uniform(&recolor_vars.max, ct->max);
}
