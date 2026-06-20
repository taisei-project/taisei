/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "sprite.h"
#include "atlas.h"

#include "memory/memory.h"
#include "memory/scratch.h"
#include "renderer/api.h"
#include "util/kvparser.h"

static char *sprite_path(const char *name) {
	StringBuffer buf = { acquire_scratch_arena() };
	strbuf_cat(&buf, SPRITE_PATH_PREFIX);
	strbuf_cat(&buf, name);
	strbuf_cat(&buf, SPRITE_EXTENSION);

	VFSInfo pinfo = vfs_query(buf.start);

	if(!pinfo.exists) {
		release_scratch_arena(buf.arena);
		return texture_res_handler.procs.find(name);
	}

	char *path = mem_strdup(buf.start);
	release_scratch_arena(buf.arena);
	return path;
}

static bool check_sprite_path(const char *path) {
	return strendswith(path, SPRITE_EXTENSION) || texture_res_handler.procs.check(path);
}

struct sprite_load_state {
	Sprite *spr;
	char *texture_name;
	char *atlas_name;
};

static void load_sprite_stage1(ResourceLoadState *st);
static void load_sprite_stage2_atlas(ResourceLoadState *st);
static void load_sprite_stage2_legacy(ResourceLoadState *st);

static void load_sprite_stage1(ResourceLoadState *st) {
	auto spr = ALLOC(Sprite, {
		.tex_area = { .offset = { 0, 0 }, .extent = { 1, 1 } },
	});
	auto state = ALLOC(struct sprite_load_state, { .spr = spr });

	if(texture_res_handler.procs.check(st->path)) {
		state->texture_name = mem_strdup(st->name);
		res_load_dependency(st, RES_TEXTURE, state->texture_name);
		res_load_continue_after_dependencies(st, load_sprite_stage2_legacy, state);
		return;
	}

	SDL_IOStream *rw = res_open_file(st, st->path, VFS_MODE_READ);

	if(UNLIKELY(!rw)) {
		log_error("VFS error: %s", vfs_get_error());
		mem_free(spr);
		mem_free(state);
		res_load_failed(st);
		return;
	}

	struct { float top, bottom, left, right; } pad = { };

	bool parsed = parse_keyvalue_stream_with_spec(rw, (KVSpec[]) {
		{ "atlas",          .out_str   = &state->atlas_name },
		{ "texture",        .out_str   = &state->texture_name },
		{ "region_x",       .out_float = &spr->tex_area.x },
		{ "region_y",       .out_float = &spr->tex_area.y },
		{ "region_w",       .out_float = &spr->tex_area.w },
		{ "region_h",       .out_float = &spr->tex_area.h },
		{ "w",              .out_float = &spr->w },
		{ "h",              .out_float = &spr->h },
		{ "padding_top",    .out_float = &pad.top },
		{ "padding_bottom", .out_float = &pad.bottom },
		{ "padding_left",   .out_float = &pad.left },
		{ "padding_right",  .out_float = &pad.right },
		{}
	});

	SDL_CloseIO(rw);

	if(UNLIKELY(!parsed)) {
		mem_free(spr);
		mem_free(state->texture_name);
		mem_free(state->atlas_name);
		mem_free(state);
		log_error("Failed to parse sprite file '%s'", st->path);
		res_load_failed(st);
		return;
	}

	if(state->atlas_name) {
		if(state->texture_name) {
			log_warn("%s: \"texture\" specified together with \"atlas\", ignoring", st->name);
			mem_free(state->texture_name);
		}

		res_load_dependency(st, RES_ATLAS, state->atlas_name);
		res_load_continue_after_dependencies(st, load_sprite_stage2_atlas, state);
		return;
	}

	if(!state->texture_name) {
		state->texture_name = mem_strdup(st->name);
		log_info("%s: inferred texture name from sprite name", state->texture_name);
	}

	res_load_dependency(st, RES_TEXTURE, state->texture_name);

	spr->padding.extent.w = pad.left + pad.right;
	spr->padding.extent.h = pad.top + pad.bottom;

	spr->padding.offset.x = 0.5f * (pad.left - pad.right);
	spr->padding.offset.y = 0.5f * (pad.top - pad.bottom);

	spr->extent.as_cmplx += spr->padding.extent.as_cmplx;

	res_load_continue_after_dependencies(st, load_sprite_stage2_legacy, state);
	return;
}

static void load_sprite_stage2_atlas(ResourceLoadState *st) {
	struct sprite_load_state *state = NOT_NULL(st->opaque);
	Sprite *spr = NOT_NULL(state->spr);
	Atlas *atlas = res_get_data(RES_ATLAS, state->atlas_name, st->flags & ~RESF_RELOAD);
	bool fail = false;

	if(atlas) {
		if(!atlas_get_sprite(atlas, st->name, spr)) {
			log_error("No sprite \"%s\" in atlas \"%s\"", st->name, state->atlas_name);
			fail = true;
		}
	} else {
		fail = true;
	}

	mem_free(state->atlas_name);
	mem_free(state);

	if(fail) {
		mem_free(spr);
		res_load_failed(st);
	} else {
		res_load_finished(st, spr);
	}
}

static void load_sprite_stage2_legacy(ResourceLoadState *st) {
	struct sprite_load_state *state = NOT_NULL(st->opaque);
	Sprite *spr = NOT_NULL(state->spr);

	spr->tex = res_get_data(RES_TEXTURE, state->texture_name, st->flags & ~RESF_RELOAD);

	mem_free(state->texture_name);
	mem_free(state);

	if(spr->tex == NULL) {
		mem_free(spr);
		res_load_failed(st);
		return;
	}

	FloatExtent denorm_coords = sprite_denormalized_tex_coords(spr).extent;

	struct infermap {
		const char *dstname;
		float *dst;
		const char *srcname;
		float *src;
	} infermap[] = {
		{ "sprite width",  &spr->w, "texture region width",  &denorm_coords.w },
		{ "sprite height", &spr->h, "texture region height", &denorm_coords.h },
		{},
	};

	for(struct infermap *m = infermap; m->dst; ++m) {
		if(*m->dst <= 0) {
			*m->dst = *m->src;
			log_info("%s: inferred %s from %s (%g)", st->name, m->dstname, m->srcname, *m->src);
		}
	}

	res_load_finished(st, spr);
}

Sprite *prefix_get_sprite(const char *name, const char *prefix) {
	uint plen = strlen(prefix);
	uint nlen = strlen(name);
	char buf[plen + nlen + 1];
	memcpy(buf, prefix, plen);
	memcpy(buf + plen, name, nlen + 1);
	Sprite *spr = res_sprite(buf);
	return spr;
}

static void begin_draw_sprite(float x, float y, float scale_x, float scale_y, Sprite *spr) {
	FloatOffset o = spr->padding.offset;

	begin_draw_texture(
		(FloatRect){ x + o.x * scale_x, y + o.y * scale_y, spr->w * scale_x, spr->h * scale_y },
		sprite_denormalized_tex_coords(spr),
		spr->tex
	);
}

static void end_draw_sprite(void) {
	end_draw_texture();
}

static void draw_sprite_ex(float x, float y, float scale_x, float scale_y, Sprite *spr) {
	begin_draw_sprite(x, y, scale_x, scale_y, spr);
	r_draw_quad();
	end_draw_sprite();
}

void draw_sprite(float x, float y, const char *name) {
	draw_sprite_p(x, y, res_sprite(name));
}

void draw_sprite_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, spr);
}

FloatRect sprite_denormalized_tex_coords(const Sprite *restrict spr) {
	Texture *tex = NOT_NULL(spr->tex);
	uint tex_w, tex_h;
	r_texture_get_size(tex, 0, &tex_w, &tex_h);
	FloatRect tc = spr->tex_area;
	tc.x *= tex_w;
	tc.y *= tex_h;
	tc.w *= tex_w;
	tc.h *= tex_h;
	return tc;
}

IntRect sprite_denormalized_int_tex_coords(const Sprite *restrict spr) {
	FloatRect tc = sprite_denormalized_tex_coords(spr);
	IntRect itc;
	itc.x = lroundf(tc.x);
	itc.y = lroundf(tc.y);
	itc.w = lroundf(tc.w);
	itc.h = lroundf(tc.h);
	return itc;
}

void sprite_set_denormalized_tex_coords(Sprite *restrict spr, FloatRect tc) {
	Texture *tex = NOT_NULL(spr->tex);
	uint tex_w, tex_h;
	r_texture_get_size(tex, 0, &tex_w, &tex_h);
	spr->tex_area.x = tc.x / tex_w;
	spr->tex_area.y = tc.y / tex_h;
	spr->tex_area.w = tc.w / tex_w;
	spr->tex_area.h = tc.h / tex_h;
}

static bool transfer_sprite(void *dst, void *src) {
	*(Sprite*)dst = *(Sprite*)src;
	mem_free(src);
	return true;
}

ResourceHandler sprite_res_handler = {
	.type = RES_SPRITE,
	.typename = "sprite",
	.subdir = SPRITE_PATH_PREFIX,

	.procs = {
		.find = sprite_path,
		.check = check_sprite_path,
		.load = load_sprite_stage1,
		.unload = mem_free,
		.transfer = transfer_sprite,
	},
};
