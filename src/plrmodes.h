/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include "ending.h"
#include "stage.h"

typedef enum {
    /* do not reorder - screws replays */

    PLR_CHAR_MARISA = 0,
    PLR_CHAR_YOUMU = 1,
    PLR_CHAR_REIMU = 2,

    NUM_CHARACTERS,
} CharacterID;

typedef enum {
    /* do not reorder - screws replays */

    PLR_SHOT_MARISA_LASER = 0,
    PLR_SHOT_MARISA_STAR = 1,

    PLR_SHOT_YOUMU_MIRROR = 0,
    PLR_SHOT_YOUMU_HAUNTING = 1,
    
    PLR_SHOT_REIMU_DREAM= 0,
    PLR_SHOT_REIMU_SPIRIT = 1,

    NUM_SHOT_MODES_PER_CHARACTER,
} ShotModeID;

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
typedef double (*PlayerModeSpeedModProc)(Player *plr, double speed);
typedef void (*PlayerModePreloadProc)(void);

typedef struct PlayerMode {
    const char *name;
    PlayerCharacter *character;
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
        PlayerModeSpeedModProc speed_mod;
        PlayerModePreloadProc preload;
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
