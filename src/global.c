/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include <SDL/SDL.h>
#include <time.h>
#include <err.h>
#include "font.h"

Global global;

void init_global() {
	memset(&global, 0, sizeof(global));	
	
	alGenSources(SNDSRC_COUNT, global.sndsrc);
	
	srand(time(0));
	
	load_resources();
	init_fonts();
	init_rtt();
}

void init_rtt() {
	glEnable(GL_BLEND);
	
	glGenTextures(1, &global.rtt.tex);
	glBindTexture(GL_TEXTURE_2D, global.rtt.tex);
	
	global.rtt.nw = 2;
	global.rtt.nh = 2;
	
	while(global.rtt.nw < VIEWPORT_W) global.rtt.nw *= 2;
	while(global.rtt.nh < VIEWPORT_H) global.rtt.nh *= 2;
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global.rtt.nw, global.rtt.nh, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
	
	glGenFramebuffersEXT(1,&global.rtt.fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, global.rtt.fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, global.rtt.tex, 0);
	
	glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	
	glGenRenderbuffersEXT(1, &global.rtt.depth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, global.rtt.depth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, global.rtt.nw, global.rtt.nh);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, global.rtt.depth);
	
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
		warnx("!- GPU seems not to support Framebuffer Objects");
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void game_over() {
	global.game_over = 1;
	printf("Game Over!\n");
}

void frame_rate() {
	int t = global.lasttime + 1000/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	global.lasttime = SDL_GetTicks();
}