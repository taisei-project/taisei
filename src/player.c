/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "player.h"

#include "projectile.h"
#include "global.h"
#include "plrmodes.h"
#include "stage.h"

void init_player(Player *plr) {
	memset(plr, 0, sizeof(Player));
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-64);
	plr->lives = PLR_START_LIVES;
	plr->bombs = PLR_START_BOMBS;
	plr->deathtime = -1;
}

void prepare_player_for_next_stage(Player *plr) {
	plr->recovery = 0;
	plr->respawntime = 0;
	plr->deathtime = -1;
	plr->graze = 0;
	plr->movetime = 0;
	plr->prevmove = 0;
	plr->prevmovetime = 0;
	plr->axis_lr = 0;
	plr->axis_ud = 0;
}

Animation *player_get_ani(Character cha) {
	Animation *ani = NULL;
	switch(cha) {
	case Youmu:
		ani = get_ani("youmu");
		break;
	case Marisa:
		ani = get_ani("marisa");
		break;
	}

	return ani;
}

static void player_full_power(Player *plr) {
	play_sound("full_power");
	stage_clear_hazards(false);
}

void player_set_power(Player *plr, short npow) {
	npow = clamp(npow, 0, PLR_MAX_POWER);

	switch(plr->cha) {
		case Youmu:
			youmu_power(plr, npow);
			break;
		case Marisa:
			marisa_power(plr, npow);
			break;
	}

	int oldpow = plr->power;
	plr->power = npow;

	if(plr->power == PLR_MAX_POWER && oldpow < PLR_MAX_POWER) {
		player_full_power(plr);
	}
}

void player_move(Player *plr, complex delta) {
	float speed = 0.01*VIEWPORT_W;

	if(plr->inputflags & INFLAG_FOCUS)
		speed /= 2.0;

	if(plr->cha == Marisa && plr->shot == MarisaLaser && global.frames - plr->recovery < 0)
		speed /= 5.0;

	complex opos = plr->pos - VIEWPORT_W/2.0 - VIEWPORT_H/2.0*I;
	complex npos = opos + delta*speed;

	Animation *ani = player_get_ani(plr->cha);

	short xfac = fabs(creal(npos)) < fabs(creal(opos)) || fabs(creal(npos)) < VIEWPORT_W/2.0 - ani->w/2;
	short yfac = fabs(cimag(npos)) < fabs(cimag(opos)) || fabs(cimag(npos)) < VIEWPORT_H/2.0 - ani->h/2;

	plr->pos += (creal(delta)*xfac + cimag(delta)*yfac*I)*speed;
}

void player_draw(Player* plr) {
	if(plr->deathtime > global.frames)
		return;

	draw_enemies(plr->slaves);

	glPushMatrix();
		glTranslatef(creal(plr->pos), cimag(plr->pos), 0);

		if(plr->focus) {
			glPushMatrix();
				glRotatef(global.frames*10, 0, 0, 1);
				glScalef(1, 1, 1);
				glColor4f(1, 1, 1, 0.2 * (clamp(plr->focus, 0, 15) / 15.0));
				draw_texture(0, 0, "fairy_circle");
				glColor4f(1,1,1,1);
			glPopMatrix();
		}

		glDisable(GL_CULL_FACE);
		if(plr->dir) {
			glPushMatrix();
			glScalef(-1,1,1);
		}

		int clr_changed = 0;

		if(global.frames - abs(plr->recovery) < 0 && (global.frames/8)&1) {
			glColor4f(0.4,0.4,1,0.9);
			clr_changed = 1;
		}

		draw_animation_p(0, 0, !plr->moving, player_get_ani(plr->cha));

		if(clr_changed)
			glColor3f(1,1,1);

		if(plr->dir)
			glPopMatrix();

		glEnable(GL_CULL_FACE);

		if(plr->focus) {
			glPushMatrix();
				glColor4f(1, 1, 1, plr->focus / 30.0);
				glRotatef(global.frames, 0, 0, -1);
				draw_texture(0, 0, "focus");
				glColor4f(1, 1, 1, 1);
			glPopMatrix();
		}

	glPopMatrix();
}

static void player_fail_spell(Player *plr) {
	if(	!global.boss ||
		!global.boss->current ||
		global.boss->current->finished ||
		global.boss->current->failtime ||
		global.stage->type == STAGE_SPELL
	) {
		return;
	}

	global.boss->current->failtime = global.frames;

	if(global.boss->current->type == AT_ExtraSpell) {
		boss_finish_current_attack(global.boss);
	}
}

void player_logic(Player* plr) {
	process_enemies(&plr->slaves);
	if(plr->deathtime < -1) {
		plr->deathtime++;
		plr->pos -= I;
		return;
	}

	plr->focus = approach(plr->focus, (plr->inputflags & INFLAG_FOCUS) ? 30 : 0, 1);

	switch(plr->cha) {
		case Youmu:
			youmu_shot(plr);
			break;
		case Marisa:
			marisa_shot(plr);
			break;
	}

	if(global.frames == plr->deathtime)
		player_realdeath(plr);

	if(global.frames - plr->recovery < 0) {
		Enemy *en;
		for(en = global.enemies; en; en = en->next)
			if(!en->unbombable && en->hp > ENEMY_IMMUNE)
				en->hp -= 300;

		if(global.boss && boss_is_vulnerable(global.boss)) {
			global.boss->current->hp -= 30;
		}

		stage_clear_hazards(false);
		player_fail_spell(plr);
	}
}

void player_bomb(Player *plr) {
	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell)
		return;

	if(global.frames - plr->recovery >= 0 && (plr->bombs > 0 || plr->iddqd) && global.frames - plr->respawntime >= 60) {
		player_fail_spell(plr);
		delete_projectiles(&global.projs);

		switch(plr->cha) {
		case Marisa:
			marisa_bomb(plr);
			break;
		case Youmu:
			youmu_bomb(plr);
			break;
		}

		plr->bombs--;

		if(plr->deathtime > 0) {
			plr->deathtime = -1;

			if(plr->bombs)
				plr->bombs--;
		}

		if(plr->bombs < 0) {
			plr->bombs = 0;
		}

		plr->recovery = global.frames + BOMB_RECOVERY;
	}
}

void player_realdeath(Player *plr) {
	plr->deathtime = -DEATH_DELAY-1;
	plr->respawntime = global.frames;
	plr->inputflags &= ~INFLAGS_MOVE;

	const double arc = 0.3 * M_PI;
	int drop = max(2, (plr->power * 0.15) / POWER_VALUE);

	for(int i = 0; i < drop; ++i) {
		double ofs = arc * (i/((double)drop - 1));
		create_item(plr->pos, (12+3*frand()) * (cexp(I*(1.5*M_PI - 0.5*arc + ofs)) - 1.0*I), Power);
	}

	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30.0*I;
	plr->recovery = -(global.frames + DEATH_DELAY + 150);
	plr->bombs = PLR_START_BOMBS;
	plr->bomb_fragments = 0;

	if(plr->iddqd)
		return;

	player_fail_spell(plr);
	player_set_power(plr, plr->power * 0.7);

	if(plr->lives-- == 0 && global.replaymode != REPLAY_PLAY)
		stage_gameover();
}

void player_death(Player *plr) {
	if(plr->deathtime == -1 && global.frames - abs(plr->recovery) > 0) {
		play_sound("death");
		int i;
		for(i = 0; i < 20; i++) {
			tsrand_fill(2);
			create_particle2c("flare", plr->pos, 0, Shrink, timeout_linear, 40, (3+afrand(0)*7)*cexp(I*tsrand_a(1)))->type=PlrProj;
		}
		create_particle2c("blast", plr->pos, rgba(1,0.3,0.3,0.5), GrowFadeAdd, timeout, 35, 2.4)->type=PlrProj;
		plr->deathtime = global.frames + DEATHBOMB_TIME;
	}
}

static PlrInputFlag key_to_inflag(KeyIndex key) {
	switch(key) {
		case KEY_UP:    return INFLAG_UP;    break;
		case KEY_DOWN:  return INFLAG_DOWN;  break;
		case KEY_LEFT:  return INFLAG_LEFT;  break;
		case KEY_RIGHT: return INFLAG_RIGHT; break;
		case KEY_FOCUS: return INFLAG_FOCUS; break;
		case KEY_SHOT:  return INFLAG_SHOT;  break;
		default:        return 0;
	}
}

void player_setinputflag(Player *plr, KeyIndex key, bool mode) {
	PlrInputFlag flag = key_to_inflag(key);

	if(!flag) {
		return;
	}

	if(mode) {
		if(flag & INFLAGS_MOVE) {
			plr->prevmove = plr->curmove;
			plr->prevmovetime = plr->movetime;
			plr->curmove = flag;
			plr->movetime = global.frames;
		}

		plr->inputflags |= flag;
	} else {
		plr->inputflags &= ~flag;
	}
}

void player_event(Player* plr, int type, int key) {
	switch(type) {
		case EV_PRESS:
			switch(key) {
				case KEY_BOMB:
					player_bomb(plr);
					break;

				case KEY_IDDQD:
					plr->iddqd = !plr->iddqd;
					break;

				case KEY_POWERUP:
					player_set_power(plr, plr->power + 100);
					break;

				case KEY_POWERDOWN:
					player_set_power(plr, plr->power - 100);
					break;

				default:
					player_setinputflag(plr, key, true);
					break;
			}
			break;

		case EV_RELEASE:
			player_setinputflag(plr, key, false);
			break;

		case EV_AXIS_LR:
			plr->axis_lr = key;
			break;

		case EV_AXIS_UD:
			plr->axis_ud = key;
			break;

		default:
			log_warn("Unknown event type %d (value=%d)", type, key);
			break;
	}
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

	complex direction = (plr->axis_lr + plr->axis_ud*I) / (double)GAMEPAD_AXIS_MAX;
	if(cabs(direction) > 1)
		direction /= cabs(direction);

	int sr = sign(creal(direction));
	int si = sign(cimag(direction));

	player_setinputflag(plr, KEY_UP,    si == -1);
	player_setinputflag(plr, KEY_DOWN,  si ==  1);
	player_setinputflag(plr, KEY_LEFT,  sr == -1);
	player_setinputflag(plr, KEY_RIGHT, sr ==  1);

	if(direction) {
		plr->gamepadmove = true;
		player_move(&global.plr, direction);
	}

	return true;
}

void player_applymovement(Player *plr) {
	if(plr->deathtime < -1)
		return;

	bool gamepad = player_applymovement_gamepad(plr);
	plr->moving = false;

	int up		=	plr->inputflags & INFLAG_UP,
		down	=	plr->inputflags & INFLAG_DOWN,
		left	=	plr->inputflags & INFLAG_LEFT,
		right	=	plr->inputflags & INFLAG_RIGHT;

	if(left && !right) {
		plr->moving = true;
		plr->dir = 1;
	} else if(right && !left) {
		plr->moving = true;
		plr->dir = 0;
	}

	if(gamepad)
		return;

	complex direction = 0;

	if(up)		direction -= 1.0*I;
	if(down)	direction += 1.0*I;
	if(left)	direction -= 1;
	if(right)	direction += 1;

	if(cabs(direction))
		direction /= cabs(direction);

	if(direction)
		player_move(&global.plr, direction);
}

void player_input_workaround(Player *plr) {
	if(global.dialog)
		return;

	for(KeyIndex key = KEYIDX_FIRST; key <= KEYIDX_LAST; ++key) {
		int flag = key_to_inflag(key);

		if(flag) {
			int event = -1;
			bool flagset = plr->inputflags & flag;
			bool keyheld = gamekeypressed(key);

			if(flagset && !keyheld) {
				// something ate the release event (most likely the ingame menu)
				event = EV_RELEASE;
			} else if(!flagset && keyheld) {
				// something ate the press event (most likely the ingame menu)
				event = EV_PRESS;
			}

			if(event != -1) {
				player_event(plr, event, key);
				replay_stage_event(global.replay_stage, global.frames, event, key);
			}
		}
	}
}

void player_graze(Player *plr, complex pos, int pts) {
	plr->graze++;

	player_add_points(&global.plr, pts);
	play_sound("graze");

	int i = 0; for(i = 0; i < 5; ++i) {
		tsrand_fill(3);
		create_particle2c("flare", pos, 0, Shrink, timeout_linear, 5 + 5 * afrand(2), (1+afrand(0)*5)*cexp(I*tsrand_a(1)))->type=PlrProj;
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

	if(*pwhole > maxwhole) {
		*pwhole = maxwhole;
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


static void try_spawn_bonus_item(Player *plr, ItemType type, unsigned int oldpoints, unsigned int reqpoints) {
	int items = plr->points / reqpoints - oldpoints / reqpoints;

	if(items > 0) {
		complex p = creal(plr->pos);
		create_item(p, -5*I, type);
		spawn_items(p, type, --items, NULL);
	}
}

void player_add_points(Player *plr, unsigned int points) {
	unsigned int old = plr->points;
	plr->points += points;

	if(global.stage->type != STAGE_SPELL) {
		try_spawn_bonus_item(plr, LifeFrag, old, PLR_SCORE_PER_LIFE_FRAG);
		try_spawn_bonus_item(plr, BombFrag, old, PLR_SCORE_PER_BOMB_FRAG);
	}
}

void player_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_TEXTURE, flags,
		"focus",
		"fairy_circle",
		"masterspark",
		"masterspark_ring",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"graze",
		"death",
		"generic_shot",
		"masterspark",
		"full_power",
		"extra_life",
		"extra_bomb",
	NULL);

	preload_resources(RES_ANIM, flags,
		"youmu",
		"marisa",
	NULL);
}
