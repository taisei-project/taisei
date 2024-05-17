/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "draw.h"
#include "laser.h"
#include "internal.h"

#include "config.h"
#include "memory/allocator.h"
#include "memory/arena.h"
#include "renderer/api.h"
#include "util.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "util/rectpack.h"
#include "video.h"

/*
 * LASER RENDERING OVERVIEW
 *
 * Laser rendering happens in two passes.
 *
 * In the first pass we quantize the laser into a sequence of connected line segments, possibly
 * simplifying the shape and culling the invisible parts[1]. The segments are then rasterized into
 * a signed distance field. To achieve this, the vertex shader outputs oversized oriented quads for
 * each segment. The amount of size overshoot depends on the maximum range of our distance field,
 * as well as the width of each laser segment. The fragment shader then calculates the signed
 * distance from each fragment to the actual segment. Since the width of a segment may be non-
 * uniform, an "uneven capsule" primitive is used for the distance calculation — effectively, this
 * interpolates the width linearly between the two points of a segment[2].
 *
 * To combine all segments into a single distance field, the minimum function is used for the
 * blending equation. The framebuffer is initially cleared with the maximum distance value, so that
 * there are no discontinuities or clipping. It is also possible to use a depth buffer to implement
 * this technique, if the blending option is not available.
 *
 * The result is stored into the red channel of a floating point texture, with negative distance
 * denoting fragments that are inside the shape. We also store the negated "time" parameter,
 * linearly interpolated between each point, into the second channel of the texture. This isn't
 * used by default, but opens possibilities for interesting custom effects, including limited
 * texturing.
 *
 * The second pass is a lot simpler: we sample our distance field texture to render a colored laser
 * directly into the main viewport framebuffer. To save some shader invocations, we only render the
 * part covered by the laser's axis-aligned bounding box.
 *
 * This method gives us very nice looking, easily tweakable, arbitrarily shaped curves free of
 * artifacts[3] and discontinuities at any resolution!
 *
 * …
 *
 * Now, here is a problem: IT'S SLOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOW.
 *
 * The naive way of implementing it is, anyway. Having to render 2 passes with completely different
 * pipeline states per laser doesn't scale — the back-and-forth switching of states and render
 * targets gets very expensive. So instead of that, let's try to minimize the amount of state
 * changes by cramming as many lasers into one pass as possible.
 *
 * First of all, we make the SDF backing texture considerably larger, so that it can accommodate
 * many lasers at once, packed into it in a non-overlapping way. A Guillotine 2D rect packing
 * algorithm is used for this implementation (see util/rectpack.c). It's not the most
 * space-efficient algorithm around, but it's pretty fast and does the job just fine here.
 *
 * We don't directly render lasers in their draw callbacks, instead we enqueue them for the next
 * draw batch. This involves allocating space for the laser's bounding box on the SDF texture as
 * described above, and generating its vertex data for both passes. If we ran out of free space, we
 * flush the batch first: render both passes with the batch data we've collected so far, invalidate
 * the vertex buffers, and start the next batch. We also flush the batch when there are no more
 * lasers to draw.
 *
 * With this optimization, the performance gets much more respectable. It's still not as fast as
 * our old (v1.3 era) laser renderer, but hey, it's a lot prettier!
 *
 * ————————————————————————————————————————————————————————————————————————————————————————————————
 *
 * [1] This quantization and culling stage actually happens in laser.c (see quantize_laser()),
 * because this data is used for collision detection as well. The laser's axis-aligned bounding box
 * is also trivially computed at this stage.
 *
 * [2] For this reason, we also can't reduce very long runs of straight segments into a single one,
 * or this interpolation becomes too obvious.
 *
 * [3] Ok, there is one small artifact: self-intersections don't quite look right, because there is
 * no way for the SDF to represent a laser stacking on top of itself. But the error is barely
 * noticeable in-game, and could be masked almost entirely with a small amount of blur.
 */

// Size of the SDF texture to pack lasers into. This is in game units.
// Must be at least about ~1.2x the viewport size to accommodate all kinds of lasers.
// Ideally should be large enough so that all lasers in a single frame can fit into it, but for
// some pathological cases we expect to render more than one pass.
#define PACKING_SPACE_SIZE   2048
#define PACKING_SPACE_SIZE_W PACKING_SPACE_SIZE
#define PACKING_SPACE_SIZE_H PACKING_SPACE_SIZE

// Hints for how much vertex buffer space to allocate upfront.
// The buffers will be dynamically resized on demand.
#define EXPECTED_MAX_SEGMENTS 512
#define EXPECTED_MAX_LASERS 64

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
		Framebuffer *sdf;
		ManagedFramebufferGroup *group;
	} fb;

	struct {
		MemArena arena;
		Allocator alloc;
		RectPack rectpack;
	} packer;

	struct {
		int pass1_num_segments;
		int pass2_num_lasers;
		bool drawing_lasers;
	} render_state;

	DYNAMIC_ARRAY(Laser*) queue;
} ldraw;

typedef LaserSegment LaserInstancedAttribsPass1;

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

static void laserdraw_ent_predraw_hook(EntityInterface *ent, void *arg);
static void laserdraw_ent_postdraw_hook(EntityInterface *ent, void *arg);

static void laserdraw_sdf_fb_resize_strategy(void *userdata, IntExtent *fb_size, FloatRect *fb_viewport) {
	float w, h;
	float vid_vp_w, vid_vp_h;
	video_get_viewport_size(&vid_vp_w, &vid_vp_h);
	float q = config_get_float(CONFIG_FG_QUALITY);
	float factor = clamp(q * vid_vp_w / SCREEN_W, 0.5f, 1.0f);

	w = roundf(PACKING_SPACE_SIZE_W * factor);
	h = roundf(PACKING_SPACE_SIZE_H * factor);

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

	VertexBuffer *vb = r_vertex_buffer_create(sz_attr * EXPECTED_MAX_SEGMENTS, NULL);
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

	VertexBuffer *vb = r_vertex_buffer_create(sz_attr * EXPECTED_MAX_LASERS, NULL);
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

void laserdraw_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"lasers/sdf_apply",
		"lasers/sdf_generate",
	NULL);
}

static void laserdraw_init_packer(void) {
	rectpack_init(&ldraw.packer.rectpack, &ldraw.packer.alloc,
		PACKING_SPACE_SIZE_W, PACKING_SPACE_SIZE_H);
}

static void laserdraw_reset_packer(void) {
	marena_reset(&ldraw.packer.arena);
	laserdraw_init_packer();
}

void laserdraw_init(void) {
	marena_init(&ldraw.packer.arena, 0);
	allocator_init_from_arena(&ldraw.packer.alloc, &ldraw.packer.arena);
	laserdraw_init_packer();
	dynarray_ensure_capacity(&ldraw.queue, 64);

	create_pass1_resources();
	create_pass2_resources();

	FBAttachmentConfig aconf = { 0 };
	aconf.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	aconf.tex_params.type = TEX_TYPE_RG_16_FLOAT;
	aconf.tex_params.filter.min = TEX_FILTER_LINEAR;
	aconf.tex_params.filter.mag = TEX_FILTER_LINEAR;
	aconf.tex_params.wrap.s = TEX_WRAP_REPEAT;
	aconf.tex_params.wrap.t = TEX_WRAP_REPEAT;

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
	dynarray_free_data(&ldraw.queue);
	allocator_deinit(&ldraw.packer.alloc);
	marena_deinit(&ldraw.packer.arena);
	fbmgr_group_destroy(ldraw.fb.group);
	r_vertex_array_destroy(ldraw.pass1.va);
	r_vertex_array_destroy(ldraw.pass2.va);
	r_vertex_buffer_destroy(ldraw.pass1.vb);
	r_vertex_buffer_destroy(ldraw.pass2.vb);
	ent_unhook_pre_draw(laserdraw_ent_predraw_hook);
	ent_unhook_post_draw(laserdraw_ent_postdraw_hook);
}

static cmplxf laser_packed_dimensions(Laser *l) {
	FloatExtent bbox_size = {
		.as_cmplx = l->_internal.bbox.bottom_right.as_cmplx - l->_internal.bbox.top_left.as_cmplx,
	};

	// Align to texel boundaries and add a small gap to prevent sampling artifacts from lasers that
	// happen to be right next to each other in the texture.
	bbox_size.w = ceilf(bbox_size.w + 0.5f);
	bbox_size.h = ceilf(bbox_size.h + 0.5f);

	return bbox_size.as_cmplx;
}

// Allocate a free region in the SDF texture for the laser's bbox.
// Returns false on failure (not enough free space).
// Returns true on success and sets out_ofs to the offset to apply to the laser's points
// for rendering into (or sampling from) the SDF texture, such that the whole laser is contained
// in the allocated region.
static bool laserdraw_pack_laser(Laser *l, cmplxf *out_ofs, bool *rotated) {
	FloatExtent bbox_size = { .as_cmplx = laser_packed_dimensions(l) };

	RectPackSection *section = rectpack_add(
		&ldraw.packer.rectpack, bbox_size.w, bbox_size.h, true);

	if(!section) {
		return false;
	}

	*out_ofs = section->rect.top_left - (cmplxf)l->_internal.bbox.top_left.as_cmplx;

	FloatExtent packed_size = { .as_cmplx = section->rect.bottom_right - section->rect.top_left };

	*rotated = (
		bbox_size.w == packed_size.h &&
		bbox_size.h == packed_size.w &&
		bbox_size.w != bbox_size.h
	);

	return true;
}

// Add laser to batch for SDF generation pass
static void laserdraw_pass1_add(Laser *l, cmplxf sdf_ofs, bool rotated) {
	int iofs = l->_internal.segments_ofs;
	int nsegs = l->_internal.num_segments;
	assert(nsegs > 0);

	cmplxf section_origin = sdf_ofs + (cmplxf)l->_internal.bbox.top_left.as_cmplx;

	for(int i = iofs; i < nsegs + iofs; ++i) {
		LaserSegment s = dynarray_get(&lintern.segments, i);

		s.pos.a += sdf_ofs;
		s.pos.b += sdf_ofs;

		if(rotated) {
			s.pos.a = section_origin + cswapf(s.pos.a - section_origin);
			s.pos.b = section_origin + cswapf(s.pos.b - section_origin);
		}

		SDL_RWwrite(ldraw.pass1.vb_stream, &s, sizeof(s), 1);
	}

	ldraw.render_state.pass1_num_segments += nsegs;
}

// Add laser to batch for coloring pass
static void laserdraw_pass2_add(Laser *l, cmplxf sdf_ofs, bool rotated) {
	FloatOffset top_left = l->_internal.bbox.top_left;
	FloatOffset bottom_right = l->_internal.bbox.bottom_right;
	cmplxf bbox_midpoint = (top_left.as_cmplx + bottom_right.as_cmplx) * 0.5f;
	cmplxf bbox_size = bottom_right.as_cmplx - top_left.as_cmplx;

	FloatRect bbox;
	bbox.offset.as_cmplx = bbox_midpoint;
	bbox.extent.as_cmplx = bbox_size;

	FloatRect frag = bbox;
	frag.offset.as_cmplx += sdf_ofs - bbox.extent.as_cmplx * 0.5;

	if(rotated) {
		// random HACK to encode the rotation bit without adding an attribute
		bbox.extent.w = -bbox.extent.w;
	}

	LaserInstancedAttribsPass2 attrs = {
		.bbox = bbox,
		.texture_ofs = frag.offset,
		.color = l->color.color3,
		.width = l->width,
	};

	SDL_RWwrite(ldraw.pass2.vb_stream, &attrs, sizeof(attrs), 1);
	++ldraw.render_state.pass2_num_lasers;
}

// Returns false if unable to pack.
static bool laserdraw_add(Laser *l) {
	if(l->_internal.num_segments < 1) {
		// This can happen if the laser is totally out of bounds so all segments have been
		// culled during quantization.
		return true;
	}

	cmplxf sdf_ofs;
	bool rotated;

	if(!laserdraw_pack_laser(l, &sdf_ofs, &rotated)) {
		return false;
	}

	laserdraw_pass1_add(l, sdf_ofs, rotated);
	laserdraw_pass2_add(l, sdf_ofs, rotated);

	return true;
}

// Render the SDF generation pass
static void laserdraw_pass1_render(void) {
	r_state_push();
	r_framebuffer(ldraw.fb.sdf);
	r_clear(BUFFER_COLOR, RGBA(LASER_SDF_RANGE, 0, 0, 0), 1);
	r_blend(BLENDMODE_COMPOSE(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR, BLENDOP_MIN,
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_DST_ALPHA, BLENDOP_MIN
	));
	r_shader_ptr(ldraw.shaders.sdf_generate);
	r_uniform_float("sdf_range", LASER_SDF_RANGE);
	r_mat_proj_push_ortho(PACKING_SPACE_SIZE_W, PACKING_SPACE_SIZE_H);
	r_draw_model_ptr(&ldraw.pass1.quad, ldraw.render_state.pass1_num_segments, 0);
	r_mat_proj_pop();
	r_state_pop();
}

// Render the coloring pass
static void laserdraw_pass2_render(void) {
	r_state_push();
	r_shader_ptr(ldraw.shaders.sdf_apply);
	Texture *tex = r_framebuffer_get_attachment(ldraw.fb.sdf, FRAMEBUFFER_ATTACH_COLOR0);
	r_uniform_sampler("tex", tex);
	r_uniform_vec2("texsize", PACKING_SPACE_SIZE_W, PACKING_SPACE_SIZE_H);
	r_blend(BLEND_PREMUL_ALPHA);
	r_draw_model_ptr(&ldraw.pass2.quad, ldraw.render_state.pass2_num_lasers, 0);
	r_state_pop();
}

// Render the two passes and reset batch state
static void laserdraw_flush(void) {
	// Sanity check: if we have no lasers, we must have no segments.
	// If we have at least one laser, we must have at least one segment.

	if(!ldraw.render_state.pass2_num_lasers) {
		assert(!ldraw.render_state.pass1_num_segments);
		return;
	}

	assert(ldraw.render_state.pass1_num_segments);

	laserdraw_pass1_render();
	r_vertex_buffer_invalidate(ldraw.pass1.vb);
	laserdraw_pass2_render();
	r_vertex_buffer_invalidate(ldraw.pass2.vb);

	ldraw.render_state.pass1_num_segments = 0;
	ldraw.render_state.pass2_num_lasers = 0;

	laserdraw_reset_packer();
}

void laserdraw_ent_drawfunc(EntityInterface *ent) {
	*dynarray_append(&ldraw.queue) = ENT_CAST(ent, Laser);
}

static float laser_comparator(Laser *l) {
	cmplxf dim = laser_packed_dimensions(l);
	return -cabs2f(dim);
}

static int laser_compare(const void *a, const void *b) {
	auto va = laser_comparator(*(Laser**)a);
	auto vb = laser_comparator(*(Laser**)b);
	return ((va > vb) - (vb > va));
}

static void laserdraw_commit(void) {
	if(ldraw.queue.num_elements < 1) {
		return;
	}

	dynarray_qsort(&ldraw.queue, laser_compare);

	r_state_push();

	dynarray_foreach_elem(&ldraw.queue, Laser **lp, {
		bool ok = laserdraw_add(*lp);

		if(!ok) {
			// No more space in the SDF framebuffer
			// render current batch and queue for next one.
			laserdraw_flush();
			ok = laserdraw_add(*lp);
			assert(ok);
		}
	});

	laserdraw_flush();
	r_state_pop();

	ldraw.queue.num_elements = 0;
}

static void laserdraw_ent_predraw_hook(EntityInterface *ent, void *arg) {
	if(ent && ent->type != ENT_TYPE_ID(Laser)) {
		laserdraw_commit();
	}
}

static void laserdraw_ent_postdraw_hook(EntityInterface *ent, void *arg) {
	if(ent == NULL) {
		// Handle edge case when the last entity drawn is a laser
		laserdraw_commit();
	}
}
