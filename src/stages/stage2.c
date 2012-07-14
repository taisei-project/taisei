/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage2.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage2_events.h"

static Stage3D bgcontext;

Vector **stage2_bg_pos(Vector pos, float maxrange) {
	Vector p = {-320, 0, 0};
	Vector r = {0, 3000, 0};
	
	return linear3dpos(pos, maxrange, p, r);
}

void stage2_bg_tunnel_draw(Vector pos) {
	int n = 7;
	float r = 300;
	int i;
	
	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
		
	for(i = 0; i < n; i++) {
		glPushMatrix();
		glRotatef(360/n*i, 0, 1, 0);
		glTranslatef(0,0,-r);
		glScalef(2*r/tan((n-2)*M_PI/n), 3000, 1);
		glBindTexture(GL_TEXTURE_2D, get_tex("stage1/border")->gltex);
		
		draw_quad();
		glPopMatrix();		
	}
	
// 	bgcontext.crot[1] = (creal(global.plr.pos)-VIEWPORT_W/2)/10.0;
	
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

void stage2_fog(int fbonum) {
	Shader *shader = get_shader("zbuf_fog");
	
	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"),2);
	glUniform4f(uniloc(shader, "fog_color"),1,1,1,1.0);
	glUniform1f(uniloc(shader, "start"),0.2);
	glUniform1f(uniloc(shader, "end"),0.8);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage2_start() {
	init_stage3d(&bgcontext);
	
// 	bgcontext.cx[2] = -10;
	bgcontext.crot[0] = -70;
	bgcontext.cv[1] = 20;
	
	add_model(&bgcontext, stage2_bg_tunnel_draw, stage2_bg_pos);
	
}

void stage2_end() {
	free_stage3d(&bgcontext);
}

void stage2_draw() {
	TIMER(&global.frames)
	
	set_perspective(&bgcontext, 500, 5000);
	
	FROM_TO(0, 160, 1)
		bgcontext.cv[1] -= 0.5;
	
	draw_stage3d(&bgcontext, 7000);
}

void stage2_loop() {
	ShaderRule shaderrules[] = { stage2_fog, NULL };
	stage_loop(stage_get(3), stage2_start, stage2_end, stage2_draw, stage2_events, shaderrules, 5500);
}
