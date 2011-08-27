/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage.h"

#include <SDL/SDL.h>
#include "global.h"
#include "menu/ingamemenu.h"

void stage_start() {
	global.timer = 0;
	global.frames = 0;
	global.game_over = 0;
	global.points = 0;
}

void stage_input() {
	if(global.menu) {
		menu_input(global.menu);
		return;
	}		
	
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		global_processevent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(sym == tconfig.intval[KEY_FOCUS])
				global.plr.focus = 1;
			else if(sym == tconfig.intval[KEY_SHOT]) {
				if(!global.dialog)
					global.plr.fire = True;
				else
					page_dialog(&global.dialog);
			} else if(sym == tconfig.intval[KEY_BOMB]) {
				if(!global.dialog)
					plr_bomb(&global.plr);
			} else if(sym == SDLK_ESCAPE) {
				if(!global.menu)
					global.menu = create_ingame_menu();
				else
					global.menu->quit = 1;
			}
		} else if(event.type == SDL_KEYUP) {
			if(sym == tconfig.intval[KEY_FOCUS])
				global.plr.focus = -30; // that's for the transparency timer
			else if(sym == tconfig.intval[KEY_SHOT])
				global.plr.fire = False;
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
	
	if(global.plr.deathtime < -1)
		return;
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	global.plr.moving = False;
	
	if(keys[tconfig.intval[KEY_LEFT]] && !keys[tconfig.intval[KEY_RIGHT]]) {
		global.plr.moving = True;
		global.plr.dir = 1;
	} else if(keys[tconfig.intval[KEY_RIGHT]] && !keys[tconfig.intval[KEY_LEFT]]) {
		global.plr.moving = True;
		global.plr.dir = 0;
	}	
	
	complex direction = 0;
	
	if(keys[tconfig.intval[KEY_LEFT]])
		direction -= 1;		
	if(keys[tconfig.intval[KEY_RIGHT]])
		direction += 1;
	if(keys[tconfig.intval[KEY_UP]])
		direction -= 1I;
	if(keys[tconfig.intval[KEY_DOWN]])
		direction += 1I;
	
	if(direction)
		plr_move(&global.plr, direction);
	
	if(!keys[tconfig.intval[KEY_SHOT]] && global.plr.fire)
		global.plr.fire = False;
	if(!keys[tconfig.intval[KEY_FOCUS]] && global.plr.focus > 0)
		global.plr.focus = -30;
}

void draw_hud() {
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");	
	
	char buf[16];
	int i;
	
	glPushMatrix();
	glTranslatef(615,0,0);
	
	for(i = 0; i < global.plr.lifes; i++)
	  draw_texture(16*i,167, "star");

	for(i = 0; i < global.plr.bombs; i++)
	  draw_texture(16*i,200, "star");
	glColor3f(1,1,1);
	
	sprintf(buf, "%.2f", global.plr.power);
	draw_text(AL_Center, 10, 236, buf, _fonts.standard);
	
	sprintf(buf, "%i", global.points);
	draw_text(AL_Center, 13, 49, buf, _fonts.standard);
	
	glPopMatrix();
	
	sprintf(buf, "%i fps", global.fps.show_fps);
	draw_text(AL_Right, SCREEN_W, SCREEN_H-20, buf, _fonts.standard);	
	
	if(global.boss)
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");
}

void stage_draw() {
	set_ortho();

	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
		
	if(!global.menu) {		
		if(!tconfig.intval[NO_SHADER])
			apply_bg_shaders();

		if(global.boss) {
			glPushMatrix();
			glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);
			
			glColor3f(0.2,0.35,0.6);
			draw_texture(0,0,"boss_shadow");
			
			glColor3f(1,1,1);			
			glRotatef(global.frames*4.0, 0, 0, -1);
			float f = 0.8+0.1*sin(global.frames/8.0);
			glScalef(f,f,f);
			draw_texture(0,0,"boss_circle");
			
			
			glPopMatrix();
		}
		
		player_draw(&global.plr);

		draw_items();
		draw_projectiles(global.projs);
		draw_enemies(global.enemies);
		draw_lasers();

		if(global.boss)
			draw_boss(global.boss);

		draw_projectiles(global.particles);

		if(global.dialog)
			draw_dialog(global.dialog);
				
		if(!tconfig.intval[NO_SHADER]) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			
			glPushMatrix();
			if(global.plr.cha == Marisa && global.plr.shot == MarisaLaser && global.frames - global.plr.recovery < 0)
				glTranslatef(8*sin(global.frames),8*sin(global.frames+3),0);
			draw_fbo_viewport(&resources.fsec);
			glPopMatrix();
		}
	} if(global.menu) {
		draw_ingame_menu(global.menu);		
	}
	
	glPopMatrix();
	
	draw_hud();
	
	if(global.frames < FADE_TIME)
		fade_out(1.0 - global.frames/(float)FADE_TIME);
	if(global.menu && global.menu->selected == 1) {
		fade_out(global.menu->fade);
	}
}

void apply_bg_shaders() {
	if(global.boss && global.boss->current && global.boss->current->draw_rule) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, resources.fbg.fbo);
		global.boss->current->draw_rule(global.boss, global.frames - global.boss->current->starttime);
		
		glPushMatrix();
			glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);
			glRotatef(global.frames*7.0, 0, 0, -1);
			
			int t;
			if((t = global.frames - global.boss->current->starttime) < 0) {
				float f = 1.0 - t/(float)ATTACK_START_DELAY;
				glScalef(f,f,f);
			}
			
			draw_texture(0,0,"boss_spellcircle0");
		glPopMatrix();
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, resources.fsec.fbo);
	
	if(global.boss) { // Boss background shader
		GLuint shader = get_shader("boss_zoom");
		glUseProgram(shader);
		
		complex fpos = VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I);
		complex pos = fpos + 15*cexp(I*global.frames/4.5);
		
		glUniform2f(glGetUniformLocation(shader, "blur_orig"),
					creal(pos)/resources.fbg.nw, cimag(pos)/resources.fbg.nh);
		glUniform2f(glGetUniformLocation(shader, "fix_orig"),
					creal(fpos)/resources.fbg.nw, cimag(fpos)/resources.fbg.nh);
		glUniform1f(glGetUniformLocation(shader, "blur_rad"), 0.2+0.025*sin(global.frames/15.0));
		glUniform1f(glGetUniformLocation(shader, "rad"), 0.24);
		glUniform1f(glGetUniformLocation(shader, "ratio"), (float)resources.fbg.nh/resources.fbg.nw);
	}
	
	draw_fbo_viewport(&resources.fbg);
	
	glUseProgramObjectARB(0);
	
	if(global.frames - global.plr.recovery < 0) {
		float t = BOMB_RECOVERY - global.plr.recovery + global.frames;
		float fade = 1;
	
		if(t < BOMB_RECOVERY/6)
			fade = t/BOMB_RECOVERY*6;
		
		if(t > BOMB_RECOVERY/4*3)
			fade = 1-t/BOMB_RECOVERY*4 + 3;
	
		glPushMatrix();
		glTranslatef(-30,-30,0);
		fade_out(fade*0.6);
		glPopMatrix();
	}
}

void stage_logic() {
	if(global.menu) {
		ingame_menu_logic(&global.menu);
		return;
	}
	
	player_logic(&global.plr);
	
	process_enemies(&global.enemies);
	process_projectiles(&global.projs, True);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, False);
	
	if(global.boss && !global.dialog) {
		process_boss(global.boss);
		if(global.boss->dmg > global.boss->attacks[global.boss->acount-1].dmglimit)
			boss_death(&global.boss);
	}
	
	global.frames++;
	
	if(!global.dialog && !global.boss)
		global.timer++;
	
	calc_fps(&global.fps);
	
}

void stage_end() {
	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();
	
	if(global.menu) {
		destroy_menu(global.menu);
		global.menu = NULL;
	}
	
	if(global.dialog) {
		delete_dialog(global.dialog);
		global.dialog = NULL;
	}
	
	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}
}

void stage_loop(StageRule start, StageRule end, StageRule draw, StageRule event) {	
	stage_start();
	start();
	
	while(global.game_over <= 0) {
		if(!global.boss && !global.dialog)
			event();
		stage_input();
		stage_logic();		
		
		if(!tconfig.intval[NO_SHADER])
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, resources.fbg.fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw();
		stage_draw();				
		
		SDL_GL_SwapBuffers();
		frame_rate(&global.lasttime);
	}
	
	end();
	stage_end();
}
		
