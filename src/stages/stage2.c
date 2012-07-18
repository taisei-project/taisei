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

typedef struct Stage2State {
	float shadeamp;
	
	float fog_exp;
	
	float tunnel_angle;
	float tunnel_avel;
	float tunnel_updn;
	float tunnel_side;
} Stage2State;

static Stage3D bgcontext;
static Stage2State stgstate;

Vector **stage2_bg_pos(Vector pos, float maxrange) {
	//Vector p = {100 * cos(global.frames / 52.0), 100, 50 * sin(global.frames / 50.0)};
	Vector p = {
		stgstate.tunnel_side * cos(global.frames / 52.0),
		0,
		stgstate.tunnel_updn * sin(global.frames / 50.0)
	};
	Vector r = {0, 3000, 0};
	
	return linear3dpos(pos, maxrange, p, r);
}

void stage2_bg_tunnel_draw(Vector pos) {
	int n = 6;
	float r = 300;
	int i;
	
	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	
	glBindTexture(GL_TEXTURE_2D, get_tex("stage2/border")->gltex);
	for(i = 0; i < n; i++) {
		glPushMatrix();
		glRotatef(360.0/n*i + stgstate.tunnel_angle, 0, 1, 0);
		glTranslatef(0,0,-r);
		glScalef(2*r/tan((n-2)*M_PI/n), 3000, 1);
		glColor4f(
					1.0 - 0.3 * stgstate.shadeamp * (0.5 + 0.5 * sin(1337.1337 + global.frames / 9.3)),
					1.0 - stgstate.shadeamp * (0.5 + 0.5 * cos(global.frames / 11.3)),
					1.0 - stgstate.shadeamp * (0.5 + 0.5 * sin(global.frames / 10.0)),
					1.0
		);
						
		draw_quad();
		glPopMatrix();
	}
	
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

void stage2_fog(int fbonum) {
	Shader *shader = get_shader("zbuf_fog");
	
	glColor4f(1,1,1,1);
	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"), 2);
	glUniform4f(uniloc(shader, "fog_color"), 1, 1, 1, 1.0);
	glUniform1f(uniloc(shader, "start"), 0.2);
	glUniform1f(uniloc(shader, "end"), 0.8);
	glUniform1f(uniloc(shader, "exponent"), stgstate.fog_exp + 0.5 * sin(global.frames / 50.0));
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage2_start() {
	init_stage3d(&bgcontext);
	
 	bgcontext.cx[2] = -10;
	bgcontext.crot[0] = -95;
	bgcontext.cv[1] = 20;
	
	add_model(&bgcontext, stage2_bg_tunnel_draw, stage2_bg_pos);
	memset(&stgstate, 0, sizeof(Stage2State));
}

void stage2_end() {
	free_stage3d(&bgcontext);
}

void stage2_draw() {
	TIMER(&global.timer)
	
	set_perspective(&bgcontext, 300, 5000);
	stgstate.tunnel_angle += stgstate.tunnel_avel * 5;
 	bgcontext.crot[2] = -(creal(global.plr.pos)-VIEWPORT_W/2)/40.0;
	
	FROM_TO(0, 160, 1)
		bgcontext.cv[1] -= 0.5;
	
	FROM_TO(0, 500, 1)
		stgstate.fog_exp += 5.0 / 500.0;
	
	FROM_TO(400, 500, 1) {
		stgstate.tunnel_avel += 0.005;
		bgcontext.cv[1] -= 0.3;
	}
	
	FROM_TO(1000, 1100, 1)
		stgstate.shadeamp += 0.0007;
	
	FROM_TO(1050, 1150, 1) {
		stgstate.tunnel_avel -= 0.010;
		bgcontext.cv[1] -= 0.2;
	}
	
	FROM_TO(1170, 1400, 1)
		stgstate.tunnel_side += 100.0 / 230.0;
	
	FROM_TO(1400, 1550, 1) {
		bgcontext.crot[0] -= 3 / 150.0;
		stgstate.tunnel_updn += 70.0 / 150.0;
		stgstate.tunnel_avel += 1 / 150.0;
		bgcontext.cv[1] -= 0.2;
	}
	
	FROM_TO(1570, 1700, 1) {
		stgstate.tunnel_updn -= 20 / 130.0;
		stgstate.shadeamp -= 0.007 / 130.0;
	}
	
	FROM_TO(1800, 1850, 1)
		stgstate.tunnel_avel -= 0.02;
	
	FROM_TO(1900, 2000, 1) {
		stgstate.tunnel_avel += 0.013;
		stgstate.shadeamp -= 0.00015;
	}
	
	FROM_TO(2000, 2740, 1) {
		stgstate.tunnel_side -= 100.0 / 740.0;
		stgstate.fog_exp -= 1.0 / 740.0;
		bgcontext.crot[0] += 11 / 740.0;
	}
	
	FROM_TO(2740, 2799, 1) {
		stgstate.fog_exp += 3.0 / 60.0;
		bgcontext.cv[1] += 1.5;
		stgstate.tunnel_avel -= 0.7 / 60.0;
		bgcontext.crot[0] -= 11 / 60.0;
	}
	
	draw_stage3d(&bgcontext, 7000);
}

void stage2_loop() {
	ShaderRule shaderrules[] = { stage2_fog, NULL };
	stage_loop(stage_get(3), stage2_start, stage2_end, stage2_draw, stage2_events, shaderrules, 5500);
}
