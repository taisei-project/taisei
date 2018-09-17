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
#include "util/glm.h"

typedef struct SpriteAttribs {
	mat4 transform;
	mat4 tex_transform;
	float rgba[4];
	FloatRect texrect;
	float sprite_size[2];
	float custom[4];

	// offset of this == size without padding.
	char end_of_fields;
} SpriteAttribs;

#define SIZEOF_SPRITE_ATTRIBS (offsetof(SpriteAttribs, end_of_fields))

static struct SpriteBatchState {
	VertexArray *varr;
	VertexBuffer *vbuf;
	uint base_instance;
	Texture *primary_texture;
	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];
	ShaderProgram *shader;
	BlendMode blend;
	Framebuffer *framebuffer;
	CullFaceMode cull_mode;
	DepthTestFunc depth_func;
	mat4 projection CGLM_ALIGN(32);
	uint cull_enabled : 1;
	uint depth_test_enabled : 1;
	uint depth_write_enabled : 1;
	uint num_pending;

	struct {
		uint flushes;
		uint sprites;
		uint best_batch;
		uint worst_batch;
	} frame_stats;
} _r_sprite_batch;

void _r_sprite_batch_init(void) {
	#ifdef DEBUG
	preload_resource(RES_FONT, "monotiny", RESF_PERMANENT);
	#endif

	size_t sz_vert = sizeof(GenericModelVertex);
	size_t sz_attr = SIZEOF_SPRITE_ATTRIBS;

	#define VERTEX_OFS(attr)   offsetof(GenericModelVertex,  attr)
	#define INSTANCE_OFS(attr) offsetof(SpriteAttribs, attr)

	VertexAttribFormat fmt[] = {
		// Per-vertex attributes (for the static models buffer, bound at 0)
		{ { 3, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(position),           0 },
		{ { 3, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(normal),             0 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(uv),                 0 },

		// Per-instance attributes (for our own sprites buffer, bound at 1)
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(transform[0]),     1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(transform[1]),     1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(transform[2]),     1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(transform[3]),     1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(tex_transform[0]), 1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(tex_transform[1]), 1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(tex_transform[2]), 1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(tex_transform[3]), 1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(rgba),             1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(texrect),          1 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(sprite_size),      1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(custom),           1 },
	};

	#undef VERTEX_OFS
	#undef INSTANCE_OFS

	uint capacity;

	if(r_supports(RFEAT_DRAW_INSTANCED_BASE_INSTANCE)) {
		capacity = 1 << 15;
	} else {
		capacity = 1 << 11;
	}

	_r_sprite_batch.vbuf = r_vertex_buffer_create(sz_attr * capacity, NULL);
	r_vertex_buffer_set_debug_label(_r_sprite_batch.vbuf, "Sprite batch vertex buffer");
	r_vertex_buffer_invalidate(_r_sprite_batch.vbuf);

	_r_sprite_batch.varr = r_vertex_array_create();
	r_vertex_array_set_debug_label(_r_sprite_batch.varr, "Sprite batch vertex array");
	r_vertex_array_layout(_r_sprite_batch.varr, sizeof(fmt)/sizeof(*fmt), fmt);
	r_vertex_array_attach_buffer(_r_sprite_batch.varr, r_vertex_buffer_static_models(), 0);
	r_vertex_array_attach_buffer(_r_sprite_batch.varr, _r_sprite_batch.vbuf, 1);
}

void _r_sprite_batch_shutdown(void) {
	r_vertex_array_destroy(_r_sprite_batch.varr);
	r_vertex_buffer_destroy(_r_sprite_batch.vbuf);
}

void r_flush_sprites(void) {
	if(_r_sprite_batch.num_pending == 0) {
		return;
	}

	uint pending = _r_sprite_batch.num_pending;

	// needs to be done early to thwart recursive callss

	if(_r_sprite_batch.frame_stats.flushes) {
		if(_r_sprite_batch.num_pending > _r_sprite_batch.frame_stats.best_batch) {
			_r_sprite_batch.frame_stats.best_batch = _r_sprite_batch.num_pending;
		}

		if(_r_sprite_batch.num_pending < _r_sprite_batch.frame_stats.worst_batch) {
			_r_sprite_batch.frame_stats.worst_batch = _r_sprite_batch.num_pending;
		}
	} else {
		_r_sprite_batch.frame_stats.worst_batch = _r_sprite_batch.frame_stats.best_batch = _r_sprite_batch.num_pending;
	}

	_r_sprite_batch.num_pending = 0;
	_r_sprite_batch.frame_stats.flushes++;

	r_state_push();

	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	glm_mat4_copy(_r_sprite_batch.projection, *r_mat_current_ptr(MM_PROJECTION));

	r_vertex_array(_r_sprite_batch.varr);
	r_shader_ptr(_r_sprite_batch.shader);
	r_uniform_sampler("tex", _r_sprite_batch.primary_texture);
	r_uniform_sampler_array("tex_aux[0]", 0, R_NUM_SPRITE_AUX_TEXTURES, _r_sprite_batch.aux_textures);
	r_framebuffer(_r_sprite_batch.framebuffer);
	r_blend(_r_sprite_batch.blend);
	r_capability(RCAP_DEPTH_TEST, _r_sprite_batch.depth_test_enabled);
	r_capability(RCAP_DEPTH_WRITE, _r_sprite_batch.depth_write_enabled);
	r_capability(RCAP_CULL_FACE, _r_sprite_batch.cull_enabled);
	r_depth_func(_r_sprite_batch.depth_func);
	r_cull(_r_sprite_batch.cull_mode);

	if(r_supports(RFEAT_DRAW_INSTANCED_BASE_INSTANCE)) {
		r_draw(PRIM_TRIANGLE_STRIP, 0, 4, NULL, pending, _r_sprite_batch.base_instance);
		_r_sprite_batch.base_instance += pending;

		size_t remaining = r_vertex_buffer_get_capacity(_r_sprite_batch.vbuf) - r_vertex_buffer_get_cursor(_r_sprite_batch.vbuf);

		if(remaining < SIZEOF_SPRITE_ATTRIBS) {
			// log_debug("Invalidating after %u sprites", _r_sprite_batch.base_instance);
			r_vertex_buffer_invalidate(_r_sprite_batch.vbuf);
			_r_sprite_batch.base_instance = 0;
		}
	} else {
		r_draw(PRIM_TRIANGLE_STRIP, 0, 4, NULL, pending, 0);
		r_vertex_buffer_invalidate(_r_sprite_batch.vbuf);
	}

	r_mat_pop();
	r_state_pop();
}

static void _r_sprite_batch_add(Sprite *spr, const SpriteParams *params, VertexBuffer *vbuf) {
	SpriteAttribs alignas(32) attribs;
	r_mat_current(MM_MODELVIEW, attribs.transform);
	r_mat_current(MM_TEXTURE, attribs.tex_transform);

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

	glm_scale(attribs.transform, (vec3) { scale_x * spr->w, scale_y * spr->h, 1 });

	if(params->color == NULL) {
		// XXX: should we use r_color_current here?
		attribs.rgba[0] = attribs.rgba[1] = attribs.rgba[2] = attribs.rgba[3] = 1;
	} else {
		memcpy(attribs.rgba, params->color, sizeof(attribs.rgba));
	}

	uint tw, th;
	r_texture_get_size(spr->tex, 0, &tw, &th);

	attribs.texrect.x = spr->tex_area.x / tw;
	attribs.texrect.y = spr->tex_area.y / th;
	attribs.texrect.w = spr->tex_area.w / tw;
	attribs.texrect.h = spr->tex_area.h / th;

	if(params->flip.x) {
		attribs.texrect.x += attribs.texrect.w;
		attribs.texrect.w *= -1;
	}

	if(params->flip.y) {
		attribs.texrect.y += attribs.texrect.h;
		attribs.texrect.h *= -1;
	}

	attribs.sprite_size[0] = spr->w;
	attribs.sprite_size[1] = spr->h;

	if(params->shader_params != NULL) {
		memcpy(attribs.custom, params->shader_params, sizeof(attribs.custom));
	} else {
		memset(attribs.custom, 0, sizeof(attribs.custom));
	}

	r_vertex_buffer_append(vbuf, SIZEOF_SPRITE_ATTRIBS, &attribs);
	_r_sprite_batch.frame_stats.sprites++;
}

void r_draw_sprite(const SpriteParams *params) {
	assert(!(params->shader && params->shader_ptr));
	assert(!(params->sprite && params->sprite_ptr));

	Sprite *spr = params->sprite_ptr;

	if(spr == NULL) {
		assert(params->sprite != NULL);
		spr = get_sprite(params->sprite);
	}

	if(spr->tex != _r_sprite_batch.primary_texture) {
		r_flush_sprites();
		_r_sprite_batch.primary_texture = spr->tex;
	}

	for(uint i = 0; i < R_NUM_SPRITE_AUX_TEXTURES; ++i) {
		Texture *aux_tex = params->aux_textures[i];

		if(aux_tex != NULL && aux_tex != _r_sprite_batch.aux_textures[i]) {
			r_flush_sprites();
			_r_sprite_batch.aux_textures[i] = aux_tex;
		}
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

	Framebuffer *fb = r_framebuffer_current();

	if(fb != _r_sprite_batch.framebuffer) {
		r_flush_sprites();
		_r_sprite_batch.framebuffer = fb;
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

	mat4 *current_projection = r_mat_current_ptr(MM_PROJECTION);

	if(memcmp(*current_projection, _r_sprite_batch.projection, sizeof(mat4))) {
		r_flush_sprites();
		glm_mat4_copy(*current_projection, _r_sprite_batch.projection);
	}

	size_t remaining = r_vertex_buffer_get_capacity(_r_sprite_batch.vbuf) - r_vertex_buffer_get_cursor(_r_sprite_batch.vbuf);

	if(remaining < SIZEOF_SPRITE_ATTRIBS) {
		if(!r_supports(RFEAT_DRAW_INSTANCED_BASE_INSTANCE)) {
			log_warn("Vertex buffer exhausted (%zu needed for next sprite, %zu remaining), flush forced", SIZEOF_SPRITE_ATTRIBS, remaining);
		}

		r_flush_sprites();
	}

	_r_sprite_batch.num_pending++;
	_r_sprite_batch_add(spr, params, _r_sprite_batch.vbuf);
}

#include "resource/font.h"

void _r_sprite_batch_end_frame(void) {
#ifdef DEBUG
	if(!_r_sprite_batch.frame_stats.flushes) {
		return;
	}

	r_flush_sprites();

	static char buf[512];
	snprintf(buf, sizeof(buf), "%6i sprites %6i flushes %9.02f spr/flush %6i best %6i worst",
		_r_sprite_batch.frame_stats.sprites,
		_r_sprite_batch.frame_stats.flushes,
		_r_sprite_batch.frame_stats.sprites / (double)_r_sprite_batch.frame_stats.flushes,
		_r_sprite_batch.frame_stats.best_batch,
		_r_sprite_batch.frame_stats.worst_batch
	);

	Font *font = get_font("monotiny");
	text_draw(buf, &(TextParams) {
		.pos = { 0, font_get_lineskip(font) },
		.font_ptr = font,
		.color = RGB(1, 1, 1),
		.shader = "text_default",
	});

	memset(&_r_sprite_batch.frame_stats, 0, sizeof(_r_sprite_batch.frame_stats));

#endif
}

void _r_sprite_batch_texture_deleted(Texture *tex) {
	if(_r_sprite_batch.primary_texture == tex) {
		_r_sprite_batch.primary_texture = NULL;
	}

	for(uint i = 0; i < R_NUM_SPRITE_AUX_TEXTURES; ++i) {
		if(_r_sprite_batch.aux_textures[i] == tex) {
			_r_sprite_batch.aux_textures[i] = NULL;
		}
	}
}
