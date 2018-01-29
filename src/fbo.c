/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "fbo.h"
#include "global.h"
#include "util.h"

static void init_fbo(FBO *fbo, float scale, int type) {
	glGenTextures(1, &fbo->tex);
	glBindTexture(GL_TEXTURE_2D, fbo->tex);

	fbo->scale = scale = sanitize_scale(scale);
	fbo->nw = scale * VIEWPORT_W;
	fbo->nh = scale * VIEWPORT_H;

	log_debug("FBO %p: q=%f, w=%i, h=%i", (void*)fbo, fbo->scale, fbo->nw, fbo->nh);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, type, fbo->nw, fbo->nh, 0, type, GL_UNSIGNED_BYTE, NULL);

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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void delete_fbo(FBO *fbo) {
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteTextures(1, &fbo->depth);
	glDeleteTextures(1, &fbo->tex);
}

static void reinit_fbo(FBO *fbo, float scale, int type) {
	if(!fbo->scale) {
		// fbo was never initialized
		init_fbo(fbo, scale, type);
		return;
	}

	if(fbo->scale != sanitize_scale(scale)) {
		delete_fbo(fbo);
		init_fbo(fbo, scale, type);
	}
}

static void swap_fbos(FBO **fbo1, FBO **fbo2) {
	FBO *temp = *fbo1;
	*fbo1 = *fbo2;
	*fbo2 = temp;
}

void init_fbo_pair(FBOPair *pair, float scale, int type) {
	pair->front = pair->_fbopair_private.array + FBO_FRONT;
	pair->back = pair->_fbopair_private.array + FBO_BACK;

	reinit_fbo(pair->front, scale, type);
	reinit_fbo(pair->back, scale, type);
}

void delete_fbo_pair(FBOPair *pair) {
	delete_fbo(pair->front);
	delete_fbo(pair->back);
}

void swap_fbo_pair(FBOPair *pair) {
	swap_fbos(&pair->front, &pair->back);
}

void draw_fbo(FBO *fbo) {
	// this floor is very important, because the size of the fbo is integer
	// and rendering it perfectly is necessary for non-blurry graphics.
	float wq = floor(fbo->scale*VIEWPORT_W)/(float)fbo->nw;
	float hq = floor(fbo->scale*VIEWPORT_H)/(float)fbo->nh;
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(wq, hq, 1);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
		glTranslatef(VIEWPORT_W/2., VIEWPORT_H/2., 0);
		glScalef(VIEWPORT_W, VIEWPORT_H, 1);
		glBindTexture(GL_TEXTURE_2D, fbo->tex);
		glDrawArrays(GL_QUADS, 4, 4);
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void draw_fbo_viewport(FBO *fbo) {
	// assumption: rendering into another, identical FBO

	glViewport(0, 0, fbo->scale*VIEWPORT_W, fbo->scale*VIEWPORT_H);
	set_ortho_ex(VIEWPORT_W,VIEWPORT_H);
	draw_fbo(fbo);
}
