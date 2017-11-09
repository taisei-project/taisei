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
};

struct recolor_varcache {
    Color prev;
    int loc;
};

static struct recolor_vars_s {
    Shader *shader;
    struct recolor_varcache R;
    struct recolor_varcache G;
    struct recolor_varcache B;
    struct recolor_varcache A;
    struct recolor_varcache O;
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
    recolor_vars.R.loc = uniloc(recolor_vars.shader, "R");
    recolor_vars.G.loc = uniloc(recolor_vars.shader, "G");
    recolor_vars.B.loc = uniloc(recolor_vars.shader, "B");
    recolor_vars.A.loc = uniloc(recolor_vars.shader, "A");
    recolor_vars.O.loc = uniloc(recolor_vars.shader, "O");

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
    recolor_set_uniform(&recolor_vars.R, subtract_colors(ct->R[1], ct->R[0]));
    recolor_set_uniform(&recolor_vars.G, subtract_colors(ct->G[1], ct->G[0]));
    recolor_set_uniform(&recolor_vars.B, subtract_colors(ct->B[1], ct->B[0]));
    recolor_set_uniform(&recolor_vars.A, subtract_colors(ct->A[1], ct->A[0]));

    float accum[4] = { 0 };
    static float tmp[4] = { 0 };

    for(int i = 0; i < 4; ++i) {
        parse_color_array(ct->pairs[i].low, tmp);
        for(int j = 0; j < 4; ++j) {
            accum[j] += tmp[j];
        }
    }

    recolor_set_uniform(&recolor_vars.O, rgba(accum[0], accum[1], accum[2], accum[3]));
}
