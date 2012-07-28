/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage6.h"
#include "stage6_events.h"

#include "stage.h"
#include "stageutils.h"

#include "global.h"

static Stage3D bgcontext;

Vector **stage6_towertop_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 70};
	
	return single3dpos(pos, maxrange, p);
}

void stage6_towertop_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/towertop")->gltex);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glScalef(28,28,28);
	draw_model("towertop");
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
}

void stage6_draw() {
	set_perspective(&bgcontext, 100, 1000);
	draw_stage3d(&bgcontext, 30000);
	
	float w = 0.01;
	float f = 1;
	if(bgcontext.cx[2] >= 290) {
		f = 0;
		if(bgcontext.crot[2] >= 360)
			w = 0;
	}
	
	
	bgcontext.cx[0] += -230*w*f*sin(w*global.frames-M_PI/2);
	bgcontext.cx[1] += 230*w*f*cos(w*global.frames-M_PI/2);
	bgcontext.cx[2] += w*f*140/M_PI;
	
	bgcontext.crot[2] += 180/M_PI*w;
	
	printf("%f\n", creal(bgcontext.crot[2]));
}

void stage6_start() {
	init_stage3d(&bgcontext);
	
	add_model(&bgcontext, stage6_towertop_draw, stage6_towertop_pos);
	
// 	bgcontext.cx[0] = 230;
	bgcontext.cx[1] = -230;
	bgcontext.crot[0] = 90;
	bgcontext.crot[2] = -40;
	
	
}

void stage6_end() {
	free_stage3d(&bgcontext);
}

void stage6_loop() {
	stage_loop(stage_get(6), stage6_start, stage6_end, stage6_draw, stage6_events, NULL, 5700);
}