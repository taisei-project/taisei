/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4.h"
#include "stage4_events.h"

#include "stage.h"
#include "stageutils.h"

#include "global.h"

static Stage3D bgcontext;

Vector **stage4_stairs_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 0};
	Vector r = {0, 0, 3000};
	
	return linear3dpos(pos, maxrange, p, r);
}

void stage4_stairs_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage4/tower")->gltex);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glScalef(300,300,300);
	draw_model("tower");
	
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
}

void stage4_draw() {
	set_perspective(&bgcontext, 100, 9000);
	
	draw_stage3d(&bgcontext, 7000);
	float w = 0.005;
	
	bgcontext.cx[0] = 2500*cos(-w*global.frames);
	bgcontext.cx[1] = 2500*sin(-w*global.frames);
	bgcontext.cx[2] = w*3000/(M_PI)*global.frames;
	
// 	bgcontext.cv[2] = w*3000/(2*M_PI);
	
// 	bgcontext.crot[0] = cimag(global.plr.pos);
	bgcontext.crot[2] = 150-180/M_PI*w*global.frames;
}

void stage4_start() {
	init_stage3d(&bgcontext);
	add_model(&bgcontext, stage4_stairs_draw, stage4_stairs_pos);
	
	bgcontext.crot[0] = 60;
	
}

void stage4_end() {
	free_stage3d(&bgcontext);
}

void stage4_loop() {
// 	ShaderRule shaderrules[] = { stage3_fog, NULL };
	stage_loop(stage_get(4), stage4_start, stage4_end, stage4_draw, stage4_events, NULL, 5550);
}