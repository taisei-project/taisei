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

void delete_fbo(FBO *fbo) {
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteTextures(1, &fbo->depth);
	glDeleteTextures(1, &fbo->tex);
}

void draw_fbo_viewport(FBO *fbo) {	
	glPushMatrix();
	glTranslatef(-VIEWPORT_X,VIEWPORT_H+VIEWPORT_Y-fbo->nh,0);
		
	glEnable(GL_TEXTURE_2D);
	
	glTranslatef(fbo->nw/2,fbo->nw/2,0);
	glScalef(fbo->nw, fbo->nh, 1);
		
	glBindTexture(GL_TEXTURE_2D, fbo->tex);
	
	glBindBuffer(GL_ARRAY_BUFFER, _quadvbo);	
	glDrawArrays(GL_QUADS, 4, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	
	
	glDisable(GL_TEXTURE_2D);
	
	glPopMatrix();
}