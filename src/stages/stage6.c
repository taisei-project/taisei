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

enum {
	NUM_STARS = 100
};
static float starpos[3*NUM_STARS];

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

	Shader *s = get_shader("tower_wall");
	glUseProgram(s->prog);

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
//	glRotatef(90, 1,0,0);
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
	Shader *s = get_shader("stage6_sky");
	glUseProgram(s->prog);

	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/sky")->gltex);

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]-30);
	glScalef(150,150,150);
	draw_model("skysphere");

	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);

	for(int i = 0; i < NUM_STARS; i++) {
		glPushMatrix();
		float x = starpos[3*i+0], y = starpos[3*i+1], z = starpos[3*i+2];
		glColor4f(0.9,0.9,1,0.8*z);
		glTranslatef(x,y,z);
		glRotatef(180/M_PI*acos(starpos[3*i+2]),-y,x,0);
		glScalef(1./4000,1./4000,1./4000);
		draw_texture(0,0,"part/lasercurve");
		glPopMatrix();
	}
	glPopMatrix();
	glColor4f(1,1,1,1);
	glEnable(GL_DEPTH_TEST);
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

	add_model(&bgcontext, stage6_skysphere_draw, stage6_skysphere_pos);
	add_model(&bgcontext, stage6_towertop_draw, stage6_towertop_pos);
	add_model(&bgcontext, stage6_towerwall_draw, stage6_towerwall_pos);

	for(int i = 0; i < NUM_STARS; i++) {
		float x,y,z,r;
		do {
			x = nfrand();
			y = nfrand();
			z = frand();
			r = sqrt(x*x+y*y+z*z);
		} while(0 < r < 1);
		starpos[3*i+0]= x/r;
		starpos[3*i+1]= y/r;
		starpos[3*i+2]= z/r;
	}

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

void stage6_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "bgm_stage6", "bgm_stage6boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage6/baryon_connector",
		"stage6/baryon",
		"stage6/scythecircle",
		"stage6/scythe",
		"stage6/sky",
		"stage6/spellbg_chalk",
		"stage6/spellbg_classic",
		"stage6/spellbg_modern",
		"stage6/towertop",
		"stage6/towerwall",
		"dialog/elly",
	NULL);
	preload_resources(RES_SHADER, RESF_DEFAULT,
		"tower_wall",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"elly",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"towerwall",
		"towertop",
		"skysphere",
	NULL);
}

void stage6_end(void) {
	free_stage3d(&bgcontext);
}

void elly_intro(Boss*, int);
void elly_unbound(Boss*, int);

void stage6_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		skip_background_anim(&bgcontext, stage6_draw, 3800, &global.timer, &global.frames);
		global.boss = create_boss("Elly", "elly", "dialog/elly", BOSS_DEFAULT_SPAWN_POS);

		// Here be dragons

		ptrdiff_t spellnum = global.stage->spell - stage6_spells;
		char go = true;

		switch(spellnum) {
			case 0: // Newton Sign ~ 2.5 Laws of Movement
				// Scythe required - this creates it
				boss_add_attack(global.boss, AT_Move, "Catch the Scythe", 2, 0, elly_intro, NULL);
				go = false;
				break;

			case 1: // Maxwell Sign ~ Wave Theory
				// Works fine on its own
				break;

			case 2: // Eigenstate ~ Many-World Interpretation
			case 3: // Ricci Sign ~ Space Time Curvature
			case 4: // LHC ~ Higgs Boson Uncovered
				// Baryon required - this creates it
				boss_add_attack(global.boss, AT_Move, "Unbound", 3, 0, elly_unbound, NULL);
				go = false;
				break;

			case 5: // Tower of Truth ~ Theory of Everything
				// Works fine on its own
				// Just needs a little extra to make us fall from the tower
				start_fall_over();
				skip_background_anim(&bgcontext, stage6_draw, 300, &global.timer, &global.frames);
				break;
		}

		boss_add_attack_from_info(global.boss, global.stage->spell, go);
		start_attack(global.boss, global.boss->attacks);

		start_bgm("bgm_stage6boss");
	}

	if(!global.boss) {
		killall(global.enemies);
	}
}

ShaderRule stage6_shaders[] = { NULL };

StageProcs stage6_procs = {
	.begin = stage6_start,
	.preload = stage6_preload,
	.end = stage6_end,
	.draw = stage6_draw,
	.event = stage6_events,
	.shader_rules = stage6_shaders,
	.spellpractice_procs = &stage6_spell_procs,
};

StageProcs stage6_spell_procs = {
	.preload = stage6_preload,
	.begin = stage6_start,
	.end = stage6_end,
	.draw = stage6_draw,
	.event = stage6_spellpractice_events,
	.shader_rules = stage6_shaders,
};
