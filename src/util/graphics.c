/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "graphics.h"
#include "global.h"
#include "video.h"

void set_ortho(float w, float h) {
	r_mat_mode(MM_PROJECTION);
	r_mat_ortho(0, w, h, 0, -100, 100);
	r_mat_mode(MM_MODELVIEW);

	// FIXME: should we take this out of here and call it explicitly instead?
	r_disable(RCAP_DEPTH_TEST);
}

void colorfill(float r, float g, float b, float a) {
	if(r <= 0 && g <= 0 && b <= 0 && a <= 0) {
		return;
	}

	r_shader_standard_notex();
	r_color4(r,g,b,a);

	r_mat_push();
	r_mat_scale(SCREEN_W,SCREEN_H,1);
	r_mat_translate(0.5,0.5,0);

	r_draw_quad();
	r_mat_pop();

	r_color4(1,1,1,1);
	r_shader_standard();
}

void fade_out(float f) {
	colorfill(0, 0, 0, f);
}

void draw_stars(int x, int y, int numstars, int numfrags, int maxstars, int maxfrags, float alpha, float star_width) {
	Sprite *star = get_sprite("star");
	int i = 0;
	float scale = star_width/star->w;

	Color *fill_clr = color_mul_scalar(RGBA(1.00, 1.00, 1.00, 1.00), alpha);
	Color *back_clr = color_mul_scalar(RGBA(0.04, 0.12, 0.20, 0.20), alpha);
	Color *frag_clr = color_mul_scalar(RGBA(0.47, 0.56, 0.65, 0.65), alpha);

	// XXX: move to call site?
	y -= 2;

	ShaderProgram *prog_saved = r_shader_current();
	r_shader("sprite_circleclipped_indicator");
	r_uniform_rgba("back_color", back_clr);

	r_mat_push();
	r_mat_translate(x - star_width, y, 0);

	while(i < numstars) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.shader_params = &(ShaderCustomParams){{ 1 }},
			.color = fill_clr,
			.scale.both = scale,
		});
		i++;
	}

	if(numfrags) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.shader_params = &(ShaderCustomParams){{ numfrags / (float)maxfrags }},
			.color = frag_clr,
			.scale.both = scale,
		});
		i++;
	}

	while(i < maxstars) {
		r_mat_translate(star_width, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = star,
			.shader_params = &(ShaderCustomParams){{ 0 }},
			.color = frag_clr,
			.scale.both = scale,
		});
		i++;
	}

	r_mat_pop();
	r_shader_ptr(prog_saved);
}

void draw_framebuffer_tex(Framebuffer *fb, double width, double height) {
	CullFaceMode cull_saved = r_cull_current();
	r_cull(CULL_FRONT);

	r_mat_push();
	r_texture_ptr(0, r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_COLOR0));
	r_mat_scale(width, height, 1);
	r_mat_translate(0.5, 0.5, 0);
	r_mat_scale(1, -1, 1);
	r_draw_quad();
	r_mat_pop();

	r_cull(cull_saved);
}
