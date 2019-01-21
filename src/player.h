/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_player_h
#define IGUARD_player_h

#include "taisei.h"

#ifdef DEBUG
	#define PLR_DPS_STATS
#endif

#include "util.h"
#include "enemy.h"
#include "gamepad.h"
#include "aniplayer.h"
#include "resource/animation.h"
#include "entity.h"

enum {
	PLR_MAX_POWER = 400,
	PLR_MAX_POWER_OVERFLOW = 200,
	PLR_MAX_LIVES = 8,
	PLR_MAX_BOMBS = 8,

	// -Wpedantic doesn't like this
	// PLR_MAX_PIV = UINT32_MAX,
	#define PLR_MAX_PIV UINT32_MAX
	#define PLR_MAX_GRAZE UINT32_MAX

	PLR_MAX_LIFE_FRAGMENTS = 5,
	PLR_MAX_BOMB_FRAGMENTS = 5,

	PLR_START_LIVES = 2,
	PLR_START_BOMBS = 3,
	PLR_START_PIV = 10000,

	PLR_SCORE_PER_LIFE_FRAG = 55000,
	PLR_SCORE_PER_BOMB_FRAG = 22000,

	PLR_STGPRACTICE_LIVES = PLR_MAX_LIVES,
	PLR_STGPRACTICE_BOMBS = PLR_START_BOMBS,

	PLR_MIN_BORDER_DIST = 16,

	PLR_POWERSURGE_DURATION = 300,
	PLR_POWERSURGE_POWERCOST = 100,
};

static const float PLR_POWERSURGE_POSITIVE_DRAIN_MAX = (0.15 / 60.0);
static const float PLR_POWERSURGE_POSITIVE_DRAIN_MIN = (0.15 / 60.0);
static const float PLR_POWERSURGE_NEGATIVE_DRAIN_MAX = (0.10 / 60.0);
static const float PLR_POWERSURGE_NEGATIVE_DRAIN_MIN = (0.01 / 60.0);
static const float PLR_POWERSURGE_POSITIVE_GAIN      = (1.50 / 60.0);
static const float PLR_POWERSURGE_NEGATIVE_GAIN      = (0.20 / 60.0);

typedef enum {
	// do not reorder these or you'll break replays

	INFLAG_UP = 1,
	INFLAG_DOWN = 2,
	INFLAG_LEFT = 4,
	INFLAG_RIGHT = 8,
	INFLAG_FOCUS = 16,
	INFLAG_SHOT = 32,
	INFLAG_SKIP = 64,
} PlrInputFlag;

enum {
	INFLAGS_MOVE = INFLAG_UP | INFLAG_DOWN | INFLAG_LEFT | INFLAG_RIGHT
};

typedef struct Player Player;

struct Player {
	ENTITY_INTERFACE_NAMED(Player, ent);

	complex pos;
	complex velocity;
	complex deathpos;
	complex lastmovedir;

	struct PlayerMode *mode;
	AniPlayer ani;
	EnemyList slaves;
	EnemyList focus_circle;

	struct {
		float positive;
		float negative;
		struct {
			int activated, expired;
		} time;
		int bonus;
		double damage_done;
	} powersurge;

	uint64_t points;

	uint point_item_value;
	int lives;
	int bombs;
	int life_fragments;
	int bomb_fragments;
	int continues_used;
	uint graze;
	int focus;
	int power;
	int power_overflow;

	int continuetime;
	int recovery;
	int deathtime;
	int respawntime;
	int bombcanceltime;
	int bombcanceldelay;
	int bombtotaltime;

	int inputflags;
	int lastmovesequence; // used for animation
	int axis_ud;
	int axis_lr;

	bool gamepadmove;
	bool iddqd;

#ifdef PLR_DPS_STATS
	int dmglogframe;
	int dmglog[240];
#endif
};

// this is used by both player and replay code
enum {
	EV_PRESS,
	EV_RELEASE,
	EV_OVER, // replay-only
	EV_AXIS_LR,
	EV_AXIS_UD,
	EV_CHECK_DESYNC, // replay-only
	EV_FPS, // replay-only
	EV_INFLAGS,
	EV_CONTINUE,
};

// This is called first before we even enter stage_loop.
// It's also called right before syncing player state from a replay stage struct, if a replay is being watched or recorded, before every stage.
// The entire state is reset here, and defaults for story mode are set.
void player_init(Player *plr);

// This is called early in stage_loop, before creating or reading replay stage data.
// State that is not supposed to be preserved between stages is reset here, and any plrmode-specific resources are preloaded.
void player_stage_pre_init(Player *plr);

// This is called right after the stage's begin proc. After that, the actual game loop starts.
void player_stage_post_init(Player *plr);

// Yes, that's 3 different initialization functions right here.

void player_free(Player *plr);

void player_logic(Player*);
bool player_should_shoot(Player *plr, bool extra);

bool player_set_power(Player *plr, short npow);
bool player_add_power(Player *plr, short pdelta);

void player_move(Player*, complex delta);

void player_realdeath(Player*);
void player_death(Player*);
void player_graze(Player *plr, complex pos, int pts, int effect_intensity, const Color *color);

void player_event(Player *plr, uint8_t type, uint16_t value, bool *out_useful, bool *out_cheat);
bool player_event_with_replay(Player *plr, uint8_t type, uint16_t value);
void player_applymovement(Player* plr);
void player_fix_input(Player *plr);

void player_add_life_fragments(Player *plr, int frags);
void player_add_bomb_fragments(Player *plr, int frags);
void player_add_lives(Player *plr, int lives);
void player_add_bombs(Player *plr, int bombs);
void player_add_points(Player *plr, uint points);
void player_add_piv(Player *plr, uint piv);
void player_extend_powersurge(Player *plr, float pos, float neg);

void player_register_damage(Player *plr, EntityInterface *target, const DamageInfo *damage);

void player_cancel_bomb(Player *plr, int delay);
void player_cancel_powersurge(Player *plr);
void player_placeholder_bomb_logic(Player *plr);

bool player_is_bomb_active(Player *plr);
bool player_is_powersurge_active(Player *plr);
bool player_is_vulnerable(Player *plr);
bool player_is_alive(Player *plr);

uint player_powersurge_calc_bonus(Player *plr);
uint player_powersurge_calc_bonus_rate(Player *plr);

// Progress is normalized from 0: bomb start to 1: bomb end
double player_get_bomb_progress(Player *plr, double *out_speed);

void player_damage_hook(Player *plr, EntityInterface *target, DamageInfo *dmg);

void player_preload(void);

// FIXME: where should this be?
complex plrutil_homing_target(complex org, complex fallback);

#endif // IGUARD_player_h
