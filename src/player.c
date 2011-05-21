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
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-20);
	
	plr->focus = False;
	plr->fire = False;
	plr->moving = False;
	plr->dir = 0;
	plr->power = 0;
	
	plr->lifes = 2;
	plr->bombs = 3;
	
	plr->slaves = NULL;
	
	plr->recovery = 0;
	
	plr->cha = cha;
	plr->shot = shot;
	
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
		
		if(global.frames - abs(plr->recovery) < 0 && (global.frames/17)&1)
			glColor4f(0.8,0.8,1,0.9);
			
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
	
	if(plr->fire && plr->cha == Youmu) {
		if(!(global.frames % 4)) {
			create_projectile("youmu", plr->pos + 10 - I*20, NULL, linear, rarg(-20I))->type = PlrProj;
			create_projectile("youmu", plr->pos - 10 - I*20, NULL, linear, rarg(-20I))->type = PlrProj;
						
			if(plr->power >= 2) {
				float a = 0.20;
				if(plr->focus > 0) a = 0.06;
				create_projectile("youmu", plr->pos - 10 - I*20, NULL, linear, rarg(I*-20*cexp(-I*a)))->type = PlrProj;
				create_projectile("youmu", plr->pos + 10 - I*20, NULL, linear, rarg(I*-20*cexp(I*a)))->type = PlrProj;
			}		
		}
		
		float a = 1;
		if(plr->focus > 0)
			a = 0.4;
		if(plr->shot == YoumuHoming && !(global.frames % 7)) {
			complex ref = -1;
			if(global.boss != NULL)
				ref = add_ref(global.boss);
			else if(global.enemies != NULL)
				ref = add_ref(global.enemies);
			
			if(ref != -1)
				create_projectile("hghost", plr->pos, NULL, youmu_homing, rarg(a*cexp(I*rand()), ref))->type = PlrProj;
		}
	}
		
	if(plr->focus < 0 || (plr->focus > 0 && plr->focus < 30))
		plr->focus++;
	
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy(&plr->slaves, youmu_opposite_draw, youmu_opposite_logic, plr->pos, ENEMY_IMMUNE, plr, 0);
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
		plr->recovery = global.frames + 200;
	}
}

void plr_death(Player *plr) {
	if(plr->lifes-- == 0) {
		game_over();
	} else {
		create_item(plr->pos, 6-15*I, Power);
		create_item(plr->pos, -6-15*I, Power);
		
		plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I;
		plr->recovery = -(global.frames + 200);		
		
		if(global.plr.bombs < 2)
			global.plr.bombs = 2;
	}
	
	if(plr->slaves)
		delete_enemies(&plr->slaves);
}