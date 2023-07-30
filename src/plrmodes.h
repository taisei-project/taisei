/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include "cutscenes/cutscene.h"
#include "dialog.h"
#include "endings.h"

typedef enum {
	// WARNING: Reordering this will break current replays, and possibly even progress files.

	PLR_CHAR_REIMU = 0,
	PLR_CHAR_MARISA = 1,
	PLR_CHAR_YOUMU = 2,

	NUM_CHARACTERS,
} CharacterID;

typedef enum {
	// WARNING: Reordering this will break current replays, and possibly even progress files.

	PLR_SHOT_A,
	PLR_SHOT_B,
	NUM_SHOT_MODES_PER_CHARACTER,

	PLR_SHOT_MARISA_LASER   = PLR_SHOT_A,
	PLR_SHOT_MARISA_STAR    = PLR_SHOT_B,

	PLR_SHOT_YOUMU_MIRROR   = PLR_SHOT_A,
	PLR_SHOT_YOUMU_HAUNTING = PLR_SHOT_B,

	PLR_SHOT_REIMU_SPIRIT   = PLR_SHOT_A,
	PLR_SHOT_REIMU_DREAM    = PLR_SHOT_B,
} ShotModeID;

typedef enum {
	// vpu = viewport units

	PLR_PROP_SPEED,              // current player movement speed (vpu/frame)
	PLR_PROP_POC,                // "point of collection" boundary: all items are auto-collected when player is above this (vpu)
	PLR_PROP_COLLECT_RADIUS,     // how near the player has to be to an item before it's auto-collected (vpu)
	PLR_PROP_BOMB_TIME,          // how long a bomb should last if it were to activate this frame; 0 prevents activation (frames)
	PLR_PROP_DEATHBOMB_WINDOW,   // how much time the player has to recover with a bomb if they died in this frame (frames)
} PlrProperty;

typedef struct PlrCharEndingCutscene {
	CutsceneID cutscene_id;
	EndingID ending_id;
} PlrCharEndingCutscene;

typedef struct PlayerCharacter {
	CharacterID id;
	const char *lower_name;
	const char *proper_name;
	const char *full_name;
	const char *title;
	const char *menu_texture_name;

	struct {
		PlrCharEndingCutscene good;
		PlrCharEndingCutscene bad;
	} ending;
} PlayerCharacter;

typedef void (*PlayerModeInitProc)(Player *plr);
typedef void (*PlayerModeFreeProc)(Player *plr);
typedef void (*PlayerModePreloadProc)(ResourceGroup *rg);
typedef double (*PlayerModePropertyProc)(Player *plr, PlrProperty prop);

typedef struct PlayerMode {
	const char *name;
	const char *description;
	const char *spellcard_name;
	PlayerCharacter *character;
	PlayerDialogTasks *dialog;
	ShotModeID shot_mode;

	struct {
		PlayerModeInitProc init;
		PlayerModePreloadProc preload;
		PlayerModePropertyProc property;
	} procs;
} PlayerMode;

enum {
	NUM_PLAYER_MODES = NUM_CHARACTERS * NUM_SHOT_MODES_PER_CHARACTER,
};

PlayerCharacter *plrchar_get(CharacterID id);
void plrchar_preload(PlayerCharacter *pc, ResourceGroup *rg);
void plrchar_render_bomb_portrait(PlayerCharacter *pc, Sprite *out_spr);
int plrchar_player_anim_name(PlayerCharacter *pc, size_t bufsize, char buf[bufsize]);
Animation *plrchar_player_anim(PlayerCharacter *pc);

PlayerMode *plrmode_find(CharacterID charid, ShotModeID shotid);
int plrmode_repr(char *out, size_t outsize, PlayerMode *mode, bool internal);
PlayerMode *plrmode_parse(const char *name);
void plrmode_preload(PlayerMode *mode, ResourceGroup *rg);

double player_property(Player *plr, PlrProperty prop);
