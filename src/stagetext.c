/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagetext.h"

#include "global.h"
#include "stageobjects.h"

static StageText *textlist = NULL;

StageText *stagetext_add(const char *text, cmplx pos, Alignment align, Font *font, const Color *clr, int delay, int lifetime, int fadeintime, int fadeouttime) {
	auto t = list_append(&textlist, STAGE_ACQUIRE_OBJ(StageText));

	if(text != NULL) {
		assert(strlen(text) < sizeof(t->text));
		strcpy(t->text, text);
	}

	t->font = font;
	t->pos = pos;
	t->align = align;
	t->color = *clr;

	t->time.spawn = global.frames + delay;
	t->time.fadein = fadeintime;
	t->time.fadeout = fadeouttime;
	t->time.life = lifetime + fadeouttime;

	return t;
}

static void stagetext_numeric_update(StageText *txt, int t, float a) {
	format_huge_num(0, (uintptr_t)txt->custom.data1 * pow(a, 5), sizeof(txt->text), txt->text);
}

StageText *stagetext_add_numeric(int n, cmplx pos, Alignment align, Font *font, const Color *clr, int delay, int lifetime, int fadeintime, int fadeouttime) {
	auto t = stagetext_add(NULL, pos, align, font, clr, delay, lifetime, fadeintime, fadeouttime);
	t->custom.data1 = (void*)(intptr_t)n;
	t->custom.update = stagetext_numeric_update;
	return t;
}

static void *stagetext_delete(List **dest, List *txt, void *arg) {
	STAGE_RELEASE_OBJ(list_unlink(dest, (StageText*)txt));
	return NULL;
}

void stagetext_free(void) {
	list_foreach(&textlist, stagetext_delete, NULL);
}

static inline float stagetext_alpha(StageText *txt) {
	int t = global.frames - txt->time.spawn;
	return clamp((txt->time.life - t) / (float)txt->time.fadeout, 0, clamp(t / (float)txt->time.fadein, 0, 1));
}

static void stagetext_update_single(StageText *txt) {
	if(global.frames < txt->time.spawn) {
		return;
	}

	if(global.frames > txt->time.spawn + txt->time.life) {
		stagetext_delete((List**)&textlist, (List*)txt, NULL);
		return;
	}

	if(txt->custom.update) {
		txt->custom.update(txt, global.frames - txt->time.spawn, stagetext_alpha(txt));
	}
}

static void stagetext_draw_single(StageText *txt) {
	if(global.frames < txt->time.spawn) {
		return;
	}

	if(global.frames > txt->time.spawn + txt->time.life) {
		log_warn("FIXME: deleting stagetext [%s] in draw function", txt->text);
		stagetext_delete((List**)&textlist, (List*)txt, NULL);
		return;
	}

	int t = global.frames - txt->time.spawn;
	float f = 1.0 - stagetext_alpha(txt);
	float ofs_x, ofs_y;

	if(txt->time.life - t < txt->time.fadeout) {
		ofs_y = 10 * pow(f, 2);
		ofs_x = 0;
	} else {
		ofs_x = ofs_y = 10 * pow(f, 2);
	}

	TextParams params = {};
	params.font_ptr = txt->font;
	params.align = txt->align;
	params.blend = BLEND_PREMUL_ALPHA;
	params.shader_ptr = res_shader("text_stagetext");
	params.shader_params = &(ShaderCustomParams){{ 1 - f }},
	params.aux_textures[0] = res_texture("titletransition");
	params.pos.x = re(txt->pos) + ofs_x;
	params.pos.y = im(txt->pos) + ofs_y;
	params.color = &txt->color;

	text_draw(txt->text, &params);
}

void stagetext_update(void) {
	for(StageText *t = textlist, *next = NULL; t; t = next) {
		next = t->next;
		stagetext_update_single(t);
	}
}

void stagetext_draw(void) {
	for(StageText *t = textlist, *next = NULL; t; t = next) {
		next = t->next;
		stagetext_draw_single(t);
	}
}

static void stagetext_table_push(StageTextTable *tbl, StageText *txt, bool update_pos) {
	list_append(&tbl->elems, list_wrap_container(txt));

	if(update_pos) {
		tbl->pos += text_height(txt->font, txt->text, 0) * I;
	}

	tbl->delay += 5;
}

void stagetext_begin_table(StageTextTable *tbl, const char *title, const Color *titleclr, const Color *clr, double width, int delay, int lifetime, int fadeintime, int fadeouttime) {
	*tbl = (typeof(*tbl)) {
		.pos = VIEWPORT_W/2 + VIEWPORT_H/2*I,
		.clr = *clr,
		.width = width,
		.lifetime = lifetime,
		.fadeintime = fadeintime,
		.fadeouttime = fadeouttime,
		.delay = delay,
	};

	StageText *txt = stagetext_add(title, tbl->pos, ALIGN_CENTER, res_font("big"), titleclr, tbl->delay, lifetime, fadeintime, fadeouttime);
	stagetext_table_push(tbl, txt, true);
}

void stagetext_end_table(StageTextTable *tbl) {
	cmplx ofs = -0.5 * I * (im(tbl->pos) - VIEWPORT_H/2);

	for(ListContainer *c = tbl->elems; c; c = c->next) {
		((StageText*)c->data)->pos += ofs;
	}

	list_free_all(&tbl->elems);
}

static void stagetext_table_add_label(StageTextTable *tbl, const char *title) {
	StageText *txt = stagetext_add(title, tbl->pos - tbl->width * 0.5, ALIGN_LEFT, res_font("standard"), &tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
	stagetext_table_push(tbl, txt, false);
}

void stagetext_table_add(StageTextTable *tbl, const char *title, const char *val) {
	stagetext_table_add_label(tbl, title);
	StageText *txt = stagetext_add(val, tbl->pos + tbl->width * 0.5, ALIGN_RIGHT, res_font("standard"), &tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
	stagetext_table_push(tbl, txt, true);
}

void stagetext_table_add_numeric(StageTextTable *tbl, const char *title, int n) {
	stagetext_table_add_label(tbl, title);
	StageText *txt = stagetext_add_numeric(n, tbl->pos + tbl->width * 0.5, ALIGN_RIGHT, res_font("standard"), &tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
	stagetext_table_push(tbl, txt, true);
}

void stagetext_table_add_numeric_nonzero(StageTextTable *tbl, const char *title, int n) {
	if(n) {
		stagetext_table_add_numeric(tbl, title, n);
	}
}

void stagetext_table_add_separator(StageTextTable *tbl) {
	tbl->pos += I * 0.5 * font_get_lineskip(res_font("standard"));
}


StageText *stagetext_list_head(void) {
	return textlist;
}
