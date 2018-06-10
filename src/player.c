/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "player.h"

#include "projectile.h"
#include "global.h"
#include "plrmodes.h"
#include "stage.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "entity.h"

void player_init(Player *plr) {
	memset(plr, 0, sizeof(Player));
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-64);
	plr->lives = PLR_START_LIVES;
	plr->bombs = PLR_START_BOMBS;
	plr->deathtime = -1;
	plr->continuetime = -1;
	plr->mode = plrmode_find(0, 0);
}

void player_stage_pre_init(Player *plr) {
	plr->recovery = 0;
	plr->respawntime = 0;
	plr->deathtime = -1;
	plr->axis_lr = 0;
	plr->axis_ud = 0;
	plrmode_preload(plr->mode);
}

double player_property(Player *plr, PlrProperty prop) {
	return plr->mode->procs.property(plr, prop);
}

static void ent_draw_player(EntityInterface *ent);
static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg);

static void player_spawn_focus_circle(Player *plr);

void player_stage_post_init(Player *plr) {
	assert(plr->mode != NULL);

	// ensure the essential callbacks are there. other code tests only for the optional ones
	assert(plr->mode->procs.shot != NULL);
	assert(plr->mode->procs.bomb != NULL);
	assert(plr->mode->procs.property != NULL);
	assert(plr->mode->character != NULL);
	assert(plr->mode->dialog != NULL);

	delete_enemies(&global.plr.slaves);

	if(plr->mode->procs.init != NULL) {
		plr->mode->procs.init(plr);
	}

	aniplayer_create(&plr->ani, get_ani(plr->mode->character->player_sprite_name), "main");

	plr->ent.draw_layer = LAYER_PLAYER;
	plr->ent.draw_func = ent_draw_player;
	plr->ent.damage_func = ent_damage_player;
	ent_register(&plr->ent, ENT_PLAYER);

	player_spawn_focus_circle(plr);
}

void player_free(Player *plr) {
	if(plr->mode->procs.free) {
		plr->mode->procs.free(plr);
	}

	aniplayer_free(&plr->ani);
	ent_unregister(&plr->ent);
	delete_enemy(&plr->focus_circle, plr->focus_circle.first);
}

static void player_full_power(Player *plr) {
	play_sound("full_power");
	stage_clear_hazards(CLEAR_HAZARDS_ALL);
	stagetext_add("Full Power!", VIEWPORT_W * 0.5 + VIEWPORT_H * 0.33 * I, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 60, 20, 20);
}

bool player_set_power(Player *plr, short npow) {
	npow = clamp(npow, 0, PLR_MAX_POWER);

	if(plr->mode->procs.power) {
		plr->mode->procs.power(plr, npow);
	}

	int oldpow = plr->power;
	plr->power = npow;

	if(oldpow / 100 < npow / 100) {
		play_sound("powerup");
	}

	if(plr->power == PLR_MAX_POWER && oldpow < PLR_MAX_POWER) {
		player_full_power(plr);
	}

	return oldpow != plr->power;
}

void player_move(Player *plr, complex delta) {
	delta *= player_property(plr, PLR_PROP_SPEED);
	complex lastpos = plr->pos;
	double x = clamp(creal(plr->pos) + creal(delta), PLR_MIN_BORDER_DIST, VIEWPORT_W - PLR_MIN_BORDER_DIST);
	double y = clamp(cimag(plr->pos) + cimag(delta), PLR_MIN_BORDER_DIST, VIEWPORT_H - PLR_MIN_BORDER_DIST);
	plr->pos = x + y*I;
	complex realdir = plr->pos - lastpos;

	if(cabs(realdir)) {
		plr->lastmovedir = realdir / cabs(realdir);
	}

	plr->velocity = realdir;
}

static void ent_draw_player(EntityInterface *ent) {
	Player *plr = ENT_CAST(ent, Player);

	if(plr->deathtime > global.frames) {
		return;
	}

	if(plr->focus) {
		r_draw_sprite(&(SpriteParams) {
			.sprite = "fairy_circle",
			.rotation.angle = DEG2RAD * global.frames * 10,
			.color = RGBA_MUL_ALPHA(1, 1, 1, 0.2 * (clamp(plr->focus, 0, 15) / 15.0)),
			.pos = { creal(plr->pos), cimag(plr->pos) },
		});
	}

	Color c;

	if(!player_is_vulnerable(plr) && (global.frames/8)&1) {
		c = *RGBA_MUL_ALPHA(0.4, 0.4, 1.0, 0.9);
	} else {
		c = *RGBA_MUL_ALPHA(1.0, 1.0, 1.0, 1.0);
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.pos = { creal(plr->pos), cimag(plr->pos) },
		.color = &c,
	});
}

static int player_focus_circle_logic(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_NONE;
	}

	double alpha = creal(e->args[0]);

	if(t <= 1) {
		alpha = min(0.1, alpha);
	} else {
		alpha = approach(alpha, (global.plr.inputflags & INFLAG_FOCUS) ? 1 : 0, 1/30.0);
	}

	e->args[0] = alpha;
	return ACTION_NONE;
}

static void player_focus_circle_visual(Enemy *e, int t, bool render) {
	if(!render || !creal(e->args[0])) {
		return;
	}

	int trans_frames = 12;
	double trans_factor = 1 - min(trans_frames, t) / (double)trans_frames;
	double rot_speed = DEG2RAD * global.frames * (1 + 3 * trans_factor);
	double scale = 1.0 + trans_factor;

	r_draw_sprite(&(SpriteParams) {
		.sprite = "focus",
		.rotation.angle = rot_speed,
		.color = RGBA_MUL_ALPHA(1, 1, 1, creal(e->args[0])),
		.pos = { creal(e->pos), cimag(e->pos) },
		.scale.both = scale,
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite = "focus",
		.rotation.angle = rot_speed * -1,
		.color = RGBA_MUL_ALPHA(1, 1, 1, creal(e->args[0])),
		.pos = { creal(e->pos), cimag(e->pos) },
		.scale.both = scale,
	});
}

static void player_spawn_focus_circle(Player *plr) {
	Enemy *f = create_enemy_p(&plr->focus_circle, 0, ENEMY_IMMUNE, player_focus_circle_visual, player_focus_circle_logic, 0, 0, 0, 0);
	f->ent.draw_layer = LAYER_PLAYER_FOCUS;
}

static void player_fail_spell(Player *plr) {
	if( !global.boss ||
		!global.boss->current ||
		global.boss->current->finished ||
		global.boss->current->failtime ||
		global.boss->current->starttime >= global.frames ||
		global.stage->type == STAGE_SPELL
	) {
		return;
	}

	global.boss->current->failtime = global.frames;

	if(global.boss->current->type == AT_ExtraSpell) {
		boss_finish_current_attack(global.boss);
	}
}

bool player_should_shoot(Player *plr, bool extra) {
	return
		(plr->inputflags & INFLAG_SHOT) &&
		!global.dialog &&
		player_is_alive(&global.plr) &&
		// TODO: maybe get rid of this?
		(!extra || !player_is_bomb_active(plr));
}

void player_placeholder_bomb_logic(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	DamageInfo dmg;
	dmg.amount = 100;
	dmg.type = DMG_PLAYER_BOMB;

	for(Enemy *en = global.enemies.first; en; en = en->next) {
		ent_damage(&en->ent, &dmg);
	}

	if(global.boss) {
		ent_damage(&global.boss->ent, &dmg);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);
}

void player_logic(Player* plr) {
	if(plr->continuetime == global.frames) {
		plr->lives = PLR_START_LIVES;
		plr->bombs = PLR_START_BOMBS;
		plr->life_fragments = 0;
		plr->bomb_fragments = 0;
		plr->continues_used += 1;
		player_set_power(plr, 0);
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
		spawn_items(plr->deathpos, Power, (int)ceil(PLR_MAX_POWER/(double)POWER_VALUE), NULL);
	}

	process_enemies(&plr->slaves);
	aniplayer_update(&plr->ani);

	if(plr->deathtime < -1) {
		plr->deathtime++;
		plr->pos -= I;
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		return;
	}

	plr->focus = approach(plr->focus, (plr->inputflags & INFLAG_FOCUS) ? 30 : 0, 1);
	plr->focus_circle.first->pos = plr->pos;
	process_enemies(&plr->focus_circle);

	if(plr->mode->procs.think) {
		plr->mode->procs.think(plr);
	}

	if(player_should_shoot(plr, false)) {
		plr->mode->procs.shot(plr);
	}

	if(global.frames == plr->deathtime) {
		player_realdeath(plr);
	} else if(plr->deathtime > global.frames) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
	}

	if(player_is_bomb_active(plr)) {
		if(plr->bombcanceltime) {
			int bctime = plr->bombcanceltime + plr->bombcanceldelay;

			if(bctime <= global.frames) {
				plr->recovery = global.frames;
				plr->bombcanceltime = 0;
				plr->bombcanceldelay = 0;
				return;
			}
		}

		player_fail_spell(plr);
	}
}

bool player_bomb(Player *plr) {
	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell)
		return false;

	int bomb_time = floor(player_property(plr, PLR_PROP_BOMB_TIME));

	if(bomb_time <= 0) {
		return false;
	}

	if(!player_is_bomb_active(plr) && (plr->bombs > 0 || plr->iddqd) && global.frames - plr->respawntime >= 60) {
		player_fail_spell(plr);
		stage_clear_hazards(CLEAR_HAZARDS_ALL);
		plr->mode->procs.bomb(plr);
		plr->bombs--;

		if(plr->deathtime > 0) {
			plr->deathtime = -1;

			if(plr->bombs) {
				plr->bombs--;
			}
		}

		if(plr->bombs < 0) {
			plr->bombs = 0;
		}

		plr->bombtotaltime = bomb_time;
		plr->recovery = global.frames + plr->bombtotaltime;
		plr->bombcanceltime = 0;
		plr->bombcanceldelay = 0;

		return true;
	}

	return false;
}

bool player_is_bomb_active(Player *plr) {
	return global.frames - plr->recovery < 0;
}

bool player_is_vulnerable(Player *plr) {
	return global.frames - abs(plr->recovery) >= 0 && !plr->iddqd && player_is_alive(plr);
}

bool player_is_alive(Player *plr) {
	return plr->deathtime >= -1 && plr->deathtime < global.frames;
}

void player_cancel_bomb(Player *plr, int delay) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	if(plr->bombcanceltime) {
		int canceltime_queued = plr->bombcanceltime + plr->bombcanceldelay;
		int canceltime_requested = global.frames + delay;

		if(canceltime_queued > canceltime_requested) {
			plr->bombcanceldelay -= (canceltime_queued - canceltime_requested);
		}
	} else {
		plr->bombcanceltime = global.frames;
		plr->bombcanceldelay = delay;
	}
}

double player_get_bomb_progress(Player *plr, double *out_speed) {
	if(!player_is_bomb_active(plr)) {
		if(out_speed != NULL) {
			*out_speed = 1.0;
		}

		return 1;
	}

	int start_time = plr->recovery - plr->bombtotaltime;
	int end_time = plr->recovery;

	if(!plr->bombcanceltime || plr->bombcanceltime + plr->bombcanceldelay >= end_time) {
		if(out_speed != NULL) {
			*out_speed = 1.0;
		}

		return (plr->bombtotaltime - (end_time - global.frames))/(double)plr->bombtotaltime;
	}

	int cancel_time = plr->bombcanceltime + plr->bombcanceldelay;
	int passed_time = plr->bombcanceltime - start_time;

	int shortened_total_time = (plr->bombtotaltime - passed_time) - (end_time - cancel_time);
	int shortened_passed_time = (global.frames - plr->bombcanceltime);

	double passed_fraction = passed_time / (double)plr->bombtotaltime;
	double shortened_fraction = shortened_passed_time / (double)shortened_total_time;
	shortened_fraction *= (1 - passed_fraction);

	if(out_speed != NULL) {
		*out_speed = (plr->bombtotaltime - passed_time) / (double)shortened_total_time;
	}

	return passed_fraction + shortened_fraction;
}

void player_realdeath(Player *plr) {
	plr->deathtime = -DEATH_DELAY-1;
	plr->respawntime = global.frames;
	plr->inputflags &= ~INFLAGS_MOVE;
	plr->deathpos = plr->pos;
	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30.0*I;
	plr->recovery = -(global.frames + DEATH_DELAY + 150);
	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	player_fail_spell(plr);

	if(global.stage->type != STAGE_SPELL && global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		// deaths in extra spells "don't count"
		return;
	}

	int drop = max(2, (plr->power * 0.15) / POWER_VALUE);
	spawn_items(plr->deathpos, Power, drop, NULL);

	player_set_power(plr, plr->power * 0.7);
	plr->bombs = PLR_START_BOMBS;
	plr->bomb_fragments = 0;

	if(plr->lives-- == 0 && global.replaymode != REPLAY_PLAY) {
		stage_gameover();
	}
}

static void player_death_effect_draw_overlay(Projectile *p, int t) {
	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);
	r_framebuffer(framebuffers->front);
	r_texture(1, "static");
	r_uniform_int("noise", 1);
	r_uniform_int("frames", global.frames);
	r_uniform_float("progress", t / p->timeout);
	r_uniform_vec2("origin", creal(p->pos), VIEWPORT_H - cimag(p->pos));
	r_uniform_vec2("clear_origin", creal(global.plr.pos), VIEWPORT_H - cimag(global.plr.pos));
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	draw_framebuffer_tex(framebuffers->back, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(framebuffers);

	// This change must propagate, hence the r_state salsa. Yes, pop then push, I know what I'm doing.
	r_state_pop();
	r_framebuffer(framebuffers->back);
	r_state_push();
}

static void player_death_effect_draw_sprite(Projectile *p, int t) {
	float s = t / p->timeout;

	float stretch_range = 3, sx, sy;

	sx = 0.5 + 0.5 * cos(M_PI * (2 * pow(s, 0.5) + 1));
	sx = (1 - s) * (1 + (stretch_range - 1) * sx) + s * stretch_range * sx;
	sy = 1 + pow(s, 3);

	if(sx <= 0 || sy <= 0) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.pos = { creal(p->pos), cimag(p->pos) },
		.scale = { .x = sx, .y = sy },
	});
}

static int player_death_effect(Projectile *p, int t) {
	if(t < 0) {
		if(t == EVENT_DEATH) {
			for(int i = 0; i < 12; ++i) {
				PARTICLE(
					.sprite = "blast",
					.pos = p->pos + 2 * frand() * cexp(I*M_PI*2*frand()),
					.color = RGBA(0.15, 0.2, 0.5, 0),
					.timeout = 12 + i + 2 * nfrand(),
					.draw_rule = GrowFade,
					.args = { 0, 20 + i },
					.angle = M_PI*2*frand(),
					.flags = PFLAG_NOREFLECT,
					.layer = LAYER_OVERLAY,
				);
			}
		}

		return ACTION_ACK;
	}

	return ACTION_NONE;
}

void player_death(Player *plr) {
	if(!player_is_vulnerable(plr)) {
		return;
	}

	play_sound("death");

	for(int i = 0; i < 60; i++) {
		tsrand_fill(2);
		PARTICLE(
			.sprite = "flare",
			.pos = plr->pos,
			.rule = linear,
			.timeout = 40,
			.draw_rule = Shrink,
			.args = { (3+afrand(0)*7)*cexp(I*tsrand_a(1)) },
			.flags = PFLAG_NOREFLECT,
		);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	PARTICLE(
		.sprite = "blast",
		.pos = plr->pos,
		.color = RGBA(0.5, 0.15, 0.15, 0),
		.timeout = 35,
		.draw_rule = GrowFade,
		.args = { 0, 2.4 },
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
	);

	PARTICLE(
		.pos = plr->pos,
		.size = 1+I,
		.timeout = 90,
		.draw_rule = player_death_effect_draw_overlay,
		.blend = BLEND_NONE,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_OVERLAY,
		.shader = "player_death",
	);

	PARTICLE(
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.pos = plr->pos,
		.timeout = 30,
		.rule = player_death_effect,
		.draw_rule = player_death_effect_draw_sprite,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PLAYER_FOCUS, // LAYER_OVERLAY | 1,
	);

	plr->deathtime = global.frames + floor(player_property(plr, PLR_PROP_DEATHBOMB_WINDOW));
}

static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg) {
	Player *plr = ENT_CAST(ent, Player);

	if(
		!player_is_vulnerable(plr) ||
		(dmg->type != DMG_ENEMY_SHOT && dmg->type != DMG_ENEMY_COLLISION)
	) {
		return DMG_RESULT_IMMUNE;
	}

	player_death(plr);
	return DMG_RESULT_OK;
}

static PlrInputFlag key_to_inflag(KeyIndex key) {
	switch(key) {
		case KEY_UP:    return INFLAG_UP;
		case KEY_DOWN:  return INFLAG_DOWN;
		case KEY_LEFT:  return INFLAG_LEFT;
		case KEY_RIGHT: return INFLAG_RIGHT;
		case KEY_FOCUS: return INFLAG_FOCUS;
		case KEY_SHOT:  return INFLAG_SHOT;
		case KEY_SKIP:  return INFLAG_SKIP;
		default:        return 0;
	}
}

bool player_updateinputflags(Player *plr, PlrInputFlag flags) {
	if(flags == plr->inputflags) {
		return false;
	}

	if((flags & INFLAG_FOCUS) && !(plr->inputflags & INFLAG_FOCUS)) {
		plr->focus_circle.first->birthtime = global.frames;
	}

	plr->inputflags = flags;
	return true;
}

bool player_updateinputflags_moveonly(Player *plr, PlrInputFlag flags) {
	return player_updateinputflags(plr, (flags & INFLAGS_MOVE) | (plr->inputflags & ~INFLAGS_MOVE));
}

bool player_setinputflag(Player *plr, KeyIndex key, bool mode) {
	PlrInputFlag newflags = plr->inputflags;
	PlrInputFlag keyflag = key_to_inflag(key);

	if(!keyflag) {
		return false;
	}

	if(mode) {
		newflags |= keyflag;
	} else {
		newflags &= ~keyflag;
	}

	return player_updateinputflags(plr, newflags);
}

static bool player_set_axis(int *aptr, uint16_t value) {
	int16_t new = (int16_t)value;

	if(*aptr == new) {
		return false;
	}

	*aptr = new;
	return true;
}

void player_event(Player *plr, uint8_t type, uint16_t value, bool *out_useful, bool *out_cheat) {
	bool useful = true;
	bool cheat = false;
	bool is_replay = global.replaymode == REPLAY_PLAY;

	switch(type) {
		case EV_PRESS:
			if(global.dialog && (value == KEY_SHOT || value == KEY_BOMB)) {
				useful = page_dialog(&global.dialog);
				break;
			}

			switch(value) {
				case KEY_BOMB:
					useful = player_bomb(plr);

					if(!useful && plr->iddqd) {
						// smooth bomb cancellation test
						player_cancel_bomb(plr, 60);
						useful = true;
					}

					break;

				case KEY_IDDQD:
					plr->iddqd = !plr->iddqd;
					cheat = true;
					break;

				case KEY_POWERUP:
					useful = player_set_power(plr, plr->power + 100);
					cheat = true;
					break;

				case KEY_POWERDOWN:
					useful = player_set_power(plr, plr->power - 100);
					cheat = true;
					break;

				default:
					useful = player_setinputflag(plr, value, true);
					break;
			}
			break;

		case EV_RELEASE:
			useful = player_setinputflag(plr, value, false);
			break;

		case EV_AXIS_LR:
			useful = player_set_axis(&plr->axis_lr, value);
			break;

		case EV_AXIS_UD:
			useful = player_set_axis(&plr->axis_ud, value);
			break;

		case EV_INFLAGS:
			useful = player_updateinputflags(plr, value);
			break;

		case EV_CONTINUE:
			// continuing in the same frame will desync the replay,
			// so schedule it for the next one
			plr->continuetime = global.frames + 1;
			useful = true;
			break;

		default:
			log_warn("Can not handle event: [%i:%02x:%04x]", global.frames, type, value);
			useful = false;
			break;
	}

	if(is_replay) {
		if(!useful) {
			log_warn("Useless event in replay: [%i:%02x:%04x]", global.frames, type, value);
		}

		if(cheat) {
			log_warn("Cheat event in replay: [%i:%02x:%04x]", global.frames, type, value);

			if( !(global.replay.flags           & REPLAY_GFLAG_CHEATS) ||
				!(global.replay_stage->flags    & REPLAY_SFLAG_CHEATS)) {
				log_warn("...but this replay was NOT properly cheat-flagged! Not cool, not cool at all");
			}
		}

		if(type == EV_CONTINUE && (
			!(global.replay.flags           & REPLAY_GFLAG_CONTINUES) ||
			!(global.replay_stage->flags    & REPLAY_SFLAG_CONTINUES))) {
			log_warn("Continue event in replay: [%i:%02x:%04x], but this replay was not properly continue-flagged", global.frames, type, value);
		}
	}

	if(out_useful) {
		*out_useful = useful;
	}

	if(out_cheat) {
		*out_cheat = cheat;
	}
}

bool player_event_with_replay(Player *plr, uint8_t type, uint16_t value) {
	bool useful, cheat;
	assert(global.replaymode == REPLAY_RECORD);

	if(config_get_int(CONFIG_SHOT_INVERTED) && value == KEY_SHOT && (type == EV_PRESS || type == EV_RELEASE)) {
		type = type == EV_PRESS ? EV_RELEASE : EV_PRESS;
	}

	player_event(plr, type, value, &useful, &cheat);

	if(useful) {
		replay_stage_event(global.replay_stage, global.frames, type, value);

		if(type == EV_CONTINUE) {
			global.replay.flags |= REPLAY_GFLAG_CONTINUES;
			global.replay_stage->flags |= REPLAY_SFLAG_CONTINUES;
		}

		if(cheat) {
			global.replay.flags |= REPLAY_GFLAG_CHEATS;
			global.replay_stage->flags |= REPLAY_SFLAG_CHEATS;
		}

		return true;
	} else {
		log_debug("Useless event discarded: [%i:%02x:%04x]", global.frames, type, value);
	}

	return false;
}

// free-axis movement
bool player_applymovement_gamepad(Player *plr) {
	if(!plr->axis_lr && !plr->axis_ud) {
		if(plr->gamepadmove) {
			plr->gamepadmove = false;
			plr->inputflags &= ~INFLAGS_MOVE;
		}
		return false;
	}

	complex direction = (
		gamepad_normalize_axis_value(plr->axis_lr) +
		gamepad_normalize_axis_value(plr->axis_ud) * I
	);

	if(cabs(direction) > 1) {
		direction /= cabs(direction);
	}

	int sr = sign(creal(direction));
	int si = sign(cimag(direction));

	player_updateinputflags_moveonly(plr,
		(INFLAG_UP    * (si == -1)) |
		(INFLAG_DOWN  * (si ==  1)) |
		(INFLAG_LEFT  * (sr == -1)) |
		(INFLAG_RIGHT * (sr ==  1))
	);

	if(direction) {
		plr->gamepadmove = true;
		player_move(&global.plr, direction);
	}

	return true;
}

static const char *moveseqname(int dir) {
	switch(dir) {
	case -1: return "left";
	case 0: return "main";
	case 1: return "right";
	default: log_fatal("Invalid player animation dir given");
	}
}

static void player_ani_moving(Player *plr, int dir) {
	if(plr->lastmovesequence == dir)
		return;
	const char *seqname = moveseqname(dir);
	const char *lastseqname = moveseqname(plr->lastmovesequence);

	char *transition = strjoin(lastseqname,"2",seqname,NULL);

	aniplayer_hard_switch(&plr->ani,transition,1);
	aniplayer_queue(&plr->ani,seqname,0);
	plr->lastmovesequence = dir;
	free(transition);
}

void player_applymovement(Player *plr) {
	plr->velocity = 0;

	if(plr->deathtime != -1)
		return;

	bool gamepad = player_applymovement_gamepad(plr);

	int up      =   plr->inputflags & INFLAG_UP;
	int down    =   plr->inputflags & INFLAG_DOWN;
	int left    =   plr->inputflags & INFLAG_LEFT;
	int right   =   plr->inputflags & INFLAG_RIGHT;

	if(left && !right) {
		player_ani_moving(plr,-1);
	} else if(right && !left) {
		player_ani_moving(plr,1);
	} else {
		player_ani_moving(plr,0);
	}

	if(gamepad) {
		return;
	}

	complex direction = 0;

	if(up)      direction -= 1.0*I;
	if(down)    direction += 1.0*I;
	if(left)    direction -= 1.0;
	if(right)   direction += 1.0;

	if(cabs(direction))
		direction /= cabs(direction);

	if(direction)
		player_move(&global.plr, direction);
}

void player_fix_input(Player *plr) {
	// correct input state to account for any events we might have missed,
	// usually because the pause menu ate them up

	PlrInputFlag newflags = plr->inputflags;
	bool invert_shot = config_get_int(CONFIG_SHOT_INVERTED);

	for(KeyIndex key = KEYIDX_FIRST; key <= KEYIDX_LAST; ++key) {
		int flag = key_to_inflag(key);

		if(flag && !(plr->gamepadmove && (flag & INFLAGS_MOVE))) {
			bool flagset = plr->inputflags & flag;
			bool keyheld = gamekeypressed(key);

			if(invert_shot && key == KEY_SHOT) {
				keyheld = !keyheld;
			}

			if(flagset && !keyheld) {
				newflags &= ~flag;
			} else if(!flagset && keyheld) {
				newflags |= flag;
			}
		}
	}

	if(newflags != plr->inputflags) {
		player_event_with_replay(plr, EV_INFLAGS, newflags);
	}

	int axis_lr = gamepad_player_axis_value(PLRAXIS_LR);
	int axis_ud = gamepad_player_axis_value(PLRAXIS_UD);

	if(plr->axis_lr != axis_lr) {
		player_event_with_replay(plr, EV_AXIS_LR, axis_lr);
	}

	if(plr->axis_ud != axis_ud) {
		player_event_with_replay(plr, EV_AXIS_UD, axis_ud);
	}
}

void player_graze(Player *plr, complex pos, int pts, int effect_intensity) {
	if(!(++plr->graze)) {
		log_warn("Graze counter overflow");
		plr->graze = 0xffff;
	}

	player_add_points(plr, pts);
	play_sound("graze");

	for(int i = 0; i < effect_intensity; ++i) {
		tsrand_fill(3);

		PARTICLE(
			.sprite = "flare",
			.pos = pos,
			.rule = linear,
			.timeout = 5 + 5 * afrand(2),
			.draw_rule = Shrink,
			.args = { (1+afrand(0)*5)*cexp(I*M_PI*2*afrand(1)) },
			.flags = PFLAG_NOREFLECT,
			.layer = LAYER_PARTICLE_LOW,
		);
	}
}

static void player_add_fragments(Player *plr, int frags, int *pwhole, int *pfrags, int maxfrags, int maxwhole, const char *fragsnd, const char *upsnd) {
	if(*pwhole >= maxwhole) {
		return;
	}

	*pfrags += frags;
	int up = *pfrags / maxfrags;

	*pwhole += up;
	*pfrags %= maxfrags;

	if(up) {
		play_sound(upsnd);
	}

	if(frags) {
		// FIXME: when we have the extra life/bomb sounds,
		//        don't play this if upsnd was just played.
		play_sound(fragsnd);
	}

	if(*pwhole >= maxwhole) {
		*pwhole = maxwhole;
		*pfrags = 0;
	}
}

void player_add_life_fragments(Player *plr, int frags) {
	player_add_fragments(plr, frags, &plr->lives, &plr->life_fragments, PLR_MAX_LIFE_FRAGMENTS, PLR_MAX_LIVES,
		"item_generic", // FIXME: replacement needed
		"extra_life"
	);
}

void player_add_bomb_fragments(Player *plr, int frags) {
	player_add_fragments(plr, frags, &plr->bombs, &plr->bomb_fragments, PLR_MAX_BOMB_FRAGMENTS, PLR_MAX_BOMBS,
		"item_generic",  // FIXME: replacement needed
		"extra_bomb"
	);
}

void player_add_lives(Player *plr, int lives) {
	player_add_life_fragments(plr, PLR_MAX_LIFE_FRAGMENTS);
}

void player_add_bombs(Player *plr, int bombs) {
	player_add_bomb_fragments(plr, PLR_MAX_BOMB_FRAGMENTS);
}


static void try_spawn_bonus_item(Player *plr, ItemType type, uint oldpoints, uint reqpoints) {
	int items = plr->points / reqpoints - oldpoints / reqpoints;

	if(items > 0) {
		complex p = creal(plr->pos);
		create_item(p, -5*I, type);
		spawn_items(p, type, --items, NULL);
	}
}

void player_add_points(Player *plr, uint points) {
	uint old = plr->points;
	plr->points += points;

	if(global.stage->type != STAGE_SPELL) {
		try_spawn_bonus_item(plr, LifeFrag, old, PLR_SCORE_PER_LIFE_FRAG);
		try_spawn_bonus_item(plr, BombFrag, old, PLR_SCORE_PER_BOMB_FRAG);
	}
}

void player_register_damage(Player *plr, EntityInterface *target, const DamageInfo *damage) {
	if(damage->type != DMG_PLAYER_SHOT && damage->type != DMG_PLAYER_BOMB) {
		return;
	}

	if(target != NULL) {
		switch(target->type) {
			case ENT_ENEMY: {
				player_add_points(&global.plr, damage->amount * 0.5);
				break;
			}

			case ENT_BOSS: {
				player_add_points(&global.plr, damage->amount * 0.2);
				break;
			}

			default: break;
		}
	}


#ifdef PLR_DPS_STATS
	while(global.frames > plr->dmglogframe) {
		memmove(plr->dmglog + 1, plr->dmglog, sizeof(plr->dmglog) - sizeof(*plr->dmglog));
		plr->dmglog[0] = 0;
		plr->dmglogframe++;
	}

	plr->dmglog[0] += damage->amount;
#endif
}

void player_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SHADER_PROGRAM, flags,
		"player_death",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"static",
	NULL);

	preload_resources(RES_SPRITE, flags,
		"focus",
		"fairy_circle",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"graze",
		"death",
		"generic_shot",
		"powerup",
		"full_power",
		"extra_life",
		"extra_bomb",
	NULL);
}

// FIXME: where should this be?

complex plrutil_homing_target(complex org, complex fallback) {
	double mindst = INFINITY;
	complex target = fallback;

	if(global.boss && boss_is_vulnerable(global.boss)) {
		target = global.boss->pos;
		mindst = cabs(target - org);
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->hp == ENEMY_IMMUNE) {
			continue;
		}

		double dst = cabs(e->pos - org);

		if(dst < mindst) {
			mindst = dst;
			target = e->pos;
		}
	}

	return target;
}
