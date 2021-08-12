/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "color.h"
#include "objectpool.h"
#include "resource/font.h"

typedef struct StageText StageText;
typedef LIST_ANCHOR(StageText) StageTextList;
typedef void (*StageTextUpdateFunc)(StageText* txt, int t, float alpha);

typedef struct StageTextTable StageTextTable;

// NOTE: tweaked to consume all padding in StageText, assuming x86_64 ABI
#define STAGETEXT_BUF_SIZE 76

struct StageText {
	LIST_INTERFACE(StageText);

	Font *font;
	cmplx pos;

	struct {
		StageTextUpdateFunc update;
		void *data1;
		void *data2;
	} custom;

	Color color;
	Alignment align;

	struct {
		int spawn;
		int life;
		int fadein;
		int fadeout;
	} time;

	char text[STAGETEXT_BUF_SIZE];
};

void stagetext_free(void);
void stagetext_update(void);
void stagetext_draw(void);
StageText *stagetext_add(const char *text, cmplx pos, Alignment align, Font *font, const Color *clr, int delay, int lifetime, int fadeintime, int fadeouttime);
StageText *stagetext_add_numeric(int n, cmplx pos, Alignment align, Font *font, const Color *clr, int delay, int lifetime, int fadeintime, int fadeouttime);
StageText *stagetext_list_head(void);

struct StageTextTable {
	cmplx pos;
	double width;
	Color clr;
	int lifetime;
	int fadeintime;
	int fadeouttime;
	int delay;
	ListContainer *elems;
};

void stagetext_begin_table(StageTextTable *tbl, const char *title, const Color *titleclr, const Color *clr, double width, int delay, int lifetime, int fadeintime, int fadeouttime);
void stagetext_end_table(StageTextTable *tbl);
void stagetext_table_add(StageTextTable *tbl, const char *title, const char *val);
void stagetext_table_add_numeric(StageTextTable *tbl, const char *title, int n);
void stagetext_table_add_numeric_nonzero(StageTextTable *tbl, const char *title, int n);
void stagetext_table_add_separator(StageTextTable *tbl);
