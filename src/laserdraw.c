/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "laserdraw.h"
#include "laser.h"
#include "renderer/api.h"
#include "resource/model.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "video.h"
#include "config.h"
#include "global.h"

/*
 * LASER RENDERING OVERVIEW
 *
 * Laser rendering happens in two passes.
 *
 * In the first pass we quantize the laser into a sequence of connected line segments, possibly
 * simplifying the shape and culling the invisible parts. The segments are then rasterized into a
 * signed distance field. To achieve this, the vertex shader outputs oversized oriented quads for
 * each segment. The amount of size overshoot depends on the maximum range of our distance field, as
 * well as the width of each laser segment. The fragment shader then calculates the signed distance
 * from each fragment to the actual segment. Since the width of a segment may be non-uniform, an
 * "uneven capsule" primitive is used for the distance calculation — effectively, this interpolates
 * the width linearly between the two points of a segment[1].
 *
 * To combine all segments into a signle distance field, the minimum function is used for the
 * blending equation. The framebuffer is initially cleared with the maximum distance value, so that
 * there are no discontinuities or clipping. It is also possible to use a depth buffer to implement
 * this technique, if the blending option is not available.
 *
 * The result is stored into the red channel of a floating point texture, with negative distance
 * denoting fragments that are inside the shape. We also store the negated "time" parameter,
 * linearly interpolated between each point, into the second channel of the texture. This isn't used
 * by default, but opens possibilities for interesting custom effects, including limited texturing.
 *
 * The second pass is a lot simpler: we sample our distance field texture to render a colored laser
 * directly into the main viewport framebuffer. To save some shader invocations, we only render the
 * part covered by the laser's axis-aligned bounding box (computed in the first pass).
 *
 * This method gives us very nice looking, easily tweakable, arbitrarily shaped curves free of
 * artifacts[2] and discontinuities at any resolution!
 *
 * …
 *
 * Now, here is a problem: IT'S SLOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOW.
 *
 * The naive way of implementing it is, anyway. Having to render 2 passes with completely different
 * pipeline states per laser doesn't scale — the back-and-forth switching of states and render
 * targets gets very expensive. So instead of that, let's try to minimize the amount of state
 * changes by cramming as many lasers into one pass as possible.
 *
 * Currently, this is implemented in the most basic way: we take the SDF backing texture and make it
 * much larger, to accomodate multiple viewport-sized areas — we'll call them "tiles". The tiles
 * don't have to be exactly the size of one game viewport, but the proportions matter. In the first
 * pass, we render SDFs for multiple lasers, each into a separate tile. We tag each laser with its
 * tile number, so that the second pass knows where to find their data in the texture. In order to
 * properly scale and constrain the output to the tile area, the viewport transformation is used[3].
 * When we run out of lasers to draw, or out of free tiles in the texture, we render the second pass
 * for the lasers processed so far[4]. If we have any more lasers left to draw, we set up the state
 * for pass one again and start over.
 *
 * With this optimization, the performance gets much more respectable. It's still not as fast as our
 * old (v1.3 era) laser renderer, but hey, it's a lot prettier!
 *
 *
 * OPEN ISSUES
 *
 *	- The tiling method is obviously very wasteful on VRAM. Laser bounding boxes don't typically
 * envelop the entire viewport, so we are wasting a lot of space here. We could use a rect packing
 * algorithm to fit a lot more lasers into a smaller texture. Our current rectpack code is not
 * optimized for real-time use, however.
 *
 *	- Because there are no safe areas between the tiles, the second pass is susceptible to linear
 * filtering artifacts at the edges of the viewport in some cases. This can be fixed at the cost of
 * even more VRAM waste (albeit a little bit) and slightly complicating the tile addressing logic.
 * But the rect packing proposal can also fix this trivially.
 *
 *	- It may be beneficial to separate the curve quantization/simplification code from the renderer.
 * It can be used to optimize collision detection, and we can use the same line segment data to
 * handle collision and rendering.
 *
 *
 *
 * [1] For this reason, we also can't reduce very long runs of straight segments into a single one,
 * or this interpolation becomes too obvious.
 *
 * [2] Ok, there is one small artifact: self-intersections don't quite look right, because there is
 * no way for the SDF to represent a laser stacking on top of itself. But the error is barely
 * noticeable in-game, and could be masked almost entirely with a small amount of blur.
 *
 * [3] Unfortunately, this means we can't dispatch all lasers with a single draw call, as we must
 * change the viewport setup each time. This doesn't seem to be a problem in practice, however.
 *
 * [4] And we can actually do it in a single draw call this time!
 */

// Should be set slightly larger than the
// negated lowest distance threshold value in the sdf_apply shader
#define SDF_RANGE 4.01

#define TILES_X 8
#define TILES_Y 7
// Maximum number of lasers that can be rendered "at once",
// i.e. in one SDF generation pass followed by one coloring pass.
#define TILES_MAX (TILES_X * TILES_Y)

// Maximum amount of segments in one laser
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
} ldraw;

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

typedef struct LaserSamplingParams {
	uint num_samples;
	float time_shift;
	float time_step;
} LaserSamplingParams;

static void laserdraw_ent_predraw_hook(EntityInterface *ent, void *arg);
static void laserdraw_ent_postdraw_hook(EntityInterface *ent, void *arg);

static void laserdraw_sdf_fb_resize_strategy(void *userdata, IntExtent *fb_size, FloatRect *fb_viewport) {
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

	ldraw.pass1.va = va;
	ldraw.pass1.vb = vb;
	ldraw.pass1.vb_stream = r_vertex_buffer_get_stream(vb);

	ldraw.pass1.quad = *r_model_get_quad();
	ldraw.pass1.quad.vertex_array = va;
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

	ldraw.pass2.va = va;
	ldraw.pass2.vb = vb;
	ldraw.pass2.vb_stream = r_vertex_buffer_get_stream(vb);

	ldraw.pass2.quad = *r_model_get_quad();
	ldraw.pass2.quad.vertex_array = va;
}

void laserdraw_preload(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"blur25",
		"blur5",
		"lasers/sdf_apply",
		"lasers/sdf_generate",
	NULL);
}

void laserdraw_init(void) {
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
	fbconf.resize_strategy.resize_func = laserdraw_sdf_fb_resize_strategy;

	ldraw.fb.group = fbmgr_group_create();
	ldraw.fb.sdf = fbmgr_group_framebuffer_create(ldraw.fb.group, "Lasers SDF FB", &fbconf);

	ent_hook_pre_draw(laserdraw_ent_predraw_hook, NULL);
	ent_hook_post_draw(laserdraw_ent_postdraw_hook, NULL);

	ldraw.shaders.sdf_generate = res_shader("lasers/sdf_generate");
	ldraw.shaders.sdf_apply = res_shader("lasers/sdf_apply");
}

void laserdraw_shutdown(void) {
	fbmgr_group_destroy(ldraw.fb.group);
	r_vertex_array_destroy(ldraw.pass1.va);
	r_vertex_array_destroy(ldraw.pass2.va);
	r_vertex_buffer_destroy(ldraw.pass1.vb);
	r_vertex_buffer_destroy(ldraw.pass2.vb);
	ent_unhook_pre_draw(laserdraw_ent_predraw_hook);
	ent_unhook_post_draw(laserdraw_ent_postdraw_hook);
}

static void laserdraw_prepare_state_pass1(void) {
	r_framebuffer(ldraw.fb.sdf);
	r_clear(CLEAR_COLOR, RGBA(SDF_RANGE, 0, 0, 0), 1);
	r_blend(BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDOP_MIN,
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_DST_ALPHA, BLENDOP_MIN
	));
	r_shader_ptr(ldraw.shaders.sdf_generate);
	r_uniform_float("sdf_range", SDF_RANGE);
}

static bool laserdraw_prepare_sampling_params(Laser *l, LaserSamplingParams *out_params) {
	float t;
	int c;

	c = l->timespan;
	t = (global.frames - l->birthtime) * l->speed - l->timespan + l->timeshift;

	if(t + l->timespan > l->deathtime + l->timeshift) {
		c += l->deathtime + l->timeshift - (t + l->timespan);
	}

	if(t < 0) {
		c += t;
		t = 0;
	}

	if(c <= 0) {
		return false;
	}

	float step = 0.5;
	out_params->num_samples = c / step;
	out_params->time_shift = t;
	out_params->time_step = step;

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

static int laserdraw_pass1_build(Laser *l) {
	LaserSamplingParams sp;

	if(!laserdraw_prepare_sampling_params(l, &sp)) {
		return 0;
	}

	// PASS 1: build the signed distance field

	// Precomputed magic parameters for width calculation
	float half_samples = sp.num_samples * 0.5;
	float tail = sp.num_samples / 1.6;
	float width_factor = -1 / (tail * tail);

	// Maximum value of `1 - cos(angle)` between two curve segments to reduce to straight lines
	const float thres_angular = 1e-4;
	// Maximum laser-time sample difference between two segment points (for width interpolation)
	const float thres_temporal = sp.num_samples / 16.0;
	// These values should be kept as high as possible without introducing artifacts.

	// Number of segments generated
	int segments = 0;

	// Time value of current sample
	float t = sp.time_shift;

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
	t += sp.time_step;

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

	for(uint i = 1; i < sp.num_samples; ++i, t += sp.time_step) {
		b = l->prule(l, t);

		if(i < sp.num_samples - 1 && (t - t0) < thres_temporal) {
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

			SDL_RWwrite(ldraw.pass1.vb_stream, &attr, sizeof(attr), 1);
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

static void laserdraw_pass1_render(Laser *l, int segments) {
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
	r_draw_model_ptr(&ldraw.pass1.quad, segments, 0);
	r_vertex_buffer_invalidate(ldraw.pass1.vb);
	r_framebuffer_viewport_rect(fb, vp_orig);
}

static void laserdraw_prepare_state_pass2(void) {
	Texture *tex = r_framebuffer_get_attachment(ldraw.fb.sdf, FRAMEBUFFER_ATTACH_COLOR0);
	r_shader_ptr(ldraw.shaders.sdf_apply);
	r_uniform_sampler("tex", tex);
	r_uniform_vec2("texsize", VIEWPORT_W * TILES_X, VIEWPORT_H * TILES_Y);
	r_blend(BLEND_PREMUL_ALPHA);
}

static void laserdraw_pass2_build(Laser *l) {
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

	SDL_RWwrite(ldraw.pass2.vb_stream, &attrs, sizeof(attrs), 1);
}

static void laserdraw_pass2_renderall(void) {
	if(!ldraw.render_state.batch.first) {
		return;
	}

	r_state_push();
	r_framebuffer(ldraw.fb.saved);
	laserdraw_prepare_state_pass2();

	int instances = 0;

	for(;;) {
		LaserRenderData *rd = ldraw.render_state.batch.last;

		if(rd == NULL) {
			break;
		}

		alist_unlink(&ldraw.render_state.batch, rd);
		Laser *l = LASER_FROM_RENDERDATA(rd);
		laserdraw_pass2_build(l);
		++instances;
	}

	assert(instances > 0);
	r_draw_model_ptr(&ldraw.pass2.quad, instances, 0);
	r_vertex_buffer_invalidate(ldraw.pass2.vb);

	r_state_pop();

	ldraw.render_state.rendered_lasers += instances;
	ldraw.render_state.pass1_state_ready = false;
	ldraw.render_state.free_tile = 0;
	assert(ldraw.render_state.batch.first == NULL);
}

void laserdraw_ent_drawfunc(EntityInterface *ent) {
	Laser *laser = ENT_CAST(ent, Laser);

	int segments = laserdraw_pass1_build(laser);

	if(segments <= 0) {
		return;
	}

	if(ldraw.render_state.free_tile == TILES_MAX) {
		// No more space in the sdf framebuffer — flush pending lasers
		laserdraw_pass2_renderall();
	}

	assert(ldraw.render_state.free_tile < TILES_MAX);
	laser->_renderdata.tile = ldraw.render_state.free_tile++;
	alist_push(&ldraw.render_state.batch, &laser->_renderdata);

	if(!ldraw.render_state.pass1_state_ready) {
		// Pop out of entity-wide state into lasers-wide state
		r_state_pop();

		laserdraw_prepare_state_pass1();
		ldraw.render_state.pass1_state_ready = true;

		// Push new entity-wide state; entity draw loop will pop it
		r_state_push();
	}

	laserdraw_pass1_render(laser, segments);
}

static void laserdraw_begin(void) {
	if(!ldraw.render_state.drawing_lasers) {
		ldraw.fb.saved = r_framebuffer_current();
		r_state_push();
		ldraw.render_state.drawing_lasers = true;
	}
}

static void laserdraw_end(void) {
	if(ldraw.render_state.drawing_lasers) {
		r_state_pop();
		ldraw.render_state.drawing_lasers = false;
	}

	laserdraw_pass2_renderall();
}

static void laserdraw_ent_predraw_hook(EntityInterface *ent, void *arg) {
	if(!ent) {
		return;
	}

	if(ent->type == ENT_TYPE_ID(Laser)) {
		laserdraw_begin();
	} else {
		laserdraw_end();
	}
}

static void laserdraw_ent_postdraw_hook(EntityInterface *ent, void *arg) {
	if(ent == NULL) {
		// Handle edge case when the last entity drawn is a laser
		laserdraw_end();
	}
}
