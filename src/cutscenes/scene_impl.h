/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_cutscenes_scene_impl_h
#define IGUARD_cutscenes_scene_impl_h

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
	{ CTT_NARRATION, NULL, text, COLOR_NARRATOR }
#define T_SPEECH(speaker, text, ...) \
	{ CTT_DIALOGUE, speaker, "“" text "”", __VA_ARGS__ }
#define T_CENTERED(header, text) \
	{ CTT_CENTERED, header, text, COLOR_NARRATOR }

#define COLOR_NARRATOR { 0.955, 0.934, 0.745, 1 }

// Add new actors here
#define T_REIMU(text)       T_SPEECH("Reimu",       text, { 0.955, 0.550, 0.500, 1 })
#define T_YUKARI(text)      T_SPEECH("Yukari",      text, { 0.750, 0.600, 0.950, 1 })

#endif // IGUARD_cutscenes_scene_impl_h
