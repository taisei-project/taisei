/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "recolor.h"
#include "resource/resource.h"
#include "renderer/api.h"

ColorTransform colortransform_identity = {
	.R[1] = RGBA(1, 0, 0, 0),
	.G[1] = RGBA(0, 1, 0, 0),
	.B[1] = RGBA(0, 0, 1, 0),
	.A[1] = RGBA(0, 0, 0, 1),
};

struct recolor_varcache {
	Color prev;
	Uniform *uni;
};

static struct recolor_vars_s {
	ShaderProgram *shader;
	struct recolor_varcache R;
	struct recolor_varcache G;
	struct recolor_varcache B;
	struct recolor_varcache A;
	struct recolor_varcache O;
	bool loaded;
	int transfers;
} recolor_vars;

static inline void recolor_set_uniform(struct recolor_varcache *vc, Color clr) {
	static float clrarr[4];

	if(vc->prev != clr) {
		parse_color_array(clr, clrarr);
		r_uniform_ptr(vc->uni, 1, clrarr);
		vc->prev = clr;
		// log_debug("%i", ++recolor_vars.transfers);
	}
}

void recolor_init(void) {
	if(recolor_vars.loaded) {
		return;
	}

	preload_resource(RES_SHADER_PROGRAM, "recolor", RESF_PERMANENT);

	recolor_vars.shader = r_shader_get("recolor");
	recolor_vars.R.uni = r_shader_uniform(recolor_vars.shader, "R");
	recolor_vars.G.uni = r_shader_uniform(recolor_vars.shader, "G");
	recolor_vars.B.uni = r_shader_uniform(recolor_vars.shader, "B");
	recolor_vars.A.uni = r_shader_uniform(recolor_vars.shader, "A");
	recolor_vars.O.uni = r_shader_uniform(recolor_vars.shader, "O");

	ShaderProgram *prev_prog = r_shader_current();
	r_shader_ptr(recolor_vars.shader);
	assert(recolor_vars.shader != NULL);
	assert(r_shader_current() == recolor_vars.shader);
	recolor_apply_transform(&colortransform_identity);
	r_shader_ptr(prev_prog);
}

void recolor_reinit(void) {
	recolor_vars.loaded = false;
	recolor_init();
}

ShaderProgram* recolor_get_shader(void) {
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
