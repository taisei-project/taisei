/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "ingamemenu.h"
#include "global.h"

void return_to_game(void *arg) {
}

void return_to_title(void *arg) {
	global.game_over = GAMEOVER_ABORT;
}
	
MenuData *create_ingame_menu() {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	add_menu_entry(m, "Return to Game", return_to_game, NULL);
	add_menu_entry(m, "Return to Title", return_to_title, NULL);
	
	return m;
}

void ingame_menu_logic(MenuData **menu) {
	menu_logic(*menu);
	if((*menu)->quit == 2 && (*menu)->selected != 1) { // let the stage clean up when returning to title
		destroy_menu(*menu);
		free(*menu);
		*menu = NULL;
	}
}

void draw_ingame_menu(MenuData *menu) {
	float rad = IMENU_BLUR;
	if(menu->selected != 1) // hardly hardcoded. 1 -> "Return to Title"
		rad = IMENU_BLUR * (1.0-menu->fade);
	
	if(!tconfig.intval[NO_SHADER]) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
		GLenum shader = get_shader("ingame_menu");
		glUseProgram(shader);	
					
		glUniform1f(glGetUniformLocation(shader, "rad"), rad);
		draw_fbo_viewport(&resources.fsec);
		glUseProgram(0);
	}
	
	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/4, 0);
	
	glColor4f(0.1,0.1,0.2,0.4*rad/IMENU_BLUR);
	glPushMatrix();
	glTranslatef(0, menu->drawdata[0], 0);
	glScalef(menu->drawdata[1],15,1);
		
	glBegin(GL_QUADS);
	glVertex3f(-1,-1,0);
	glVertex3f(-1,1,0);
	glVertex3f(1,1,0);
	glVertex3f(1,-1,0);
	glEnd();
	
	glPopMatrix();
	
	// cirno's perfect math class #2: Euler Sign ~ Differential Fun
	menu->drawdata[0] += (menu->cursor*35 - menu->drawdata[0])/7.0;
	menu->drawdata[1] += (strlen(menu->entries[menu->cursor].name)*5 - menu->drawdata[1])/10.0;
		
	int i;
	for(i = 0; i < menu->ecount; i++) {
		float s = 0;
		if(i == menu->cursor)
			s = 0.3 + 0.2*sin(menu->frames/7.0);
		
		glColor4f(1-s,1-s,1, 1.0 - menu->fade);
		draw_text(AL_Center, 0, i*35, menu->entries[i].name, _fonts.standard);
	}
		
	glColor4f(1,1,1,1);
	glPopMatrix();
}