/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "color.h"

// NOTE: Includes the terminator, so the actual value of usable lines per phase is 1 less than this.
#define CUTSCENE_PHASE_MAX_TEXT_ENTRIES 6

/*
 * Data structure definitions
 */

typedef enum CutsceneTextType {
	CTT_INVALID,
	CTT_NARRATION,
	CTT_DIALOGUE,
	CTT_CENTERED,
} CutsceneTextType;

typedef struct CutscenePhaseTextEntry {
	CutsceneTextType type;
	const char *header;
	const char *text;
	Color color;
} CutscenePhaseTextEntry;

typedef struct CutscenePhase {
	const char *background;
	CutscenePhaseTextEntry text_entries[CUTSCENE_PHASE_MAX_TEXT_ENTRIES];
} CutscenePhase;

typedef struct Cutscene {
	const char *name;
	const char *bgm;
	const CutscenePhase *phases;
} Cutscene;

/*
 * Helper macros for cutscene construction
 */

#define T_NARRATOR(text) \
	{ CTT_NARRATION, NULL, _(text), COLOR_NARRATOR }
#define T_SPEECH(speaker, text, ...) \
	{ CTT_DIALOGUE, speaker, "“" _(text) "”", __VA_ARGS__ }
#define T_CENTERED(header, text) \
	{ CTT_CENTERED, header, _(text), COLOR_NARRATOR }

#define COLOR_NARRATOR { 0.955, 0.934, 0.745, 1 }

// Add new actors here
#define T_REIMU(text)       T_SPEECH("Reimu",       text, { 0.955, 0.550, 0.500, 1 })
#define T_MARISA(text)      T_SPEECH("Marisa",      text, { 1.000, 0.843, 0.216, 1 })
#define T_YOUMU(text)       T_SPEECH("Yōmu",        text, { 0.490, 0.803, 0.725, 1 })
#define T_YUKARI(text)      T_SPEECH("Yukari",      text, { 0.750, 0.600, 0.950, 1 })
#define T_KANAKO(text)      T_SPEECH("Kanako",      text, { 0.600, 0.700, 0.950, 1 })
#define T_SUWAKO(text)      T_SPEECH("Suwako",      text, { 0.515, 0.685, 0.375, 1 })
#define T_SANAE(text)       T_SPEECH("Sanae",       text, { 0.350, 0.650, 0.660, 1 })
#define T_SUMIREKO(text)    T_SPEECH("Sumireko",    text, { 0.700, 0.500, 0.830, 1 })
#define T_YUYUKO(text)      T_SPEECH("Yuyuko",      text, { 0.900, 0.600, 0.900, 1 })
#define T_ELLY(text)        T_SPEECH("Elly",        text, { 0.800, 0.530, 0.525, 1 })
#define T_KURUMI(text)      T_SPEECH("Kurumi",      text, { 0.900, 0.800, 0.470, 1 })
#define T_REMILIA(text)     T_SPEECH("Remilia",     text, { 0.900, 0.100, 0.100, 1 })
#define T_FLANDRE(text)     T_SPEECH("Flandre",     text, { 1.000, 0.600, 0.600, 1 })
#define T_PATCHOULI(text)   T_SPEECH("Patchouli",   text, { 0.700, 0.600, 1.000, 1 })
