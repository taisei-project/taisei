/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "texture_loader/texture_loader.h"

#include "global.h"
#include "video.h"
#include "renderer/api.h"

static bool texture_transfer(void *dst, void *src) {
	return r_texture_transfer(dst, src);
}

ResourceHandler texture_res_handler = {
	.type = RES_TEXTURE,
	.typename = "texture",
	.subdir = TEX_PATH_PREFIX,

	.procs = {
		.find = texture_loader_path,
		.check = texture_loader_check_path,
		.load = texture_loader_stage1,
		.unload = texture_loader_unload,
		.transfer = texture_transfer,
	},
};

static struct draw_texture_state {
	bool drawing;
	bool texture_matrix_tainted;
} draw_texture_state;

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex) {
	assume(!draw_texture_state.drawing);
	draw_texture_state.drawing = true;

	r_uniform_sampler("tex", tex);
	r_mat_mv_push();

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	float x = dest.x;
	float y = dest.y;
	float w = dest.w;
	float h = dest.h;

	float s = frag.w/tw;
	float t = frag.h/th;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: please somehow abstract this shit away!
		frag.y = th - frag.y - frag.h;
	}

	if(s != 1 || t != 1 || frag.x || frag.y) {
		draw_texture_state.texture_matrix_tainted = true;

		r_mat_tex_push();

		r_mat_tex_scale(1.0/tw, 1.0/th, 1);

		if(frag.x || frag.y) {
			r_mat_tex_translate(frag.x, frag.y, 0);
		}

		if(s != 1 || t != 1) {
			r_mat_tex_scale(frag.w, frag.h, 1);
		}
	}

	if(x || y) {
		r_mat_mv_translate(x, y, 0);
	}

	if(w != 1 || h != 1) {
		r_mat_mv_scale(w, h, 1);
	}
}

void end_draw_texture(void) {
	assume(draw_texture_state.drawing);

	if(draw_texture_state.texture_matrix_tainted) {
		draw_texture_state.texture_matrix_tainted = false;
		r_mat_tex_pop();
	}

	r_mat_mv_pop();
	draw_texture_state.drawing = false;
}

void fill_viewport(float xoff, float yoff, float ratio, const char *name) {
	fill_viewport_p(xoff, yoff, ratio, 1, 0, res_texture(name));
}

void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex) {
	r_uniform_sampler("tex", tex);

	float rw, rh;

	if(ratio == 0) {
		rw = aspect;
		rh = 1;
	} else {
		rw = ratio * aspect;
		rh = ratio;
	}

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: we should somehow account for this globally if possible...
		yoff *= -1;
		yoff += (1 - ratio);
	}

	bool texture_matrix_tainted = false;

	if(xoff || yoff || rw != 1 || rh != 1 || angle) {
		texture_matrix_tainted = true;

		r_mat_tex_push();

		if(xoff || yoff) {
			r_mat_tex_translate(xoff, yoff, 0);
		}

		if(rw != 1 || rh != 1) {
			r_mat_tex_scale(rw, rh, 1);
		}

		if(angle) {
			r_mat_tex_translate(0.5, 0.5, 0);
			r_mat_tex_rotate(angle * DEG2RAD, 0, 0, 1);
			r_mat_tex_translate(-0.5, -0.5, 0);
		}
	}

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_W*0.5, VIEWPORT_H*0.5, 0);
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_draw_quad();
	r_mat_mv_pop();

	if(texture_matrix_tainted) {
		r_mat_tex_pop();
	}
}

void fill_screen(const char *name) {
	fill_screen_p(res_texture(name));
}

void fill_screen_p(Texture *tex) {
	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);
	begin_draw_texture((FloatRect){ SCREEN_W*0.5, SCREEN_H*0.5, SCREEN_W, SCREEN_H }, (FloatRect){ 0, 0, tw, th }, tex);
	r_draw_quad();
	end_draw_texture();
}

// draws a thin, w-width rectangle from point A to point B with a texture that
// moves along the line.
//
void loop_tex_line_p(cmplx a, cmplx b, float w, float t, Texture *texture) {
	cmplx d = b-a;
	cmplx c = (b+a)/2;

	r_mat_mv_push();
	r_mat_mv_translate(re(c), im(c), 0);
	r_mat_mv_rotate(carg(d), 0, 0, 1);
	r_mat_mv_scale(cabs(d), w, 1);

	r_mat_tex_push();
	r_mat_tex_translate(t, 0, 0);

	r_uniform_sampler("tex", texture);
	r_draw_quad();

	r_mat_tex_pop();
	r_mat_mv_pop();
}

void loop_tex_line(cmplx a, cmplx b, float w, float t, const char *texture) {
	loop_tex_line_p(a, b, w, t, res_texture(texture));
}
