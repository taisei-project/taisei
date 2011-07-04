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
	
	float speed = 0.01*VIEWPORT_W/((global.plr.focus > 0)+1);	
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	global.plr.moving = False;
	
	if(keys[tconfig.intval[KEY_LEFT]] && !keys[tconfig.intval[KEY_RIGHT]]) {
		global.plr.moving = True;
		global.plr.dir = 1;
	} else if(keys[tconfig.intval[KEY_RIGHT]] && !keys[tconfig.intval[KEY_LEFT]]) {
		global.plr.moving = True;
		global.plr.dir = 0;
	}	
	
	if(keys[tconfig.intval[KEY_LEFT]] && creal(global.plr.pos) - global.plr.ani->w/2 - speed > 0)
		global.plr.pos -= speed;		
	if(keys[tconfig.intval[KEY_RIGHT]] && creal(global.plr.pos) + global.plr.ani->w/2 + speed < VIEWPORT_W)
		global.plr.pos += speed;
	if(keys[tconfig.intval[KEY_UP]] && cimag(global.plr.pos) - global.plr.ani->h/2 - speed > 0)
		global.plr.pos -= I*speed;
	if(keys[tconfig.intval[KEY_DOWN]] && cimag(global.plr.pos) + global.plr.ani->h/2 + speed < VIEWPORT_H)
		global.plr.pos += I*speed;
	
	if(!keys[tconfig.intval[KEY_SHOT]] && global.plr.fire)
		global.plr.fire = False;
	if(!keys[tconfig.intval[KEY_FOCUS]] && global.plr.focus > 0)
		global.plr.focus = -30;
}

void draw_hud() {
	draw_texture(SCREEN_W/2, SCREEN_H/2, "hud");	
	
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
}

void stage_draw() {
	set_ortho();
	
	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
		
	if(!global.menu) {		
		if(!tconfig.intval[NO_SHADER])
			apply_bg_shaders();

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
			draw_fbo_viewport(&resources.fsec);
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
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, resources.fsec.fbo);
		
	if(global.boss) { // Boss background shader
		GLuint shader = get_shader("boss_zoom");
		glUseProgram(shader);
		
		complex fpos = VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I)*0.5;
		complex pos = fpos + 15*cexp(I*global.frames/5.0);
		
		glUniform2f(glGetUniformLocation(shader, "blur_orig"),
					creal(pos)/resources.fbg.nw, cimag(pos)/resources.fbg.nh);
		glUniform2f(glGetUniformLocation(shader, "fix_orig"),
					creal(fpos)/resources.fbg.nw, cimag(fpos)/resources.fbg.nh);
		glUniform1f(glGetUniformLocation(shader, "blur_rad"), 0.2+0.05*sin(global.frames/10.0));
		glUniform1f(glGetUniformLocation(shader, "rad"), 0.3);
		glUniform1f(glGetUniformLocation(shader, "ratio"), (float)resources.fbg.nh/resources.fbg.nw);
	}
	
	draw_fbo_viewport(&resources.fbg);
	
	glUseProgramObjectARB(0);
		
	if(global.boss && global.boss->current && global.boss->current->draw_rule) {
		glPushMatrix();
		glTranslatef(-VIEWPORT_X,0,0); // Some transformation for convenience. Don't ask why. it's because of rtt.
		glScalef((VIEWPORT_W+VIEWPORT_X)/(float)VIEWPORT_W, (VIEWPORT_H+VIEWPORT_Y)/(float)VIEWPORT_H, 1);
		global.boss->current->draw_rule(global.boss, global.frames - global.boss->current->starttime);
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
	
	if(global.boss) {
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
		
