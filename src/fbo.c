/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "fbo.h"
#include "global.h"
#include "util.h"

// TODO: rename and move this
Resources resources;

static void init_fbo(FBO *fbo, uint width, uint height, TextureType type) {
	Texture *rgba = calloc(1, sizeof(Texture));
	Texture *depth = calloc(1, sizeof(Texture));

	r_texture_create(rgba, &(TextureParams) {
		.width  = width,
		.height = height,
		.type = type,
	});

	r_texture_create(depth, &(TextureParams) {
		.width  = width,
		.height = height,
		.type = TEX_TYPE_DEPTH,
	});

	r_target_create(fbo);
	r_target_attach(fbo, rgba, RENDERTARGET_ATTACHMENT_COLOR0);
	r_target_attach(fbo, depth, RENDERTARGET_ATTACHMENT_DEPTH);

	log_debug("FBO %p: w=%i, h=%i", (void*)fbo, width, height);
}

static void delete_tex(Texture *tex) {
	r_texture_destroy(tex);
	free(tex);
}

static void delete_fbo(FBO *fbo) {
	delete_tex(r_target_get_attachment(fbo, RENDERTARGET_ATTACHMENT_COLOR0));
	delete_tex(r_target_get_attachment(fbo, RENDERTARGET_ATTACHMENT_DEPTH));
	r_target_destroy(fbo);
}

static void reinit_fbo(FBO *fbo, float scale, TextureType type) {
	scale = sanitize_scale(scale);
	uint w = VIEWPORT_W * scale;
	uint h = VIEWPORT_H * scale;

	Texture *rgba;

	if(fbo->impl) {
		rgba = r_target_get_attachment(fbo, RENDERTARGET_ATTACHMENT_COLOR0);
	} else {
		rgba = NULL;
	}

	if(!rgba) {
		init_fbo(fbo, w, h, type);
	} else if(w != rgba->w || h != rgba->h) {
		delete_fbo(fbo);
		init_fbo(fbo, w, h, type);
	}
}

static void swap_fbos(FBO **fbo1, FBO **fbo2) {
	FBO *temp = *fbo1;
	*fbo1 = *fbo2;
	*fbo2 = temp;
}

void init_fbo_pair(FBOPair *pair, float scale, TextureType type) {
	pair->front = pair->_private.targets+0;
	pair->back  = pair->_private.targets+1;

	reinit_fbo(pair->front, scale, type);
	reinit_fbo(pair->back, scale, type);
}

void delete_fbo_pair(FBOPair *pair) {
	delete_fbo(pair->front);
	delete_fbo(pair->back);
}

void swap_fbo_pair(FBOPair *pair) {
	swap_fbos(&pair->front, &pair->back);
}

void draw_fbo(FBO *fbo) {
	CullFaceMode cull_saved = r_cull_current();
	r_cull(CULL_FRONT);

	r_mat_push();
	r_texture_ptr(0, r_target_get_attachment(fbo, RENDERTARGET_ATTACHMENT_COLOR0));
	r_mat_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_mat_translate(0.5, 0.5, 0);
	r_mat_scale(1, -1, 1);
	r_draw_quad();
	r_mat_pop();

	r_cull(cull_saved);
}

void draw_fbo_viewport(FBO *fbo) {
	// assumption: rendering into another, identical FBO

	// r_viewport(0, 0, fbo->scale*VIEWPORT_W, fbo->scale*VIEWPORT_H);
	set_ortho_ex(VIEWPORT_W, VIEWPORT_H);
	draw_fbo(fbo);
}
