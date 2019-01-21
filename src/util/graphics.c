/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "graphics.h"
#include "global.h"
#include "video.h"
#include "pixmap.h"

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

void draw_fragments(const DrawFragmentsParams *params) {
	int i = 0;

	const Color *fill_clr = params->color.fill;
	const Color *back_clr = params->color.back;
	const Color *frag_clr = params->color.frag;

	if(params->alpha < 1) {
		fill_clr = color_mul_scalar(COLOR_COPY(fill_clr), params->alpha);
		frag_clr = color_mul_scalar(COLOR_COPY(frag_clr), params->alpha);
		back_clr = color_mul_scalar(COLOR_COPY(back_clr), params->alpha);
	}

	static Color prev_back_clr;

	ShaderProgram *prog_saved = r_shader_current();
	ShaderProgram *prog_wanted = r_shader_get("sprite_circleclipped_indicator");

	if(memcmp(&prev_back_clr, back_clr, sizeof(prev_back_clr))) {
		// HACK/FIXME: we can't pass more than 4 floats via custom params, and we don't have auto-flush based on uniforms state, so...
		prev_back_clr = *back_clr;
		r_flush_sprites();
	}

	r_shader_ptr(prog_wanted);
	r_uniform_vec4_rgba("back_color", back_clr);
	r_uniform_vec2_vec("origin_ofs", (float*)&params->origin_offset);

	r_flush_sprites();

	float spacing = params->spacing + params->fill->w;

	r_mat_push();
	r_mat_translate(params->pos.x - spacing, params->pos.y, 0);

	while(i < params->filled.elements) {
		r_mat_translate(spacing, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = params->fill,
			.shader_params = &(ShaderCustomParams){{ 1 }},
			.color = fill_clr,
		});
		i++;
	}

	if(params->filled.fragments) {
		r_mat_translate(spacing, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = params->fill,
			.shader_params = &(ShaderCustomParams){{ params->filled.fragments / (float)params->limits.fragments }},
			.color = frag_clr,
		});
		i++;
	}

	while(i < params->limits.elements) {
		r_mat_translate(spacing, 0, 0);
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = params->fill,
			.shader_params = &(ShaderCustomParams){{ 0 }},
			.color = frag_clr,
		});
		i++;
	}

	r_mat_pop();
	r_shader_ptr(prog_saved);
}

double draw_fraction(double value, Alignment a, double pos_x, double pos_y, Font *f_int, Font *f_fract, const Color *c_int, const Color *c_fract, bool zero_pad) {
	double val_int, val_fract;
	char buf_int[4], buf_fract[4];
	val_fract = modf(value, &val_int);

	bool kern_int = font_get_kerning_enabled(f_int);
	bool kern_fract = font_get_kerning_enabled(f_fract);

	font_set_kerning_enabled(f_int, 0);
	font_set_kerning_enabled(f_fract, 0);

	snprintf(buf_int, sizeof(buf_int), zero_pad ? "%02i" : "%i", (int)val_int);
	snprintf(buf_fract, sizeof(buf_fract), ".%02i", (int)floor(val_fract * 100));

	double w_int = text_width(f_int, buf_int, 0);
	double w_fract = text_width(f_fract, buf_fract, 0);
	double w_total = w_int + w_fract;

	switch(a) {
		case ALIGN_CENTER:
			pos_x -= w_total * 0.5;
			break;

		case ALIGN_RIGHT:
			pos_x -= w_total;
			break;

		case ALIGN_LEFT:
			break;

		default: UNREACHABLE;
	}

	double ofs_int = font_get_metrics(f_int)->descent / font_get_metrics(f_int)->scale;
	double ofs_fract = font_get_metrics(f_fract)->descent / font_get_metrics(f_fract)->scale - ofs_int;
	ofs_int = 0;

	pos_x += text_draw(buf_int, &(TextParams) {
		.pos = { pos_x, pos_y + ofs_int },
		.color = c_int,
		.font_ptr = f_int,
		.align = ALIGN_LEFT,
	});

	pos_x += text_draw(buf_fract, &(TextParams) {
		.pos = { pos_x, pos_y + ofs_fract },
		.color = c_fract,
		.font_ptr = f_fract,
		.align = ALIGN_LEFT,
	});

	font_set_kerning_enabled(f_int, kern_int);
	font_set_kerning_enabled(f_fract, kern_fract);

	return pos_x;
}

void draw_framebuffer_attachment(Framebuffer *fb, double width, double height, FramebufferAttachment attachment) {
	CullFaceMode cull_saved = r_cull_current();
	r_cull(CULL_BACK);

	r_mat_push();
	r_uniform_sampler("tex", r_framebuffer_get_attachment(fb, attachment));
	r_mat_scale(width, height, 1);
	r_mat_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_pop();

	r_cull(cull_saved);
}

void draw_framebuffer_tex(Framebuffer *fb, double width, double height) {
	draw_framebuffer_attachment(fb, width, height, FRAMEBUFFER_ATTACH_COLOR0);
}

static const char* attachment_name(FramebufferAttachment a) {
	static const char *map[FRAMEBUFFER_MAX_ATTACHMENTS] = {
		[FRAMEBUFFER_ATTACH_DEPTH] = "depth",
		[FRAMEBUFFER_ATTACH_COLOR0] = "color0",
		[FRAMEBUFFER_ATTACH_COLOR1] = "color1",
		[FRAMEBUFFER_ATTACH_COLOR2] = "color2",
		[FRAMEBUFFER_ATTACH_COLOR3] = "color3",
	};

	assert((uint)a < FRAMEBUFFER_MAX_ATTACHMENTS);
	return map[(uint)a];
}

void fbutil_create_attachments(Framebuffer *fb, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	char buf[128];

	for(uint i = 0; i < num_attachments; ++i) {
		log_debug("%i %i", attachments[i].tex_params.width, attachments[i].tex_params.height);
		Texture *tex = r_texture_create(&attachments[i].tex_params);
		snprintf(buf, sizeof(buf), "%s [%s]", r_framebuffer_get_debug_label(fb), attachment_name(attachments[i].attachment));
		r_texture_set_debug_label(tex, buf);
		r_framebuffer_attach(fb, tex, 0, attachments[i].attachment);
	}
}

void fbutil_destroy_attachments(Framebuffer *fb) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		Texture *tex = r_framebuffer_get_attachment(fb, i);

		if(tex != NULL) {
			r_texture_destroy(tex);
		}
	}
}

void fbutil_resize_attachment(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height) {
	Texture *tex = r_framebuffer_get_attachment(fb, attachment);

	if(tex == NULL) {
		return;
	}

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	if(tw == width && th == height) {
		return;
	}

	// TODO: We could render a rescaled version of the old texture contents here

	TextureParams params;
	r_texture_get_params(tex, &params);
	r_texture_destroy(tex);
	params.width = width;
	params.height = height;
	params.mipmaps = 0; // FIXME
	r_framebuffer_attach(fb, r_texture_create(&params), 0, attachment);
}
