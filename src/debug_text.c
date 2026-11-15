/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "debug_text.h"

#ifdef ENABLE_DEBUG_TEXT

#include "memory/arena.h"
#include "renderer/api.h"
#include "resource/font.h"
#include "util/env.h"
#include "util/strbuf.h"
#include "video.h"

#define GROUPS 32

#define SHADER "text_default"
#define FONT   "mono"

#define FILTER_SEPARATORS ",; \t\n"

typedef struct TagFilter {
	uint size;
	bool invert;
	char pattern[];
} TagFilter;

static struct {
	MemArena arena;
	StringBuffer buffer;
	Font *font;
	ShaderProgram *shader;
	ResourceGroup rg;
	int commited_ofs;
	float inv_scale;
	DYNAMIC_ARRAY(TagFilter*) filters;
} G;

static void debug_text_add_filter(const char *filter) {
	bool invert = false;

	if(filter[0] == '-') {
		invert = true;
		++filter;
	}

	auto f = ALLOC_FLEX(TagFilter, strlen(filter) + 1);
	f->invert = invert;
	f->size = strlen(filter);
	memcpy(f->pattern, filter, strlen(filter) + 1);
	dynarray_append(&G.filters, f);
}

static void debug_text_init_filters(void) {
	char *file_filters = SDL_LoadFile_IO(vfs_open("storage/debug_text_filter", VFS_MODE_READ), NULL, true);
	char *filter;

	if(file_filters) {
		char *tmp = file_filters;

		while((filter = strtok_r(NULL, FILTER_SEPARATORS, &tmp))) {
			debug_text_add_filter(filter);
		}

		SDL_free(file_filters);
	}

	char *filters_expr = marena_strdup(&G.arena, env_get_string("TAISEI_DEBUG_TEXT", ""));

	while((filter = strtok_r(NULL, FILTER_SEPARATORS, &filters_expr))) {
		debug_text_add_filter(filter);
	}
}

void (debug_text_init)(void) {
	marena_init(&G.arena, 1024);
	G.buffer = STRBUF(&G.arena);

	res_group_init(&G.rg);
	res_group_preload(&G.rg, RES_SHADER_PROGRAM, RESF_DEFAULT, SHADER, NULL);
	res_group_preload(&G.rg, RES_FONT, RESF_DEFAULT, FONT, NULL);

	G.inv_scale = 1.0 / max(0.25, env_get_double("TAISEI_DEBUG_TEXT_SCALE", 0.5));

	auto snap = marena_snapshot(&G.arena);
	debug_text_init_filters();
	marena_rollback(&G.arena, &snap);
}

void (debug_text_shutdown)(void) {
	dynarray_foreach_elem(&G.filters, TagFilter **pfilter, {
		mem_free(*pfilter);
	});

	dynarray_free_data(&G.filters);
	strbuf_clear(&G.buffer);
	marena_deinit(&G.arena);
	res_group_release(&G.rg);
	G = (typeof(G)) {};
}

void (debug_text_draw)(void) {
	if(!G.buffer.start) {
		return;
	}

	if(!G.shader) {
		G.shader = res_shader(SHADER);
	};

	if(!G.font) {
		G.font = res_font(FONT);
	}

	r_flush_sprites();

	FloatRect vp;
	video_get_viewport(&vp);
	vp.w += vp.x * 2;
	vp.x = 0;

	r_state_push();

	auto fb = r_framebuffer_current();
	FloatRect old_vp;
	r_framebuffer_viewport_current(fb, &old_vp);
	r_framebuffer_viewport_rect(fb, vp);

	real ar = (vp.w / vp.h) / (SCREEN_W / SCREEN_H);
	// r_mat_proj_push_ortho(vp.w * G.inv_scale, vp.h * G.inv_scale);
	r_mat_proj_push_ortho(ar * SCREEN_W * G.inv_scale, SCREEN_H * G.inv_scale);

	text_draw(G.buffer.start, &(TextParams) {
		.font_ptr = G.font,
		.shader_ptr = G.shader,
		.color = { 1, 1, 1, 1 },
		.pos = { 0, font_get_lineskip(G.font) + font_get_descent(G.font) },
		.align = ALIGN_LEFT,
		.blend = BLEND_PREMUL_ALPHA,
	});

	r_flush_sprites();

	r_mat_proj_pop();
	r_framebuffer_viewport_rect(fb, old_vp);
	r_state_pop();

	debug_text_clear();
}

void (debug_text_rollback)(void) {
	if(!G.buffer.start) {
		return;
	}

	G.buffer.pos = G.buffer.start + G.commited_ofs;
	*G.buffer.pos = '\0';
}

void (debug_text_commit)(void) {
	G.commited_ofs = strbuf_ofs(&G.buffer);
}

void (debug_text_clear)(void) {
	strbuf_clear(&G.buffer);
	marena_reset(&G.arena);
	G.commited_ofs = 0;
}

bool (debug_text_check_filter)(const char *file, const char *func, int line) {
	char tag[128];
	size_t func_len = strlen(func);
	size_t end = log_util_module_name(file, strlen(file), tag, sizeof(tag) - func_len - 1);
	tag[end] = ':';
	memcpy(tag + end + 1, func, func_len + 1);
	size_t tag_len = end + func_len + 1;

	bool enabled = false;

	dynarray_foreach_elem(&G.filters, TagFilter **pfilter, {
		auto f = *pfilter;
		if(strnmatch(f->size, f->pattern, tag_len, tag)) {
			enabled = !f->invert;
		}
	});

	if(!enabled) {
		log_debug("Tag %s suppressed at %s:%i %s (reported once)", tag, file, line, func);
	}

	return enabled;
}

void (debug_text_raw)(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	strbuf_vprintf(&G.buffer, fmt, va);
	strbuf_cat(&G.buffer, "\n");
	va_end(va);
}

#endif
