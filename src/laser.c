/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "laser.h"
#include "global.h"
#include "list.h"
#include "stageobjects.h"
#include "stagedraw.h"
#include "renderer/api.h"
#include "resource/model.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "video.h"

// Should be set slightly larger than the
// negated lowest distance threshold value in the sdf_apply shader
#define SDF_RANGE 4.01

#define TILES_X 8
#define TILES_Y 7
// Maximum number of lasers that can be rendered "at once",
// i.e. in one SDF generation pass followed by one coloring pass.
#define TILES_MAX (TILES_X * TILES_Y)

#define SEGMENTS_MAX 512

static struct {
	struct {
		VertexArray *va;
		VertexBuffer *vb;
		SDL_RWops *vb_stream;
		Model quad;
	} pass1, pass2;

	struct {
		ShaderProgram *sdf_generate;
		ShaderProgram *sdf_apply;
	} shaders;

	struct {
		Framebuffer *saved;
		Framebuffer *sdf;
		ManagedFramebufferGroup *group;
	} fb;

	struct {
		LIST_ANCHOR(LaserRenderData) batch;
		int free_tile;
		int rendered_lasers;
		bool pass1_state_ready;
		bool drawing_lasers;
	} render_state;
} lasers;

#define LASER_FROM_RENDERDATA(rd) \
	UNION_CAST(char*, Laser*, \
		UNION_CAST(LaserRenderData*, char*, (rd)) - offsetof(Laser, _renderdata))

typedef struct LaserInstancedAttribsPass1 {
	union {
		struct { cmplxf a, b; } pos;
		float attr0[4];
	};

	union {
		struct {
			struct { float a, b; } width;
			struct { float a, b; } time;
		};
		float attr1[4];
	};
} LaserInstancedAttribsPass1;

typedef struct LaserInstancedAttribsPass2 {
	union {
		FloatRect bbox;
		float attr0[4];
	};

	union {
		struct {
			Color3 color;
			float width;
		};
		float attr1[4];
	};

	union {
		FloatOffset texture_ofs;
		float attr2[2];
	};
} LaserInstancedAttribsPass2;

static void lasers_ent_predraw_hook(EntityInterface *ent, void *arg);
static void lasers_ent_postdraw_hook(EntityInterface *ent, void *arg);

static void lasers_sdf_fb_resize_strategy(void *userdata, IntExtent *fb_size, FloatRect *fb_viewport) {
	float w, h;
	float vid_vp_w, vid_vp_h;
	video_get_viewport_size(&vid_vp_w, &vid_vp_h);

	float q = config_get_float(CONFIG_FG_QUALITY);

	float wfactor = fminf(1.0f, vid_vp_w / SCREEN_W);
	float hfactor = fminf(1.0f, vid_vp_h / SCREEN_H);

	float wbase = VIEWPORT_W * TILES_X;
	float hbase = VIEWPORT_H * TILES_Y;

	w = floorf(wfactor * wbase) * q;
	h = floorf(hfactor * hbase) * q;

	*fb_size = (IntExtent) { w, h };
	*fb_viewport = (FloatRect) { 0, 0, w, h };
}

static void create_pass1_resources(void) {
	size_t sz_vert = sizeof(GenericModelVertex);
	size_t sz_attr = sizeof(LaserInstancedAttribsPass1);

	#define VERTEX_OFS(attr)   offsetof(GenericModelVertex,         attr)
	#define INSTANCE_OFS(attr) offsetof(LaserInstancedAttribsPass1, attr)

	VertexAttribFormat fmt[] = {
		// Per-vertex attributes (for the static models buffer, bound at 0)
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(position), 0 },

		// Per-instance attributes (for our streaming buffer, bound at 1)
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(attr0),  1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(attr1),  1 },
	};

	#undef VERTEX_OFS
	#undef INSTANCE_OFS

	VertexBuffer *vb = r_vertex_buffer_create(sz_attr * SEGMENTS_MAX, NULL);
	r_vertex_buffer_set_debug_label(vb, "Lasers VB pass 1");

	VertexArray *va = r_vertex_array_create();
	r_vertex_array_set_debug_label(va, "Lasers VA pass 1");
	r_vertex_array_attach_vertex_buffer(va, r_vertex_buffer_static_models(), 0);
	r_vertex_array_attach_vertex_buffer(va, vb, 1);
	r_vertex_array_layout(va, ARRAY_SIZE(fmt), fmt);

	lasers.pass1.va = va;
	lasers.pass1.vb = vb;
	lasers.pass1.vb_stream = r_vertex_buffer_get_stream(vb);

	lasers.pass1.quad = *r_model_get_quad();
	lasers.pass1.quad.vertex_array = va;
}

static void create_pass2_resources(void) {
	size_t sz_vert = sizeof(GenericModelVertex);
	size_t sz_attr = sizeof(LaserInstancedAttribsPass2);

	#define VERTEX_OFS(attr)   offsetof(GenericModelVertex,         attr)
	#define INSTANCE_OFS(attr) offsetof(LaserInstancedAttribsPass2, attr)

	VertexAttribFormat fmt[] = {
		// Per-vertex attributes (for the static models buffer, bound at 0)
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(position), 0 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(uv),       0 },

		// Per-instance attributes (for our streaming buffer, bound at 1)
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(attr0),  1 },
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(attr1),  1 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(attr2),  1 },
	};

	#undef VERTEX_OFS
	#undef INSTANCE_OFS

	VertexBuffer *vb = r_vertex_buffer_create(sz_attr * TILES_MAX, NULL);
	r_vertex_buffer_set_debug_label(vb, "Lasers VB pass 2");

	VertexArray *va = r_vertex_array_create();
	r_vertex_array_set_debug_label(va, "Lasers VA pass 2");
	r_vertex_array_attach_vertex_buffer(va, r_vertex_buffer_static_models(), 0);
	r_vertex_array_attach_vertex_buffer(va, vb, 1);
	r_vertex_array_layout(va, ARRAY_SIZE(fmt), fmt);

	lasers.pass2.va = va;
	lasers.pass2.vb = vb;
	lasers.pass2.vb_stream = r_vertex_buffer_get_stream(vb);

	lasers.pass2.quad = *r_model_get_quad();
	lasers.pass2.quad.vertex_array = va;
}

void lasers_preload(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"blur25",
		"blur5",
		"lasers/sdf_apply",
		"lasers/sdf_generate",
	NULL);

	create_pass1_resources();
	create_pass2_resources();

	FBAttachmentConfig aconf = { 0 };
	aconf.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	aconf.tex_params.type = TEX_TYPE_RG_16_FLOAT;
	aconf.tex_params.filter.min = TEX_FILTER_LINEAR;
	aconf.tex_params.filter.mag = TEX_FILTER_LINEAR;
	aconf.tex_params.wrap.s = TEX_WRAP_MIRROR;
	aconf.tex_params.wrap.t = TEX_WRAP_MIRROR;

	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = &aconf;
	fbconf.num_attachments = 1;
	fbconf.resize_strategy.resize_func = lasers_sdf_fb_resize_strategy;

	lasers.fb.group = fbmgr_group_create();
	lasers.fb.sdf = fbmgr_group_framebuffer_create(lasers.fb.group, "Lasers SDF FB", &fbconf);

	ent_hook_pre_draw(lasers_ent_predraw_hook, NULL);
	ent_hook_post_draw(lasers_ent_postdraw_hook, NULL);

	lasers.shaders.sdf_generate = res_shader("lasers/sdf_generate");
	lasers.shaders.sdf_apply = res_shader("lasers/sdf_apply");
}

void lasers_free(void) {
	fbmgr_group_destroy(lasers.fb.group);
	r_vertex_array_destroy(lasers.pass1.va);
	r_vertex_array_destroy(lasers.pass2.va);
	r_vertex_buffer_destroy(lasers.pass1.vb);
	r_vertex_buffer_destroy(lasers.pass2.vb);
	ent_unhook_pre_draw(lasers_ent_predraw_hook);
	ent_unhook_post_draw(lasers_ent_postdraw_hook);
}

static void ent_draw_laser(EntityInterface *ent);

Laser *create_laser(cmplx pos, float time, float deathtime, const Color *color, LaserPosRule prule, LaserLogicRule lrule, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	Laser *l = alist_push(&global.lasers, (Laser*)objpool_acquire(stage_object_pools.lasers));

	l->birthtime = global.frames;
	l->timespan = time;
	l->deathtime = deathtime;
	l->pos = pos;
	l->color = *color;

	l->args[0] = a0;
	l->args[1] = a1;
	l->args[2] = a2;
	l->args[3] = a3;

	l->prule = prule;
	l->lrule = lrule;

	l->collision_step = 1;
	l->width = 10;
	l->width_exponent = 1.0;
	l->speed = 1;
	l->timeshift = 0;
	l->next_graze = 0;
	l->clear_flags = 0;
	l->unclearable = false;
	l->collision_active = true;

	l->ent.draw_layer = LAYER_LASER_HIGH;
	l->ent.draw_func = ent_draw_laser;
	ent_register(&l->ent, ENT_TYPE_ID(Laser));

	if(l->lrule)
		l->lrule(l, EVENT_BIRTH);

	l->prule(l, EVENT_BIRTH);

	return l;
}

Laser *create_laserline(cmplx pos, cmplx dir, float charge, float dur, const Color *clr) {
	return create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr);
}

Laser *create_laserline_ab(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr) {
	cmplx m = (b-a)*0.005;

	return create_laser(a, 200, dur, clr, las_linear, static_laser, m, charge + I*width, 0, 0);
}

static float laser_graze_width(Laser *l, float *exponent) {
	*exponent = 0; // l->width_exponent * 0.5f;
	return 5.0f * sqrtf(l->width) + 15.0f;
}

static void draw_laser_prepare_state_pass1(void) {
	r_framebuffer(lasers.fb.sdf);
	r_clear(CLEAR_COLOR, RGBA(SDF_RANGE, 0, 0, 0), 1);
	r_blend(BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDOP_MIN,
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_DST_ALPHA, BLENDOP_MIN
	));
	r_shader_ptr(lasers.shaders.sdf_generate);
	r_uniform_float("sdf_range", SDF_RANGE);
}

static bool draw_laser_instanced_prepare(Laser *l, uint *out_instances, float *out_timeshift) {
	float t;
	int c;

	c = l->timespan;

	t = (global.frames - l->birthtime)*l->speed - l->timespan + l->timeshift;

	if(t + l->timespan > l->deathtime + l->timeshift)
		c += l->deathtime + l->timeshift - (t + l->timespan);

	if(t < 0) {
		c += t;
		t = 0;
	}

	if(c <= 0) {
		return false;
	}

	*out_instances = c * 2;
	*out_timeshift = t;

	return true;
}

static float calc_sample_width(
	Laser *l, float sample, float half_samples, float width_factor, float tail
) {
	float mid_ofs = sample - half_samples;
	return 0.75f * l->width * powf(
		width_factor * (mid_ofs - tail) * (mid_ofs + tail),
		l->width_exponent
	);
}

static int draw_laser_pass1_build(Laser *l) {
	float timeshift;
	uint samples;

	if(!draw_laser_instanced_prepare(l, &samples, &timeshift)) {
		return 0;
	}

	// PASS 1: build the signed distance field

	// Precomputed magic parameters for width calculation
	float half_samples = samples * 0.5;
	float tail = samples / 1.6;
	float width_factor = -1 / (tail * tail);

	// Maximum value of `1 - cos(angle)` between two curve segments to reduce to straight lines
	const float thres_angular = 1e-4;
	// Maximum laser-time sample difference between two segment points (for width interpolation)
	const float thres_temporal = samples / 16.0;
	// These values should be kept as high as possible without introducing artifacts.

	// Time step between samples
	// FIXME currently hardcoded, do not change. draw_laser_instanced_prepare should be adjusted
	// to take this into account.
	const float tstep = 0.5;

	// Number of segments generated
	int segments = 0;

	// Time value of current sample
	float t = timeshift;

	// Time value of last included sample
	float t0 = t;

	// Points of the current line segment
	// Begin constructing at t0
	cmplxf a, b;
	a = l->prule(l, t0);

	// Width value of the last included sample
	// Initialized to the width at t0
	float w0 = calc_sample_width(l, 0, half_samples, width_factor, tail);

	// Already sampled the first point, so shift
	t += tstep;

	// Vector from A to B of the last included segment, and its squared length.
	cmplxf v0;
	float v0_abs2;

	// FIXME: we have to initialize these with something sane…
	// but this method always causes the second sample to be skipped.
	v0 = l->prule(l, t) - a;
	v0_abs2 = cabs2f(v0);

	float viewmargin = l->width * 0.5f;
	FloatRect viewbounds = { .extent = VIEWPORT_SIZE };
	viewbounds.w += viewmargin * 2.0f;
	viewbounds.h += viewmargin * 2.0f;
	viewbounds.x -= viewmargin;
	viewbounds.y -= viewmargin;

	FloatOffset top_left, bottom_right;
	top_left.as_cmplx = a;
	bottom_right.as_cmplx = a;

	for(uint i = 1; i < samples; ++i, t += tstep) {
		b = l->prule(l, t);

		if(i < samples - 1 && (t - t0) < thres_temporal) {
			cmplxf v1 = b - a;

			// dot(a, b) == |a|*|b|*cos(theta)
			float dot = cdotf(v0, v1);
			float norm = sqrtf(v0_abs2 * cabs2f(v1));

			if(norm == 0.0f) {
				// degenerate case
				continue;
			}

			float cosTheta = dot / norm;
			float d = 1.0f - fabsf(cosTheta);

			if(d < thres_angular) {
				continue;
			}
		}

		float w = calc_sample_width(l, i, half_samples, width_factor, tail);

		float xa = crealf(a);
		float ya = cimagf(a);
		float xb = crealf(b);
		float yb = cimagf(b);

		bool visible =
			(xa > viewbounds.x && xa < viewbounds.w && ya > viewbounds.y && ya < viewbounds.h) ||
			(xb > viewbounds.x && xb < viewbounds.w && yb > viewbounds.y && yb < viewbounds.h);

		if(visible) {
			LaserInstancedAttribsPass1 attr = {
				.pos   = {   a,  b },
				.width = {  w0,  w },
				.time  = { -t0, -t },
			};

			SDL_RWwrite(lasers.pass1.vb_stream, &attr, sizeof(attr), 1);
			++segments;

			top_left.x     = fminf(    top_left.x, fminf(xa, xb));
			top_left.y     = fminf(    top_left.y, fminf(ya, yb));
			bottom_right.x = fmaxf(bottom_right.x, fmaxf(xa, xb));
			bottom_right.y = fmaxf(bottom_right.y, fmaxf(ya, yb));
		}

		t0 = t;
		w0 = w;
		v0 = b - a;
		v0_abs2 = cabs2f(v0);
		a = b;
	}

	float aabb_margin = SDF_RANGE + l->width * 0.5f;
	top_left.as_cmplx -= aabb_margin * (1.0f + I);
	bottom_right.as_cmplx += aabb_margin * (1.0f + I);

	top_left.x = fmaxf(0, top_left.x);
	top_left.y = fmaxf(0, top_left.y);
	bottom_right.x = fminf(VIEWPORT_W, bottom_right.x);
	bottom_right.y = fminf(VIEWPORT_H, bottom_right.y);

	l->_renderdata.bbox.top_left = top_left;
	l->_renderdata.bbox.bottom_right = bottom_right;
	return segments;
}

static void draw_laser_pass1_render(Laser *l, int segments) {
	Framebuffer *fb = r_framebuffer_current();
	FloatRect vp_orig;
	r_framebuffer_viewport_current(fb, &vp_orig);
	FloatRect vp = vp_orig;
	int sx = l->_renderdata.tile % TILES_X;
	int sy = l->_renderdata.tile / TILES_X;
	vp.w /= TILES_X;
	vp.h /= TILES_Y;
	vp.x = sx * vp.w;
	vp.y = sy * vp.h;
	r_framebuffer_viewport_rect(fb, vp);
	r_draw_model_ptr(&lasers.pass1.quad, segments, 0);
	r_vertex_buffer_invalidate(lasers.pass1.vb);
	r_framebuffer_viewport_rect(fb, vp_orig);
}

static void draw_laser_prepare_state_pass2(void) {
	Texture *tex = r_framebuffer_get_attachment(lasers.fb.sdf, FRAMEBUFFER_ATTACH_COLOR0);
	r_shader_ptr(lasers.shaders.sdf_apply);
	r_uniform_sampler("tex", tex);
	r_uniform_vec2("texsize", VIEWPORT_W * TILES_X, VIEWPORT_H * TILES_Y);
	r_blend(BLEND_PREMUL_ALPHA);
}

static void draw_laser_pass2(Laser *l) {
	// PASS 2: color the curve using the generated SDF

	FloatOffset top_left = l->_renderdata.bbox.top_left;
	FloatOffset bottom_right = l->_renderdata.bbox.bottom_right;
	cmplxf bbox_midpoint = (top_left.as_cmplx + bottom_right.as_cmplx) * 0.5f;
	cmplxf bbox_size = bottom_right.as_cmplx - top_left.as_cmplx;

	FloatRect bbox;
	bbox.offset.as_cmplx = bbox_midpoint;
	bbox.extent.as_cmplx = bbox_size;

	FloatRect frag = bbox;
	frag.offset.as_cmplx -= bbox.extent.as_cmplx * 0.5;
	frag.x += VIEWPORT_W * (l->_renderdata.tile % TILES_X);
	frag.y += VIEWPORT_H * (l->_renderdata.tile / TILES_X);

	LaserInstancedAttribsPass2 attrs = {
		.bbox = bbox,
		.texture_ofs = frag.offset,
		.color = l->color.color3,
		.width = l->width,
	};

	SDL_RWwrite(lasers.pass2.vb_stream, &attrs, sizeof(attrs), 1);
}

static void draw_lasers_pass2(void) {
	if(!lasers.render_state.batch.first) {
		return;
	}

	r_state_push();
	r_framebuffer(lasers.fb.saved);
	draw_laser_prepare_state_pass2();

	int instances = 0;

	for(;;) {
		LaserRenderData *rd = lasers.render_state.batch.last;

		if(rd == NULL) {
			break;
		}

		alist_unlink(&lasers.render_state.batch, rd);
		Laser *l = LASER_FROM_RENDERDATA(rd);
		draw_laser_pass2(l);
		++instances;
	}

	assert(instances > 0);
	r_draw_model_ptr(&lasers.pass2.quad, instances, 0);
	r_vertex_buffer_invalidate(lasers.pass2.vb);

	r_state_pop();

	lasers.render_state.rendered_lasers += instances;
	lasers.render_state.pass1_state_ready = false;
	lasers.render_state.free_tile = 0;
	assert(lasers.render_state.batch.first == NULL);
}

static void ent_draw_laser(EntityInterface *ent) {
	Laser *laser = ENT_CAST(ent, Laser);

	int segments = draw_laser_pass1_build(laser);

	if(segments <= 0) {
		return;
	}

	if(lasers.render_state.free_tile == TILES_MAX) {
		draw_lasers_pass2();
	}

	assert(lasers.render_state.free_tile < TILES_MAX);
	laser->_renderdata.tile = lasers.render_state.free_tile++;
	alist_push(&lasers.render_state.batch, &laser->_renderdata);

	if(!lasers.render_state.pass1_state_ready) {
		// Pop out of entity-wide state into lasers-wide state
		r_state_pop();

		draw_laser_prepare_state_pass1();
		lasers.render_state.pass1_state_ready = true;

		// Push new entity-wide state; entity draw loop will pop it
		r_state_push();
	}

	draw_laser_pass1_render(laser, segments);
}

static void lasers_ent_predraw_hook(EntityInterface *ent, void *arg) {
	if(ent && ent->type == ENT_TYPE_ID(Laser)) {
		if(!lasers.render_state.drawing_lasers) {
			lasers.fb.saved = r_framebuffer_current();
			r_state_push();
			lasers.render_state.drawing_lasers = true;
		}
	} else {
		if(lasers.render_state.drawing_lasers) {
			r_state_pop();
			lasers.render_state.drawing_lasers = false;
		}

		draw_lasers_pass2();
	}
}

static void lasers_ent_postdraw_hook(EntityInterface *ent, void *arg) {
	if(ent == NULL) {
		// Edge case when the last entity drawn is a laser
		lasers_ent_predraw_hook(NULL, arg);
	}
}

static void *_delete_laser(ListAnchor *lasers, List *laser, void *arg) {
	Laser *l = (Laser*)laser;

	if(l->lrule) {
		l->lrule(l, EVENT_DEATH);
	}

	ent_unregister(&l->ent);
	objpool_release(stage_object_pools.lasers, alist_unlink(lasers, laser));
	return NULL;
}

static void delete_laser(LaserList *lasers, Laser *laser) {
	_delete_laser((ListAnchor*)lasers, (List*)laser, NULL);
}

void delete_lasers(void) {
	alist_foreach(&global.lasers, _delete_laser, NULL);
}

bool laser_is_active(Laser *l) {
	// return l->width > 3.0;
	return l->collision_active;
}

bool laser_is_clearable(Laser *l) {
	return !l->unclearable && laser_is_active(l);
}

bool clear_laser(Laser *l, uint flags) {
	if(!(flags & CLEAR_HAZARDS_FORCE) && !laser_is_clearable(l)) {
		return false;
	}

	l->clear_flags |= flags;
	return true;
}

static bool laser_collision(Laser *l);

void process_lasers(void) {
	Laser *laser = global.lasers.first, *del = NULL;
	bool stage_cleared = stage_is_cleared();

	while(laser != NULL) {
		if(stage_cleared) {
			clear_laser(laser, CLEAR_HAZARDS_LASERS | CLEAR_HAZARDS_FORCE);
		}

		if(laser->clear_flags & CLEAR_HAZARDS_LASERS) {
			// TODO: implement CLEAR_HAZARDS_NOW

			laser->timespan *= 0.9;
			bool kill_now = laser->timespan < 5;

			if(!((global.frames - laser->birthtime) % 2) || kill_now) {
				double t = fmax(0, (global.frames - laser->birthtime)*laser->speed - laser->timespan + laser->timeshift);
				cmplx p = laser->prule(laser, t);
				double x = creal(p);
				double y = cimag(p);

				if(x > 0 && x < VIEWPORT_W && y > 0 && y < VIEWPORT_H) {
					create_clear_item(p, laser->clear_flags);
				}

				if(kill_now) {
					PARTICLE(
						.sprite = "flare",
						.pos = p,
						.timeout = 20,
						.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
					);
					laser->deathtime = 0;
				}
			}
		} else {
			if(laser_collision(laser)) {
				ent_damage(&global.plr.ent, &(DamageInfo) { .type = DMG_ENEMY_SHOT });
			}

			if(laser->lrule) {
				laser->lrule(laser, global.frames - laser->birthtime);
			}
		}

		if(global.frames - laser->birthtime > laser->deathtime + laser->timespan*laser->speed) {
			del = laser;
			laser = laser->next;
			delete_laser(&global.lasers, del);
		} else {
			laser = laser->next;
		}
	}
}

static inline bool laser_collision_segment(Laser *l, LineSegment *segment, Circle *collision_area, float t, float segment_width_factor, float tail) {
	float mid_ofs = t - l->timespan * 0.5f;
	float widthfac_orig = segment_width_factor * (mid_ofs - tail) * (mid_ofs + tail);
	float widthfac = 0.75f * 0.5f * powf(widthfac_orig, l->width_exponent);

	collision_area->radius = fmaxf(widthfac * l->width - 4.0f, 2.0f);

	if(lineseg_circle_intersect(*segment, *collision_area) >= 0.0f) {
		return true;
	}

	if(global.frames >= l->next_graze) {
		float exponent;
		collision_area->radius = laser_graze_width(l, &exponent) * fmaxf(0.25f, powf(widthfac_orig, exponent));
		assert(collision_area->radius > 0);
		float f = lineseg_circle_intersect(*segment, *collision_area);

		if(f >= 0) {
			player_graze(&global.plr, segment->a + f * (segment->b - segment->a), 7, 5, &l->color);
			l->next_graze = global.frames + 4;
		}
	}

	return false;
}

static bool laser_collision(Laser *l) {
	if(!laser_is_active(l)) {
		return false;
	}

	float t_end_len = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_end_lifetime = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end_len - l->timespan;
	float t_end = fmin(t_end_len, t_end_lifetime);

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l, t) };
	Circle collision_area = { .origin = global.plr.pos };

	float tail = l->timespan / 1.6f;
	float width_factor = -1.0f / (tail * tail);

	for(t += l->collision_step; t < t_end; t += l->collision_step) {
		segment.b = l->prule(l, t);

		if(laser_collision_segment(l, &segment, &collision_area, t, width_factor, tail)) {
			return true;
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, t_end);
	return laser_collision_segment(l, &segment, &collision_area, t_end, width_factor, tail);
}

bool laser_intersects_ellipse(Laser *l, Ellipse ellipse) {
	// NOTE: this function does not take laser width into account

	float t_end_len = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_end_lifetime = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end_len - l->timespan;
	float t_end = fmin(t_end_len, t_end_lifetime);

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l, t) };

	for(t += l->collision_step; t < t_end; t += l->collision_step) {
		segment.b = l->prule(l, t);

		if(lineseg_ellipse_intersect(segment, ellipse)) {
			return true;
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, t_end);
	return lineseg_ellipse_intersect(segment, ellipse);
}

bool laser_intersects_circle(Laser *l, Circle circle) {
	Ellipse ellipse = {
		.origin = circle.origin,
		.axes = circle.radius * 2 * (1 + I),
	};

	return laser_intersects_ellipse(l, ellipse);
}

cmplx las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->collision_step = fmax(3, l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t;
}

cmplx las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->collision_step = fmax(3, l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

cmplx las_weird_sine(Laser *l, float t) {             // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this used to be called "las_sine", but it's actually not a proper sine wave
	// do we even still need this?

	if(t == EVENT_BIRTH) {
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s) / s)) * t * cabs(l->args[0]);
}

cmplx las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx line_vel = l->args[0];
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = cimag(line_dir) - I*creal(line_dir);
	cmplx sine_amp = l->args[1];
	cmplx sine_freq = l->args[2];
	cmplx sine_phase = l->args[3];

	cmplx sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
	return l->pos + t * line_vel + sine_ofs;
}

cmplx las_sine_expanding(Laser *l, float t) { // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this is also a "weird" one

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx velocity = l->args[0];
	double amplitude = creal(l->args[1]);
	double frequency = creal(l->args[2]);
	double phase = creal(l->args[3]);

	double angle = carg(velocity);
	double speed = cabs(velocity);

	double s = (frequency * t + phase);
	return l->pos + cexp(I * (angle + amplitude * sin(s))) * t * speed;
}

cmplx las_turning(Laser *l, float t) { // [0] = vel0; [1] = vel1; [2] r: turn begin time, i: turn end time
	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx v0 = l->args[0];
	cmplx v1 = l->args[1];
	float begin = creal(l->args[2]);
	float end = cimag(l->args[2]);

	float a = clamp((t - begin) / (end - begin), 0, 1);
	a = 1.0 - (0.5 + 0.5 * cos(a * M_PI));
	a = 1.0 - pow(1.0 - a, 2);

	cmplx v = v1 * a + v0 * (1 - a);

	return l->pos + v * t;
}

cmplx las_circle(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	// XXX: should turn speed be in rad/sec or rad/frame? currently rad/sec.
	double turn_speed = creal(l->args[0]) / 60;
	double time_ofs = cimag(l->args[0]);
	double radius = creal(l->args[1]);

	return l->pos + radius * cexp(I * (t + time_ofs) * turn_speed);
}

void laser_charge(Laser *l, int t, float charge, float width) {
	float new_width;

	if(t < charge - 10) {
		new_width = fminf(2.0f, 2.0f * t / fminf(30.0f, charge - 10.0f));
	} else if(t >= charge - 10.0f && t < l->deathtime - 20.0f) {
		new_width = fminf(width, 1.7f + width / 20.0f * (t - charge + 10.0f));
	} else if(t >= l->deathtime - 20.0f) {
		new_width = fmaxf(0.0f, width - width / 20.0f * (t - l->deathtime + 20.0f));
	} else {
		new_width = width;
	}

	l->width = new_width;
	l->collision_active = (new_width > width * 0.6f);
}

void laser_make_static(Laser *l) {
	l->speed = 0;
	l->timeshift = l->timespan;
}

void static_laser(Laser *l, int t) {
	if(t == EVENT_BIRTH) {
		l->width = 0;
		l->collision_active = false;
		laser_make_static(l);
		return;
	}

	laser_charge(l, t, creal(l->args[1]), cimag(l->args[1]));
}

DEFINE_EXTERN_TASK(laser_charge) {
	Laser *l = TASK_BIND(ARGS.laser);

	l->width = 0;
	l->collision_active = false;
	laser_make_static(l);

	float target_width = ARGS.target_width;
	float charge_delay = ARGS.charge_delay;

	for(int t = 0;; ++t) {
		laser_charge(l, t, charge_delay, target_width);
		YIELD;
	}
}
