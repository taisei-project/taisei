/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "fbo.h"
#include "global.h"
#include "taisei_err.h"

void init_fbo(FBO *fbo) {
	if(!GLEW_ARB_framebuffer_object) {
		warnx("missing FBO support. seriously.");
		tconfig.intval[NO_SHADER] = 1;
		return;
	}
	
	glGenTextures(1, &fbo->tex);
	glBindTexture(GL_TEXTURE_2D, fbo->tex);
	
	fbo->nw = 2;
	fbo->nh = 2;
	
	while(fbo->nw < VIEWPORT_W+VIEWPORT_X) fbo->nw *= 2;
	while(fbo->nh < VIEWPORT_H+VIEWPORT_Y) fbo->nh *= 2;
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbo->nw, fbo->nh, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
	
	glGenFramebuffers(1,&fbo->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->tex, 0);
	
	glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	
	glGenTextures(1, &fbo->depth);
	glBindTexture(GL_TEXTURE_2D, fbo->depth);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, fbo->nw, fbo->nh, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,fbo->depth, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_fbo_viewport(FBO *fbo) {	
	glPushMatrix();
	glTranslatef(-VIEWPORT_X,VIEWPORT_H+VIEWPORT_Y-fbo->nh,0);
		
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fbo->tex);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex3f(0, 0, 0);
		glTexCoord2f(0,0); glVertex3f(0, fbo->nh, 0);
		glTexCoord2f(1,0); glVertex3f(fbo->nw, fbo->nh, 0);
		glTexCoord2f(1,1); glVertex3f(fbo->nw, 0, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	glUseProgramObjectARB(0);
	glPopMatrix();
}