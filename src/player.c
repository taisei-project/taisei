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

void init_player(Player* plr, Character cha, ShotMode shot) {
	memset(plr, 0, sizeof(Player));
	
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-20);
		
	plr->lifes = 2;
	plr->bombs = 3;
	
	plr->cha = cha;
	plr->shot = shot;
	
	plr->deathtime = -1;
	
	switch(cha) {
	case Youmu:
		plr->ani = get_ani("youmu");
		break;
	case Marisa:
		plr->ani = get_ani("marisa");
		break;
	}
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
		
		if(global.frames - abs(plr->recovery) < 0 && (global.frames/8)&1)
			glColor4f(0.4,0.4,1,0.9);
			
		draw_animation_p(0, 0, !plr->moving, plr->ani);
		
		glColor4f(1,1,1,1);
		
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
		plr_realdeath(plr);
}

void plr_set_power(Player *plr, float npow) {
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

void plr_bomb(Player *plr) {
	if(global.frames - plr->recovery >= 0 && plr->bombs > 0) {
		Enemy *e;
		for(e = global.enemies; e; e = e->next)
			if(e->hp != ENEMY_IMMUNE)
				e->hp = 0;
		
		delete_projectiles(&global.projs);
		
		play_sound("laser1");
		
		plr->bombs--;
		
		if(plr->deathtime > 0) {
			plr->deathtime = -1;
			plr->bombs /= 2;
		}			
		
		plr->recovery = global.frames + 200;
		
	}
}

void plr_realdeath(Player *plr) {
	plr->deathtime = -1;
	
	if(plr->lifes-- == 0) {
		game_over();
	} else {
		create_item(plr->pos, 6-15*I, Power);
		create_item(plr->pos, -6-15*I, Power);
		
		plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I;
		plr->recovery = -(global.frames + 150);
		
		if(global.plr.bombs < 3)
			global.plr.bombs = 3;
	}
}

void plr_death(Player *plr) {
	if(plr->deathtime == -1 && global.frames - abs(plr->recovery) > 0) {
		int i;
		for(i = 0; i < 20; i++)
			create_particle2c("flare", plr->pos, NULL, Shrink, timeout_linear, 40, (3+frand()*7)*cexp(I*rand()));
		create_particle2c("blast", plr->pos, rgb(1,0.5,0.3), GrowFade, timeout, 35, 2.4);
		plr->deathtime = global.frames + DEATHBOMB_TIME;
	}
}