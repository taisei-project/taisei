/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "util.h"
#include "color.h"
#include "resource/font.h"

typedef struct StageText StageText;
typedef void (*StageTextPreDrawFunc)(StageText* txt, int t, float alpha);

typedef struct StageTextTable StageTextTable;

struct StageText {
    StageText *next;
    StageText *prev;

    char *text;
    Font **font;

    complex pos;
    Alignment align;
    float clr[4];

    struct {
        int spawn;
        int life;
        int fadein;
        int fadeout;
    } time;

    struct {
        StageTextPreDrawFunc predraw;
        void *data1;
        void *data2;
    } custom;
};

void stagetext_free(void);
void stagetext_draw(void);
StageText* stagetext_add(const char *text, complex pos, Alignment align, Font **font, Color clr, int delay, int lifetime, int fadeintime, int fadeouttime);
StageText* stagetext_add_numeric(int n, complex pos, Alignment align, Font **font, Color clr, int delay, int lifetime, int fadeintime, int fadeouttime);

struct StageTextTable {
    complex pos;
    double width;
    Color clr;
    int lifetime;
    int fadeintime;
    int fadeouttime;
    int delay;
    ListContainer *elems;
};

void stagetext_begin_table(StageTextTable *tbl, const char *title, Color titleclr, Color clr, double width, int delay, int lifetime, int fadeintime, int fadeouttime);
void stagetext_end_table(StageTextTable *tbl);
void stagetext_table_add(StageTextTable *tbl, const char *title, const char *val);
void stagetext_table_add_numeric(StageTextTable *tbl, const char *title, int n);
void stagetext_table_add_numeric_nonzero(StageTextTable *tbl, const char *title, int n);
void stagetext_table_add_separator(StageTextTable *tbl);

void stagetext_table_test(void);
