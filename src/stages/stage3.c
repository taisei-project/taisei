/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "stage3.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage3_events.h"

typedef struct Stage3State {
	float shadeamp;
	
	float clr_r;
	float clr_g;
	float clr_b;
	
	float fog_exp;
	
	float tunnel_angle;
	float tunnel_avel;
	float tunnel_updn;
	float tunnel_side;
} Stage3State;

static Stage3D bgcontext;
static Stage3State stgstate;

Vector **stage3_bg_pos(Vector pos, float maxrange) {
	//Vector p = {100 * cos(global.frames / 52.0), 100, 50 * sin(global.frames / 50.0)};
	Vector p = {
		stgstate.tunnel_side * cos(global.frames / 52.0),
		0,
		stgstate.tunnel_updn * sin(global.frames / 50.0)
	};
	Vector r = {0, 3000, 0};
	
	return linear3dpos(pos, maxrange, p, r);
}

void stage3_bg_tunnel_draw(Vector pos) {
	int n = 6;
	float r = 300;
	int i;
	
	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	
	glBindTexture(GL_TEXTURE_2D, get_tex("stage3/border")->gltex);
	for(i = 0; i < n; i++) {
		glPushMatrix();
		glRotatef(360.0/n*i + stgstate.tunnel_angle, 0, 1, 0);
		glTranslatef(0,0,-r);
		glScalef(2*r/tan((n-2)*M_PI/n), 3000, 1);
		/*
		glColor4f(
					1.0 - 0.3 * stgstate.shadeamp * (0.5 + 0.5 * sin(1337.1337 + global.frames / 9.3)),
					1.0 - stgstate.shadeamp * (0.5 + 0.5 * cos(global.frames / 11.3)),
					1.0 - stgstate.shadeamp * (0.5 + 0.5 * sin(global.frames / 10.0)),
					1.0
		);
		*/
		draw_quad();
		glPopMatrix();
}
	
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

void stage3_tunnel(int fbonum) {
	Shader *shader = get_shader("tunnel");
	
	glColor4f(1,1,1,1);
	glUseProgram(shader->prog);
	glUniform3f(uniloc(shader, "color"),
		stgstate.clr_r - 0.3 * stgstate.shadeamp * (0.5 + 0.5 * sin(1337.1337 + global.frames / 9.3)),
		stgstate.clr_g - stgstate.shadeamp * (0.5 + 0.5 * cos(global.frames / 11.3)),
		stgstate.clr_b - stgstate.shadeamp * (0.5 + 0.5 * sin(global.frames / 10.0))
	);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);
	
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage3_fog(int fbonum) {
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

void stage3_start(void) {
	init_stage3d(&bgcontext);
	
	start_bgm("bgm_stage3");
 	bgcontext.cx[2] = -10;
	bgcontext.crot[0] = -95;
	bgcontext.cv[1] = 20;
	
	add_model(&bgcontext, stage3_bg_tunnel_draw, stage3_bg_pos);
	
	memset(&stgstate, 0, sizeof(Stage3State));
	stgstate.clr_r = 1.0;
	stgstate.clr_g = 0.0;
	stgstate.clr_b = 0.5;

// TESTING
//	bgcontext.cv[1] = -90;
//	stgstate.fog_exp = 5.0;
}

void stage3_end(void) {
	free_stage3d(&bgcontext);
}

void stage3_draw(void) {
	TIMER(&global.timer)
	
	set_perspective(&bgcontext, 300, 5000);
	stgstate.tunnel_angle += stgstate.tunnel_avel * 5;
 	bgcontext.crot[2] = -(creal(global.plr.pos)-VIEWPORT_W/2)/40.0;
#if 1
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
	
	FROM_TO(1060, 1400, 1) {
		stgstate.clr_r -= 1.0 / 340.0;
		stgstate.clr_g += 1.0 / 340.0;
		stgstate.clr_b -= 0.5 / 340.0;
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
	
	// 2800 - MIDBOSS
	
	FROM_TO(2900, 3100, 1) {
		bgcontext.cv[1] -= 90 / 200.0;
		stgstate.tunnel_avel -= 1 / 200.0;
		stgstate.fog_exp -= 1.0 / 200.0;
		stgstate.clr_r += 0.5 / 200.0;
		stgstate.clr_g -= 1.0 / 200.0;
		stgstate.clr_b += 1.0 / 200.0;
	}
	
	FROM_TO(3200, 3450, 1) {
		stgstate.shadeamp += 0.1 / 250.0;
	}
	
	FROM_TO(3300, 3360, 1) {
		stgstate.tunnel_avel += 2 / 60.0;
		stgstate.tunnel_side += 70 / 60.0;
	}
	
	FROM_TO(3600, 3700, 1) {
		stgstate.tunnel_side += 20 / 60.0;
		stgstate.tunnel_updn += 40 / 60.0;
	}
	
	FROM_TO(3830, 3950, 1) {
		stgstate.tunnel_avel -= 2 / 120.0;
	}
	
	FROM_TO(3960, 4000, 1) {
		stgstate.tunnel_avel += 2 / 40.0;
	}
	
	FROM_TO(4360, 4390, 1) {
		stgstate.clr_r -= 1.0 / 30.0;
		stgstate.clr_g += 0.5 / 30.0;
	}
	
	FROM_TO(4390, 4510, 1) {
		stgstate.clr_r += 1.0 / 120.0;
		stgstate.clr_g -= 0.5 / 120.0;
	}
	
	FROM_TO(4299, 5299, 1) {
		stgstate.tunnel_side -= 90 / 1000.0;
		stgstate.tunnel_updn -= 40 / 1000.0;
		stgstate.clr_r -= 0.5 / 1000.0;
		bgcontext.crot[0] += 7 / 1000.0;
		stgstate.shadeamp -= 0.15 / 1000.0;
		stgstate.fog_exp -= 3.0 / 1000.0;
	}
	
	// 5300 - BOSS
	
	FROM_TO(5099, 5299, 1) {
		bgcontext.cv[1] += 90 / 200.0;
		stgstate.tunnel_avel -= 1.1 / 200.0;
		bgcontext.crot[0] -= 15 / 200.0;
		stgstate.fog_exp += 3.0 / 200.0;
	}
	
	FROM_TO(5301, 5500, 1) {
		bgcontext.cv[1] -= 70 / 200.0;
		stgstate.clr_r += 1.1 / 200.0;
		stgstate.clr_b -= 0.6 / 200.0;
	}
	
	FROM_TO(5301, 5700, 1) {
		bgcontext.crot[0] -= 10 / 400.0;
		stgstate.fog_exp -= 4.0 / 400.0;
		//stgstate.tunnel_avel -= 0.5 / 200.0;
	}
#endif

	draw_stage3d(&bgcontext, 7000);
}

void stage3_loop(void) {
	ShaderRule shaderrules[] = { stage3_fog, stage3_tunnel, NULL };
	stage_loop(stage_get(3), stage3_start, stage3_end, stage3_draw, stage3_events, shaderrules, 5700);
}
