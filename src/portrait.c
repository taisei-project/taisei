/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "portrait.h"
#include "renderer/api.h"
#include "config.h"

#define RETURN_RESOURCE_NAME(name1, suffix, name2) \
	assert(bufsize >= strlen(PORTRAIT_PREFIX) + strlen(name1) + strlen(suffix) + strlen(name2) + 1); \
	return snprintf(buf, bufsize, PORTRAIT_PREFIX "%s" suffix "%s", name1, name2)

#define BUFFER_SIZE 128

int portrait_get_base_sprite_name(const char *charname, const char *variant, size_t bufsize, char buf[bufsize]) {
	if(variant == NULL) {
		RETURN_RESOURCE_NAME(charname, "", "");
	} else {
		RETURN_RESOURCE_NAME(charname, PORTRAIT_VARIANT_SUFFIX, variant);
	}
}

Sprite *portrait_get_base_sprite(const char *charname, const char *variant) {
	char buf[BUFFER_SIZE];
	portrait_get_base_sprite_name(charname, variant, sizeof(buf), buf);
	return res_sprite(buf);
}

void portrait_preload_base_sprite(const char *charname, const char *variant, ResourceFlags rflags) {
	char buf[BUFFER_SIZE];
	portrait_get_base_sprite_name(charname, variant, sizeof(buf), buf);
	preload_resource(RES_SPRITE, buf, rflags);
}

int portrait_get_face_sprite_name(const char *charname, const char *face, size_t bufsize, char buf[bufsize]) {
	RETURN_RESOURCE_NAME(charname, PORTRAIT_FACE_SUFFIX, face);
}

Sprite *portrait_get_face_sprite(const char *charname, const char *face) {
	char buf[BUFFER_SIZE];
	portrait_get_face_sprite_name(charname, face, sizeof(buf), buf);
	return res_sprite(buf);
}

void portrait_preload_face_sprite(const char *charname, const char *face, ResourceFlags rflags) {
	char buf[BUFFER_SIZE];
	portrait_get_face_sprite_name(charname, face, sizeof(buf), buf);
	preload_resource(RES_SPRITE, buf, rflags);
}

void portrait_render(Sprite *s_base, Sprite *s_face, Sprite *s_out) {
	r_state_push();

	IntRect itc = sprite_denormalized_int_tex_coords(s_base);

	uint tex_w = imax(itc.w, 1);
	uint tex_h = imax(itc.h, 1);
	float spr_w = s_base->extent.w;
	float spr_h = s_base->extent.h;

	Texture *ptex = r_texture_create(&(TextureParams) {
		.type = TEX_TYPE_RGBA_8,
		.width = tex_w,
		.height = tex_h,
		.filter.min = TEX_FILTER_LINEAR_MIPMAP_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_CLAMP,
		.wrap.t = TEX_WRAP_CLAMP,
		.mipmap_mode = TEX_MIPMAP_AUTO,
		.mipmaps = 3,
	});

	Framebuffer *fb = r_framebuffer_create();
	r_framebuffer_attach(fb, ptex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(fb, 0, 0, tex_w, tex_h);
	r_framebuffer(fb);
	r_framebuffer_clear(fb, CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);

	r_mat_proj_push_ortho(spr_w - s_base->padding.w, spr_h - s_base->padding.h);
	r_mat_mv_push_identity();

	SpriteParams sp = { 0 };
	sp.sprite_ptr = s_base;
	sp.blend = BLEND_NONE;
	sp.pos.x = spr_w * 0.5f - s_base->padding.offset.x;
	sp.pos.y = spr_h * 0.5f - s_base->padding.offset.y;
	sp.color = RGBA(1, 1, 1, 1);
	sp.shader_ptr = res_shader("sprite_default"),
	r_draw_sprite(&sp);
	sp.blend = BLEND_PREMUL_ALPHA;
	sp.sprite_ptr = s_face;
	r_draw_sprite(&sp);
	r_flush_sprites();

	r_mat_mv_pop();
	r_mat_proj_pop();
	r_state_pop();
	r_framebuffer_destroy(fb);

	Sprite s = { 0 };
	s.tex = ptex;
	s.extent = s_base->extent;
	s.padding = s_base->padding;
	s.tex_area.w = 1.0f;
	s.tex_area.h = 1.0f;
	*s_out = s;
}

void portrait_render_byname(const char *charname, const char *variant, const char *face, Sprite *s_out) {
	portrait_render(
		portrait_get_base_sprite(charname, variant),
		portrait_get_face_sprite(charname, face),
		s_out
	);
}
