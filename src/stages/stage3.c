/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage3.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage3_events.h"

static Stage3D bgcontext;

void stage3_fog(int fbonum) {
	Shader *shader = get_shader("zbuf_fog");
	
	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"),2);
	glUniform4f(uniloc(shader, "fog_color"),0,0,0.1,1.0);
	glUniform1f(uniloc(shader, "start"),0.2);
	glUniform1f(uniloc(shader, "end"),0.4);
	glUniform1f(uniloc(shader, "exponent"),4.0);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

Vector **stage3_lake_pos(Vector pos, float maxrange) {
	Vector p = {-60, 600, 0};
	Vector d;
	
	int i;
	
	for(i = 0; i < 3; i++)
		d[i] = p[i] - pos[i];
	
	if(length(d) > maxrange) {
		return NULL;
	} else {
		Vector **list = calloc(2, sizeof(Vector*));
		
		list[0] = malloc(sizeof(Vector));
		for(i = 0; i < 3; i++)
			(*list[0])[i] = p[i];
		list[1] = NULL;
		
		return list;
	}	
}

void stage3_lake_draw(Vector pos) {	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage3/lake")->gltex);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glScalef(20,20,20);
	glRotatef(180,1,0,0);
	
	draw_model("lake");
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
}

void stage3_start() {
	init_stage3d(&bgcontext);
	
	bgcontext.cx[2] = 120;
	bgcontext.cv[1] = 0.1;
	bgcontext.crot[0] = 90;
	
	add_model(&bgcontext, stage3_lake_draw, stage3_lake_pos);
}

void stage3_end() {
	free_stage3d(&bgcontext);
}

void stage3_draw() {
	set_perspective(&bgcontext, 200, 10000);
	
	draw_stage3d(&bgcontext, 7000);
}

void stage3_loop() {
	ShaderRule shaderrules[] = { stage3_fog, NULL };
	stage_loop(stage3_start, stage3_end, stage3_draw, stage3_events, shaderrules, 5500);
}