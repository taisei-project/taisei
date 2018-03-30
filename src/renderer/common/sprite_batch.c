/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "sprite_batch.h"
#include "../api.h"
#include "glm.h"

#define SPRITE_BATCH_CAPACITY 2048

typedef struct Quad2DVertex {
	float pos[2];
} Quad2DVertex;

typedef struct Sprite2DAttrs {
	mat4 transform;
	float rgba[4];
	float texrect[4];
	float custom;
	uint8_t flip[2];
} Sprite2DAttrs;

static struct SpriteBatchState {
	VertexBuffer vbuf;
	Quad2DVertex quad_2d[4];
	Texture *tex;
	ShaderProgram *shader;
	BlendMode blend;
	RenderTarget *render_target;
	CullFaceMode cull_mode;
	DepthTestFunc depth_func;
	uint cull_enabled : 1;
	uint depth_test_enabled : 1;
	uint depth_write_enabled : 1;
	uint num_pending;
} _r_sprite_batch = {
	.quad_2d = {
		{ { -0.5, -0.5 } },
		{ { -0.5,  0.5 } },
		{ {  0.5,  0.5 } },
		{ {  0.5, -0.5 } },
	}
};

static void _r_sprite_batch_reset_buffer(void) {
	r_vertex_buffer_invalidate(&_r_sprite_batch.vbuf);
	r_vertex_buffer_append(&_r_sprite_batch.vbuf, sizeof(_r_sprite_batch.quad_2d), &_r_sprite_batch.quad_2d);
}

void _r_sprite_batch_init(void) {
	size_t sz_vert = sizeof(Quad2DVertex);
	size_t sz_attr = sizeof(Sprite2DAttrs);
	size_t sz_quad = sizeof(_r_sprite_batch.quad_2d);

	#define QUAD_OFS(attr)     offsetof(Quad2DVertex,  attr)
	#define INSTANCE_OFS(attr) offsetof(Sprite2DAttrs, attr) + sz_quad

	VertexAttribFormat fmt[] = {
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT,            0 }, sz_vert, QUAD_OFS(pos)              },

		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[0]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[1]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[2]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(transform[3]) },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(rgba)         },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(texrect)      },
		{ { 1, VA_FLOAT, VA_CONVERT_FLOAT,            1 }, sz_attr, INSTANCE_OFS(custom)       },
		{ { 2, VA_UBYTE, VA_CONVERT_FLOAT_NORMALIZED, 1 }, sz_attr, INSTANCE_OFS(flip)         },
	};

	#undef QUAD_OFS
	#undef INSTANCE_OFS

	r_vertex_buffer_create(
		&_r_sprite_batch.vbuf,
		sizeof(_r_sprite_batch.quad_2d) + SPRITE_BATCH_CAPACITY * sizeof(Sprite2DAttrs),
		sizeof(fmt)/sizeof(*fmt),
		fmt
	);
	_r_sprite_batch_reset_buffer();
}

void _r_sprite_batch_shutdown(void) {
	r_vertex_buffer_destroy(&_r_sprite_batch.vbuf);
}

void r_flush_sprites(void) {
	if(_r_sprite_batch.num_pending == 0) {
		return;
	}

	uint pending = _r_sprite_batch.num_pending;

	// needs to be done early to thwart recursive callss
	_r_sprite_batch.num_pending = 0;

	// log_warn("flush! %u", pending);

	VertexBuffer *vbuf_saved = r_vertex_buffer_current();
	Texture *tex_saved = r_texture_current(0);
	ShaderProgram *prog_saved = r_shader_current();
	RenderTarget *target_saved = r_target_current();
	BlendMode blend_saved = r_blend_current();
	bool cap_deptp_test_saved = r_capability_current(RCAP_DEPTH_TEST);
	bool cap_depth_write_saved = r_capability_current(RCAP_DEPTH_WRITE);
	bool cap_cull_saved = r_capability_current(RCAP_CULL_FACE);
	DepthTestFunc depth_func_saved = r_depth_func_current();
	CullFaceMode cull_mode_saved = r_cull_current();

	r_vertex_buffer(&_r_sprite_batch.vbuf);
	r_texture_ptr(0, _r_sprite_batch.tex);
	r_shader_ptr(_r_sprite_batch.shader);
	r_target(_r_sprite_batch.render_target);
	r_blend(_r_sprite_batch.blend);
	r_capability(RCAP_DEPTH_TEST, _r_sprite_batch.depth_test_enabled);
	r_capability(RCAP_DEPTH_WRITE, _r_sprite_batch.depth_write_enabled);
	r_capability(RCAP_CULL_FACE, _r_sprite_batch.cull_enabled);
	r_depth_func(_r_sprite_batch.depth_func);
	r_cull(_r_sprite_batch.cull_mode);

	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, pending);
	_r_sprite_batch_reset_buffer();

	r_vertex_buffer(vbuf_saved);
	r_texture_ptr(0, tex_saved);
	r_shader_ptr(prog_saved);
	r_target(target_saved);
	r_blend(blend_saved);
	r_capability(RCAP_DEPTH_TEST, cap_deptp_test_saved);
	r_capability(RCAP_DEPTH_WRITE, cap_depth_write_saved);
	r_capability(RCAP_CULL_FACE, cap_cull_saved);
	r_depth_func(depth_func_saved);
	r_cull(cull_mode_saved);
}

static void _r_sprite_batch_add(Sprite *spr, const SpriteParams *params, VertexBuffer *vbuf) {
	Sprite2DAttrs alignas(32) attribs;
	r_mat_current(MM_MODELVIEW, attribs.transform);

	float scale_x = params->scale.x ? params->scale.x : 1;
	float scale_y = params->scale.y ? params->scale.y : scale_x;

	if(params->pos.x || params->pos.y) {
		glm_translate(attribs.transform, (vec3) { params->pos.x, params->pos.y });
	}

	if(params->rotation.angle) {
		float *rvec = (float*)params->rotation.vector;

		if(rvec[0] == 0 && rvec[1] == 0 && rvec[2] == 0) {
			glm_rotate(attribs.transform, params->rotation.angle, (vec3) { 0, 0, 1 });
		} else {
			glm_rotate(attribs.transform, params->rotation.angle, rvec);
		}
	}

	glm_scale(attribs.transform, (vec3) { scale_x * spr->w, scale_y * spr->h });

	if(params->color == 0) {
		// XXX: should we use r_color_current here?
		parse_color_array(rgba(1, 1, 1, 1), attribs.rgba);
	} else {
		parse_color_array(params->color, attribs.rgba);
	}

	attribs.texrect[0] = spr->tex_area.x / spr->tex->w;
	attribs.texrect[1] = spr->tex_area.y / spr->tex->h;
	attribs.texrect[2] = spr->tex_area.w / spr->tex->w;
	attribs.texrect[3] = spr->tex_area.h / spr->tex->h;
	attribs.flip[0] = params->flip.x ? 0xFF : 0x0;
	attribs.flip[1] = params->flip.y ? 0xFF : 0x0;
	attribs.custom = params->custom;

	r_vertex_buffer_append(vbuf, sizeof(attribs), &attribs);

	if(vbuf->size - vbuf->offset < sizeof(attribs)) {
		log_warn("Vertex buffer exhausted (%zu needed for next sprite, %u remaining), flush forced", sizeof(attribs), vbuf->size - vbuf->offset);
		r_flush_sprites();
	}
}

void r_draw_sprite(const SpriteParams *params) {
	assert(!(params->shader && params->shader_ptr));
	assert(!(params->sprite && params->sprite_ptr));

	Sprite *spr = params->sprite_ptr;

	if(spr == NULL) {
		assert(params->sprite != NULL);
		spr = get_sprite(params->sprite);
	}

	if(spr->tex != _r_sprite_batch.tex) {
		r_flush_sprites();
		_r_sprite_batch.tex = spr->tex;
	}

	ShaderProgram *prog = params->shader_ptr;

	if(prog == NULL) {
		if(params->shader == NULL) {
			prog = r_shader_current();
		} else {
			prog = r_shader_get(params->shader);
		}
	}

	assert(prog != NULL);

	if(prog != _r_sprite_batch.shader) {
		r_flush_sprites();
		_r_sprite_batch.shader = prog;
	}

	RenderTarget *target = r_target_current();

	if(target != _r_sprite_batch.render_target) {
		r_flush_sprites();
		_r_sprite_batch.render_target = target;
	}

	BlendMode blend = params->blend;

	if(blend == 0) {
		blend = r_blend_current();
	}

	if(blend != _r_sprite_batch.blend) {
		r_flush_sprites();
		_r_sprite_batch.blend = blend;
	}

	bool depth_test_enabled = r_capability_current(RCAP_DEPTH_TEST);
	bool depth_write_enabled = r_capability_current(RCAP_DEPTH_WRITE);
	bool cull_enabled = r_capability_current(RCAP_CULL_FACE);
	DepthTestFunc depth_func = r_depth_func_current();
	CullFaceMode cull_mode = r_cull_current();

	if(_r_sprite_batch.depth_test_enabled != depth_test_enabled) {
		r_flush_sprites();
		_r_sprite_batch.depth_test_enabled = depth_test_enabled;
	}

	if(_r_sprite_batch.depth_write_enabled != depth_write_enabled) {
		r_flush_sprites();
		_r_sprite_batch.depth_write_enabled = depth_write_enabled;
	}

	if(_r_sprite_batch.cull_enabled != cull_enabled) {
		r_flush_sprites();
		_r_sprite_batch.cull_enabled = cull_enabled;
	}

	if(_r_sprite_batch.depth_func != depth_func) {
		r_flush_sprites();
		_r_sprite_batch.depth_func = depth_func;
	}

	if(_r_sprite_batch.cull_mode != cull_mode) {
		r_flush_sprites();
		_r_sprite_batch.cull_mode = cull_mode;
	}

	_r_sprite_batch.num_pending++;
	_r_sprite_batch_add(spr, params, &_r_sprite_batch.vbuf);
}
