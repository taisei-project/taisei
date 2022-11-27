/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sprite_batch.h"
#include "../api.h"
#include "util/glm.h"
#include "resource/sprite.h"
#include "resource/model.h"

#define SPRITE_BATCH_STATS 0

#ifndef SPRITE_BATCH_STATS
#ifdef DEBUG
	#define SPRITE_BATCH_STATS 1
#else
	#define SPRITE_BATCH_STATS 0
#endif
#endif

#define SIZEOF_SPRITE_ATTRIBS (offsetof(SpriteInstanceAttribs, end_of_fields))

static struct SpriteBatchState {
	// constants (set once on init and not expected to change)
	VertexArray *varr;
	VertexBuffer *vbuf;
	Model quad;
	r_feature_bits_t renderer_features;

	// varying state
	mat4 projection;
	Texture *primary_texture;
	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];
	ShaderProgram *shader;
	Framebuffer *framebuffer;
	uint base_instance;
	BlendMode blend;
	CullFaceMode cull_mode;
	DepthTestFunc depth_func;
	uint num_pending;
	r_capability_bits_t capbits;

#if SPRITE_BATCH_STATS
	struct {
		uint flushes;
		uint sprites;
		uint best_batch;
		uint worst_batch;
	} frame_stats;
#endif
} _r_sprite_batch;

void _r_sprite_batch_init(void) {
#if SPRITE_BATCH_STATS
	preload_resource(RES_FONT, "monotiny", RESF_PERMANENT);
#endif

	size_t sz_vert = sizeof(GenericModelVertex);
	size_t sz_attr = SIZEOF_SPRITE_ATTRIBS;

	#define VERTEX_OFS(attr)   offsetof(GenericModelVertex,  attr)
	#define INSTANCE_OFS(attr) offsetof(SpriteInstanceAttribs, attr)

	VertexAttribFormat fmt[] = {
		// Per-vertex attributes (for the static models buffer, bound at 0)
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(position),           0 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(uv),                 0 },
		{ { 3, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(normal),             0 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(tangent),            0 },

		// Per-instance attributes (for our own sprites buffer, bound at 1)
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(mv_transform[0]),  1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(mv_transform[1]),  1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(mv_transform[2]),  1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(mv_transform[3]),  1 },
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

	uint capacity = 1 << 11;

	_r_sprite_batch.vbuf = r_vertex_buffer_create(sz_attr * capacity, NULL);
	r_vertex_buffer_set_debug_label(_r_sprite_batch.vbuf, "Sprite batch vertex buffer");
	r_vertex_buffer_invalidate(_r_sprite_batch.vbuf);

	_r_sprite_batch.varr = r_vertex_array_create();
	r_vertex_array_set_debug_label(_r_sprite_batch.varr, "Sprite batch vertex array");
	r_vertex_array_layout(_r_sprite_batch.varr, sizeof(fmt)/sizeof(*fmt), fmt);
	r_vertex_array_attach_vertex_buffer(_r_sprite_batch.varr, r_vertex_buffer_static_models(), 0);
	r_vertex_array_attach_vertex_buffer(_r_sprite_batch.varr, _r_sprite_batch.vbuf, 1);

	_r_sprite_batch.quad.num_indices = 0;
	_r_sprite_batch.quad.num_vertices = 4;
	_r_sprite_batch.quad.offset = 0;
	_r_sprite_batch.quad.primitive = PRIM_TRIANGLE_STRIP;
	_r_sprite_batch.quad.vertex_array = _r_sprite_batch.varr;

	_r_sprite_batch.renderer_features = r_features();
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

	// needs to be done early to thwart recursive calls
	_r_sprite_batch.num_pending = 0;

#if SPRITE_BATCH_STATS
	if(_r_sprite_batch.frame_stats.flushes) {
		if(pending > _r_sprite_batch.frame_stats.best_batch) {
			_r_sprite_batch.frame_stats.best_batch = pending;
		}

		if(pending < _r_sprite_batch.frame_stats.worst_batch) {
			_r_sprite_batch.frame_stats.worst_batch = pending;
		}
	} else {
		_r_sprite_batch.frame_stats.worst_batch = _r_sprite_batch.frame_stats.best_batch = pending;
	}

	_r_sprite_batch.frame_stats.flushes++;
#endif

	r_state_push();
	r_mat_proj_push_premade(_r_sprite_batch.projection);

	r_shader_ptr(NOT_NULL(_r_sprite_batch.shader));
	r_uniform_sampler("tex", _r_sprite_batch.primary_texture);
	r_uniform_sampler_array("tex_aux[0]", 0, R_NUM_SPRITE_AUX_TEXTURES, _r_sprite_batch.aux_textures);
	r_framebuffer(_r_sprite_batch.framebuffer);
	r_blend(_r_sprite_batch.blend);
	r_capabilities(_r_sprite_batch.capbits);

	if(_r_sprite_batch.capbits & r_capability_bit(RCAP_DEPTH_TEST)) {
		r_depth_func(_r_sprite_batch.depth_func);
	}

	if(_r_sprite_batch.capbits & r_capability_bit(RCAP_CULL_FACE)) {
		r_cull(_r_sprite_batch.cull_mode);
	}

	r_draw_model_ptr(&_r_sprite_batch.quad, pending, 0);
	r_vertex_buffer_invalidate(_r_sprite_batch.vbuf);

	r_mat_proj_pop();
	r_state_pop();
}

static void _r_sprite_batch_compute_attribs(
	const Sprite *restrict spr,
	const SpriteParams *restrict params,
	SpriteInstanceAttribs *out_attribs
) {
	SpriteInstanceAttribs attribs;
	r_mat_mv_current(attribs.mv_transform);
	r_mat_tex_current(attribs.tex_transform);

	float scale_x = params->scale.x ? params->scale.x : 1;
	float scale_y = params->scale.y ? params->scale.y : scale_x;

	FloatOffset ofs = spr->padding.offset;
	FloatExtent imgdims = spr->extent;
	imgdims.as_cmplx -= spr->padding.extent.as_cmplx;

	if(params->pos.x || params->pos.y) {
		glm_translate(attribs.mv_transform, (vec3) { params->pos.x, params->pos.y });
	}

	if(params->rotation.angle) {
		float *rvec = (float*)params->rotation.vector;

		if(rvec[0] == 0 && rvec[1] == 0 && rvec[2] == 0) {
			glm_rotate(attribs.mv_transform, params->rotation.angle, (vec3) { 0, 0, 1 });
		} else {
			glm_rotate(attribs.mv_transform, params->rotation.angle, rvec);
		}
	}

	glm_scale(attribs.mv_transform, (vec3) { scale_x * imgdims.w, scale_y * imgdims.h, 1 });

	if(ofs.x || ofs.y) {
		if(params->flip.x) {
			ofs.x *= -1;
		}

		if(params->flip.y) {
			ofs.y *= -1;
		}

		glm_translate(attribs.mv_transform, (vec3) { ofs.x / imgdims.w, ofs.y / imgdims.h });
	}

	if(params->color == NULL) {
		// XXX: should we use r_color_current here?
		attribs.rgba = *RGBA(1, 1, 1, 1);
	} else {
		attribs.rgba = *params->color;
	}

	attribs.texrect = spr->tex_area;

	if(params->flip.x) {
		attribs.texrect.x += attribs.texrect.w;
		attribs.texrect.w *= -1;
	}

	if(params->flip.y) {
		attribs.texrect.y += attribs.texrect.h;
		attribs.texrect.h *= -1;
	}

	attribs.sprite_size = spr->extent;

	if(params->shader_params == NULL) {
		memset(&attribs.custom, 0, sizeof(attribs.custom));
	} else {
		attribs.custom = *params->shader_params;
	}

	*out_attribs = attribs;
}

INLINE void _r_sprite_batch_process_params(
	const SpriteParams *restrict sprite_params,
	SpriteStateParams *restrict state_params,
	Sprite *restrict *sprite
) {
	assert(!(sprite_params->shader && sprite_params->shader_ptr));
	assert(!(sprite_params->sprite && sprite_params->sprite_ptr));

	if((*sprite = sprite_params->sprite_ptr) == NULL) {
		*sprite = res_sprite(NOT_NULL(sprite_params->sprite));
	}

	state_params->primary_texture = (*sprite)->tex;
	memcpy(&state_params->aux_textures, &sprite_params->aux_textures, sizeof(sprite_params->aux_textures));
	state_params->blend = sprite_params->blend;

	if(state_params->blend == 0) {
		state_params->blend = r_blend_current();
	}

	if((state_params->shader = sprite_params->shader_ptr) == NULL) {
		if(sprite_params->shader != NULL) {
			state_params->shader = res_shader(sprite_params->shader);
		} else {
			state_params->shader = r_shader_current();
		}
	}
}

void r_sprite_batch_prepare_state(const SpriteStateParams *stp) {
	if(stp->primary_texture != _r_sprite_batch.primary_texture) {
		r_flush_sprites();
		_r_sprite_batch.primary_texture = stp->primary_texture;
	}

	for(uint i = 0; i < R_NUM_SPRITE_AUX_TEXTURES; ++i) {
		Texture *aux_tex = stp->aux_textures[i];

		if(aux_tex != NULL && aux_tex != _r_sprite_batch.aux_textures[i]) {
			r_flush_sprites();
			_r_sprite_batch.aux_textures[i] = aux_tex;
		}
	}

	assume(stp->shader != NULL);

	if(stp->shader != _r_sprite_batch.shader) {
		r_flush_sprites();
		_r_sprite_batch.shader = stp->shader;
	}

	BlendMode blend = stp->blend;

	if(blend != _r_sprite_batch.blend) {
		r_flush_sprites();
		_r_sprite_batch.blend = blend;
	}

	Framebuffer *fb = r_framebuffer_current();

	if(fb != _r_sprite_batch.framebuffer) {
		r_flush_sprites();
		_r_sprite_batch.framebuffer = fb;
	}

	r_capability_bits_t caps = r_capabilities_current();
	DepthTestFunc depth_func = r_depth_func_current();
	CullFaceMode cull_mode = r_cull_current();

	if(_r_sprite_batch.capbits != caps) {
		r_flush_sprites();
		_r_sprite_batch.capbits = caps;
	}

	if((caps & r_capability_bit(RCAP_DEPTH_TEST)) && _r_sprite_batch.depth_func != depth_func) {
		r_flush_sprites();
		_r_sprite_batch.depth_func = depth_func;
	}

	if((caps & r_capability_bit(RCAP_CULL_FACE)) && _r_sprite_batch.cull_mode != cull_mode) {
		r_flush_sprites();
		_r_sprite_batch.cull_mode = cull_mode;
	}

	mat4 *current_projection = r_mat_proj_current_ptr();

	if(memcmp(*current_projection, _r_sprite_batch.projection, sizeof(mat4))) {
		r_flush_sprites();
		glm_mat4_copy(*current_projection, _r_sprite_batch.projection);
	}
}

void r_sprite_batch_add_instance(const SpriteInstanceAttribs *attribs) {
	SDL_RWops *stream = r_vertex_buffer_get_stream(_r_sprite_batch.vbuf);
	SDL_RWwrite(stream, attribs, SIZEOF_SPRITE_ATTRIBS, 1);

	_r_sprite_batch.num_pending++;

#if SPRITE_BATCH_STATS
	_r_sprite_batch.frame_stats.sprites++;
#endif
}

void r_draw_sprite(const SpriteParams *params) {
	SpriteStateParams state_params;
	SpriteInstanceAttribs attribs;
	Sprite *spr;

	_r_sprite_batch_process_params(params, &state_params, &spr);
	r_sprite_batch_prepare_state(&state_params);
	_r_sprite_batch_compute_attribs(spr, params, &attribs);
	r_sprite_batch_add_instance(&attribs);
}

#if SPRITE_BATCH_STATS
#include "resource/font.h"
#include "global.h"
#endif

void _r_sprite_batch_end_frame(void) {
	r_flush_sprites();

#if SPRITE_BATCH_STATS
	if(!_r_sprite_batch.frame_stats.flushes) {
		return;
	}

	static char buf[512];
	snprintf(buf, sizeof(buf), "%6i sprites %6i flushes %9.02f spr/flush %6i best %6i worst %12.02f fps",
		_r_sprite_batch.frame_stats.sprites,
		_r_sprite_batch.frame_stats.flushes,
		_r_sprite_batch.frame_stats.sprites / (double)_r_sprite_batch.frame_stats.flushes,
		_r_sprite_batch.frame_stats.best_batch,
		_r_sprite_batch.frame_stats.worst_batch,
		global.fps.render.fps
	);

	Font *font = res_font("monotiny");
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
