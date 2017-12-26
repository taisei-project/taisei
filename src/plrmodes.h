/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include "ending.h"
#include "stage.h"
#include "dialog.h"

typedef enum {
	/* do not reorder - screws replays */

	PLR_CHAR_MARISA = 0,
	PLR_CHAR_YOUMU = 1,
	PLR_CHAR_REIMU = 2,

	NUM_CHARACTERS,
} CharacterID;

typedef enum {
	/* do not reorder - screws replays */

	PLR_SHOT_A,
	PLR_SHOT_B,
	NUM_SHOT_MODES_PER_CHARACTER,

	PLR_SHOT_MARISA_LASER   = PLR_SHOT_A,
	PLR_SHOT_MARISA_STAR    = PLR_SHOT_B,

	PLR_SHOT_YOUMU_MIRROR   = PLR_SHOT_A,
	PLR_SHOT_YOUMU_HAUNTING = PLR_SHOT_B,

	PLR_SHOT_REIMU_DREAM    = PLR_SHOT_A,
	PLR_SHOT_REIMU_SPIRIT   = PLR_SHOT_B,
} ShotModeID;

typedef enum {
	// vpu = viewport units

	PLR_PROP_SPEED,              // current player movement speed (vpu/frame)
	PLR_PROP_POC,                // "point of collection" boundary: all items are auto-collected when player is above this (vpu)
	PLR_PROP_COLLECT_RADIUS,     // how near the player has to be to an item before it's auto-collected (vpu)
	PLR_PROP_BOMB_TIME,          // how long a bomb should last if it were to activate this frame; 0 prevents activation (frames)
	PLR_PROP_DEATHBOMB_WINDOW,   // how much time the player has to recover with a bomb if they died in this frame (frames)
} PlrProperty;

typedef void (*PlrCharEndingProc)(Ending *e);

typedef struct PlayerCharacter {
	char id;
	const char *lower_name;
	const char *proper_name;
	const char *full_name;
	const char *title;
	const char *dialog_sprite_name;
	const char *player_sprite_name;

	struct {
		PlrCharEndingProc good;
		PlrCharEndingProc bad;
	} ending;
} PlayerCharacter;

typedef void (*PlayerModeInitProc)(Player *plr);
typedef void (*PlayerModeFreeProc)(Player *plr);
typedef void (*PlayerModeThinkProc)(Player *plr);
typedef void (*PlayerModeShotProc)(Player *plr);
typedef void (*PlayerModeBombProc)(Player *plr);
typedef void (*PlayerModeBombBgProc)(Player *plr);
typedef void (*PlayerModePowerProc)(Player *plr, short npow);
typedef void (*PlayerModePreloadProc)(void);
typedef double (*PlayerModePropertyProc)(Player *plr, PlrProperty prop);

typedef struct PlayerMode {
	const char *name;
	PlayerCharacter *character;
	PlayerDialogProcs *dialog;
	ShotModeID shot_mode;

	struct {
		PlayerModeInitProc init;
		PlayerModeFreeProc free;
		PlayerModeThinkProc think;
		PlayerModeShotProc shot;
		PlayerModeBombProc bomb;
		ShaderRule bomb_shader;
		PlayerModeBombBgProc bombbg;
		PlayerModePowerProc power;
		PlayerModePreloadProc preload;
		PlayerModePropertyProc property;
	} procs;
} PlayerMode;

enum {
	NUM_PLAYER_MODES = NUM_CHARACTERS * NUM_SHOT_MODES_PER_CHARACTER,
};

PlayerCharacter* plrchar_get(CharacterID id);
void plrchar_preload(PlayerCharacter *pc);

PlayerMode* plrmode_find(CharacterID charid, ShotModeID shotid);
int plrmode_repr(char *out, size_t outsize, PlayerMode *mode);
PlayerMode* plrmode_parse(const char *name);
void plrmode_preload(PlayerMode *mode);

double player_property(Player *plr, PlrProperty prop);
