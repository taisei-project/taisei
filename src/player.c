/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "player.h"

#include <SDL/SDL.h>
#include "projectile.h"
#include "global.h"
#include "plrmodes.h"
#include "stage.h"

void init_player(Player* plr) {
	memset(plr, 0, sizeof(Player));
	
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-20);
	
	plr->lifes = PLR_START_LIVES;
	plr->bombs = PLR_START_BOMBS;
	
	plr->deathtime = -1;
	plr->continues = 0;
}

Animation *player_get_ani(Character cha) {
	Animation *ani;
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

void player_set_power(Player *plr, float npow) {
	switch(plr->cha) {
		case Youmu:
			youmu_power(plr, npow);
			break;
		case Marisa:
			marisa_power(plr, npow);
			break;
	}
	
	plr->power = npow;
	
	if(plr->power > PLR_MAXPOWER)
		plr->power = PLR_MAXPOWER;
	
	if(plr->power < 0)
		plr->power = 0;
}

void player_move(Player *plr, complex delta) {
	float speed = 0.01*VIEWPORT_W/((plr->focus > 0)+1);
	
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
	draw_enemies(plr->slaves);
	
	glPushMatrix();
		glTranslatef(creal(plr->pos), cimag(plr->pos), 0);
		
		if(plr->focus != 0) {
			glPushMatrix();
				glRotatef(global.frames*10, 0, 0, 1);
				glScalef(1, 1, 1);
				glColor4f(1,1,1,0.2);
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
		
		if(plr->focus != 0) {
			glPushMatrix();
				glColor4f(1,1,1,fabs((float)plr->focus/30.0f));
				glRotatef(global.frames, 0, 0, -1);
				draw_texture(0, 0, "focus");
				glColor4f(1,1,1,1);
			glPopMatrix();
		}
		
	glPopMatrix();
}

void player_logic(Player* plr) {
	process_enemies(&plr->slaves);
	if(plr->deathtime < -1) {
		plr->deathtime++;
		plr->pos -= 0.7I;
		return;
	}

	if(plr->focus < 0 || (plr->focus > 0 && plr->focus < 30))
		plr->focus++;


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

		Projectile *p;
		for(p = global.projs; p; p = p->next)
			if(p->type >= FairyProj)
				p->type = DeadProj;

		if(global.boss && global.boss->current) {
			AttackType at = global.boss->current->type;
			if(at != AT_Move && at != AT_SurvivalSpell)
				global.boss->dmg += 30;
		}
	}
}

void player_bomb(Player *plr) {
	if(global.frames - plr->recovery >= 0 && plr->bombs > 0 && global.frames - plr->respawntime >= 60) {
		
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
			plr->bombs /= 2;
		}			
		
		plr->recovery = global.frames + BOMB_RECOVERY;
	}
}

void player_realdeath(Player *plr) {
	plr->deathtime = -DEATH_DELAY-1;
	plr->respawntime = global.frames;
	plr->moveflags = 0;
		
	create_item(plr->pos, 6-15*I, Power);
	create_item(plr->pos, -6-15*I, Power);
	
	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30I;
	plr->recovery = -(global.frames + DEATH_DELAY + 150);

	if(plr->bombs < PLR_START_BOMBS)
		plr->bombs = PLR_START_BOMBS;
	
	if(plr->lifes-- == 0 && global.replaymode != REPLAY_PLAY)
		stage_gameover();
}

void player_death(Player *plr) {
	if(plr->deathtime == -1 && global.frames - abs(plr->recovery) > 0) {
		int i;
		for(i = 0; i < 20; i++) {
			tsrand_fill(2);
			create_particle2c("flare", plr->pos, NULL, Shrink, timeout_linear, 40, (3+afrand(0)*7)*cexp(I*tsrand_a(1)));
		}
		create_particle2c("blast", plr->pos, rgb(1,0.5,0.3), GrowFade, timeout, 35, 2.4);
		plr->deathtime = global.frames + DEATHBOMB_TIME;
	}
}

void player_setmoveflag(Player* plr, int key, int mode) {
	int flag = 0;
	
	switch(key) {
		case KEY_UP:	flag = MOVEFLAG_UP; 	break;
		case KEY_DOWN:	flag = MOVEFLAG_DOWN;	break;
		case KEY_LEFT:	flag = MOVEFLAG_LEFT;	break;
		case KEY_RIGHT:	flag = MOVEFLAG_RIGHT;	break;
	}
	
	if(!flag)
		return;
	
	if(mode) {
		plr->prevmove = plr->curmove;
		plr->prevmovetime = plr->movetime;
		plr->curmove = flag;
		plr->moveflags |= flag;
		plr->movetime = global.frames;
	} else {
		plr->moveflags &= ~flag;
	}
}

void player_event(Player* plr, int type, int key) {
	switch(type) {
		case EV_PRESS:
			switch(key) {
				case KEY_FOCUS:
					plr->focus = 1;
					break;
				
				case KEY_SHOT:
					plr->fire = True;
					break;
				
				case KEY_BOMB:
					player_bomb(plr);
					break;
				
				default:
					player_setmoveflag(plr, key, True);
					break;
			}
			break;
		
		case EV_RELEASE:
			switch(key) {
				case KEY_FOCUS:
					plr->focus = -30;  // that's for the transparency timer
					break;
				
				case KEY_SHOT:
					plr->fire = False;
					break;
				
				default:
					player_setmoveflag(plr, key, False);
					break;
			}
			
			break;
		
		case EV_AXIS_LR:
			plr->axis_lr = key;
			break;
			
		case EV_AXIS_UD:
			plr->axis_ud = key;
			break;
	}
}

// free-axis movement
int player_applymovement_gamepad(Player *plr) {
	if(!plr->axis_lr && !plr->axis_ud)
		return False;
	
	complex direction = (plr->axis_lr + plr->axis_ud*I) / (double)GAMEPAD_AXIS_RANGE;
	if(cabs(direction) > 1)
		direction /= cabs(direction);
	
	double real = creal(direction);
	double imag = cimag(direction);
	int sr = SIGN(real);
	int si = SIGN(imag);
	
	player_setmoveflag(plr, KEY_UP,		si == -1);
	player_setmoveflag(plr, KEY_DOWN,	si ==  1);
	player_setmoveflag(plr, KEY_LEFT,	sr == -1);
	player_setmoveflag(plr, KEY_RIGHT,	sr ==  1);
	
	if(direction)
		player_move(&global.plr, direction);
	
	return True;
}

void player_applymovement(Player *plr) {
	if(plr->deathtime < -1)
		return;
	
	int gamepad = player_applymovement_gamepad(plr);
	plr->moving = False;
	
	int up		=	plr->moveflags & MOVEFLAG_UP,
		down	=	plr->moveflags & MOVEFLAG_DOWN,
		left	=	plr->moveflags & MOVEFLAG_LEFT,
		right	=	plr->moveflags & MOVEFLAG_RIGHT;
	
	if(left && !right) {
		plr->moving = True;
		plr->dir = 1;
	} else if(right && !left) {
		plr->moving = True;
		plr->dir = 0;
	}
	
	if(gamepad)
		return;
	
	complex direction = 0;
	
	if(up)		direction -= 1I;
	if(down)	direction += 1I;
	if(left)	direction -= 1;
	if(right)	direction += 1;
	
	if(cabs(direction))
		direction /= cabs(direction);
	
	if(direction)
		player_move(&global.plr, direction);
}

void player_input_workaround(Player *plr) {
	if(!global.dialog) {
		int shot  = gamekeypressed(KEY_SHOT);
		int focus = gamekeypressed(KEY_FOCUS);
		
		if(!shot && plr->fire) {
			player_event(plr, EV_RELEASE, KEY_SHOT);
			replay_event(&global.replay, EV_RELEASE, KEY_SHOT);
		} else if(shot && !plr->fire) {
			player_event(plr, EV_PRESS, KEY_SHOT);
			replay_event(&global.replay, EV_PRESS, KEY_SHOT);
		}
		
		if(!focus && plr->focus > 0) {
			player_event(plr, EV_RELEASE, KEY_FOCUS);
			replay_event(&global.replay, EV_RELEASE, KEY_FOCUS);
		}
	}
}

void player_graze(Player *plr, complex pos, int pts) {
	global.points += pts;
	plr->graze++;
	play_sound("graze");
	
	int i = 0; for(i = 0; i < 5; ++i) {
		tsrand_fill(3);
		create_particle2c("flare", pos, NULL, Shrink, timeout_linear, 5 + 5 * afrand(2), (1+afrand(0)*5)*cexp(I*tsrand_a(1)));
	}
}
