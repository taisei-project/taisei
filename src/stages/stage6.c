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
static int fall_over;

Vector **stage6_towerwall_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, -220};
	Vector r = {0, 0, 300};
	
	Vector **list = linear3dpos(pos, maxrange, p, r);
	
	int i;
	
	for(i = 0; list[i] != NULL; i++) {
		if((*list[i])[2] > 0)
			(*list[i])[1] = -90000;
	}
	
	return list;
}


void stage6_towerwall_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/towerwall")->gltex);
	if(!tconfig.intval[NO_SHADER]) {
		Shader *s = get_shader("tower_wall");
		glUseProgram(s->prog);
		glUniform1i(uniloc(s, "lendiv"), 3000.0);
	}
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
// 	glRotatef(90, 1,0,0);
	glScalef(30,30,30);
	draw_model("towerwall");
	glPopMatrix();
	
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
}

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

Vector **stage6_skysphere_pos(Vector pos, float maxrange) {
	return single3dpos(pos, maxrange, bgcontext.cx);
}

void stage6_skysphere_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/sky")->gltex);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]-30);
	glScalef(150,150,150);
	draw_model("skysphere");
	glPopMatrix();
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
}

void stage6_draw(void) {
	set_perspective(&bgcontext, 100, 9000);
	draw_stage3d(&bgcontext, 10000);
	
	if(fall_over) {
		int t = global.frames - fall_over;
		TIMER(&t);
		
		FROM_TO(0, 240, 1) {
			bgcontext.cx[0] += 0.02*cos(M_PI/180*bgcontext.crot[2]+M_PI/2)*_i;
			bgcontext.cx[1] += 0.02*sin(M_PI/180*bgcontext.crot[2]+M_PI/2)*_i;
		}		
				
		FROM_TO(150, 1000, 1) {
			bgcontext.crot[0] -= 0.02*(global.frames-fall_over-150);
			if(bgcontext.crot[0] < 0)
				bgcontext.crot[0] = 0;			
		}
		
		if(t >= 190)
			bgcontext.cx[2] -= max(6, 0.05*(global.frames-fall_over-150));
		
		FROM_TO(300, 470,1) {
			bgcontext.cx[0] -= 0.01*cos(M_PI/180*bgcontext.crot[2]+M_PI/2)*_i;
			bgcontext.cx[1] -= 0.01*sin(M_PI/180*bgcontext.crot[2]+M_PI/2)*_i;			
		}
		
		if(t > 470)
			bgcontext.cx[0] += 1-2*frand();
		
	}
	
	float w = 0.002;
	float f = 1, g = 1;
	if(global.timer > 3273) {
		f = max(0, f-0.01*(global.timer-3273));
		
	}
	
	if(global.timer > 3628)
		g = max(0, g-0.01*(global.timer - 3628));
	
	
	bgcontext.cx[0] += -230*w*f*sin(w*global.frames-M_PI/2);
	bgcontext.cx[1] += 230*w*f*cos(w*global.frames-M_PI/2);
	bgcontext.cx[2] += w*f*140/M_PI;
	
	bgcontext.crot[2] += 180/M_PI*g*w;
}

void start_fall_over(void) { //troll
	fall_over = global.frames;
}

void stage6_start(void) {
	init_stage3d(&bgcontext);
	fall_over = 0;
	
	start_bgm("bgm_stage6");
	
	add_model(&bgcontext, stage6_skysphere_draw, stage6_skysphere_pos);
	add_model(&bgcontext, stage6_towertop_draw, stage6_towertop_pos);
	add_model(&bgcontext, stage6_towerwall_draw, stage6_towerwall_pos);
	
	bgcontext.cx[1] = -230;
	bgcontext.crot[0] = 90;
	bgcontext.crot[2] = -40;
	
// 	for testing
// 	bgcontext.cx[0] = 80;
// 	bgcontext.cx[1] = -215;
// 	bgcontext.cx[2] = 295;
// 	bgcontext.crot[0] = 90;
// 	bgcontext.crot[2] = 381.415100;
// 	
	
}

void stage6_end(void) {
	free_stage3d(&bgcontext);
}

void stage6_loop(void) {
// 	ShaderRule shaderrules[] = { stage6_bloom, NULL };
	stage_loop(stage_get(6), stage6_start, stage6_end, stage6_draw, stage6_events, NULL, 3900);
}
