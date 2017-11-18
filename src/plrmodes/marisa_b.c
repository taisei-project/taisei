/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "global.h"
#include "plrmodes.h"
#include "marisa.h"

static void marisa_star(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(t*10, 0, 0, 1);
	ProjDrawCore(p, p->color);
	glPopMatrix();
}

static void marisa_star_trail_draw(Projectile *p, int t) {
	float s = 1 - t / creal(p->args[0]);

	Color clr = derive_color(p->color, CLRMASK_A, rgba(0, 0, 0, s*0.5));

    // s = 1 + t / creal(p->args[0]);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
    // glRotatef(t*10, 0, 0, 1);
    glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	glScalef(s, s, 1);
	ProjDrawCore(p, clr);
	glColor4f(1,1,1,1);
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static int marisa_star_projectile(Projectile *p, int t) {
	float c = 0.3 * psin(t * 0.2);
	p->color = rgb(1 - c, 0.7 + 0.3 * psin(t * 0.1), 0.9 + c/3);

	int r = accelerated(p, t);
    p->angle = t * 10;

	PARTICLE(
		.texture_ptr = get_tex("proj/maristar"),
		.pos = p->pos,
		.color = p->color,
		.rule = timeout,
        .args = { 8 },
		.draw_rule = marisa_star_trail_draw,
		.type = PlrProj,
        .angle = p->angle,
        .flags = PFLAG_DRAWADD,
	);

	if(t == EVENT_DEATH) {
		PARTICLE(
			.texture_ptr = get_tex("proj/maristar"),
			.pos = p->pos,
			.color = p->color,
			.rule = timeout,
			.draw_rule = GrowFade,
			.args = { 40, 2 },
			.type = PlrProj,
			.angle = frand() * 2 * M_PI,
			.flags = PFLAG_DRAWADD,
		);
	}

	return r;
}

static int marisa_star_slave(Enemy *e, int t) {
	double focus = global.plr.focus/30.0;

    for(int i = 0; i < 3; ++i) {
        if(player_should_shoot(&global.plr, true) && !((global.frames+2*i) % 15)) {
            complex v = e->args[1] * 2;
            complex a = e->args[2];

            v = creal(v) * (1 - 5 * focus) + I * cimag(v) * (1 - 2.5 * focus);
            a = creal(a) * focus * -0.0525 + I * cimag(a) * 2;

            v *= cexp(I*i*M_PI/20*sign(v));
            a *= cexp(I*i*M_PI/20*sign(v)*focus);

            PROJECTILE(
                .texture_ptr = get_tex("proj/maristar"),
                .pos = e->pos,
                .color = rgb(1.0, 0.5, 1.0),
                .rule = marisa_star_projectile,
                // .draw_rule = marisa_star,
                .args = { v, a },
                .type = PlrProj + e->args[3],
                .color_transform_rule = proj_clrtransform_particle,
            );
        }
    }

	e->pos = global.plr.pos + (1 - focus)*e->pos0 + focus*e->args[0];

	return 1;
}


static int marisa_star_orbit_star(Projectile *p, int t) { // XXX: because growfade is the worst
	p->args[1] += p->args[1]/cabs(p->args[1])*0.05;
	return timeout_linear(p,t);
}

static Color marisa_slaveclr(int i, float low) {
	Color slaveclrs[] = {
		rgb(1.,low,low),
		rgb(1.,1.,low),
		rgb(low,1.,low),
		rgb(low,1.,1.),
		rgb(1.,low,1.)
	};

	return slaveclrs[i];
}

static int marisa_star_orbit(Enemy *e, int t) {
	Color color = marisa_slaveclr(rint(creal(e->args[0])),0.5);

	float tb = player_get_bomb_progress(&global.plr, NULL);
	if(t == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	}
	if(t < 0) {
		return 1;
	}

	if(tb >= BOMB_RECOVERY) {
		global.shake_view = 0;
		return ACTION_DESTROY;
	}

	double r = 100*pow(tanh(t/20.),2);
	complex dir = e->args[1]*r*cexp(I*(sqrt(1000+t*t+0.01*t*t*t))*0.04);
	e->pos = global.plr.pos+dir;

	float clr[4];
	parse_color_array(color,clr);
	float fadetime = BOMB_RECOVERY*3./4;
	if(tb >= fadetime) {
		clr[3] = 1-(tb-fadetime)/(BOMB_RECOVERY-fadetime);
	}

	if(t%1 == 0) {
		PARTICLE(
			.texture_ptr=get_tex("part/lightningball"),
			.pos=e->pos,
			.color=rgba(clr[0],clr[1],clr[2],clr[3]/2),
			.rule=timeout,
			.draw_rule=Fade,
			.flags=PFLAG_DRAWADD,
			.args = { 10, 2 },
			.type = PlrProj,
		);
	}

	if(t%(10-t/30) == 0 && tb < fadetime) {
		PARTICLE(
			.texture="maristar_orbit",
			.pos=e->pos,
			.color=rgba(clr[0],clr[1],clr[2],1),
			.rule=marisa_star_orbit_star,
			.draw_rule=GrowFade,
			.flags=PFLAG_DRAWADD,
			.args = { 150, -5*dir/cabs(dir), 3 },
			.type = PlrProj,
		);
	}

	return 1;
}

static void marisa_star_orbit_visual(Enemy *e, int t, bool render) {
    if(!render) {
        return;
    }

	float tb = player_get_bomb_progress(&global.plr, NULL);
	Color color = marisa_slaveclr(rint(creal(e->args[0])),0.2);

	float fade = 1;

	if(t < BOMB_RECOVERY/6.) {
		fade = tb/BOMB_RECOVERY*6;
		fade = sqrt(fade);
	}

	if(t > BOMB_RECOVERY/4.*3) {
		fade = 1-tb/BOMB_RECOVERY*4 + 3;
		fade *= fade;
	}

	float clr[4];
	parse_color_array(color,clr);
	clr[3] = fade;

	glPushMatrix();
	glTranslatef(creal(e->pos),cimag(e->pos),0);
	glPushMatrix();
	glRotatef(carg(e->pos-global.plr.pos)*180/M_PI+90,0,0,1);

	int sparktime = 0;
	glScalef(40*fade,VIEWPORT_H,1);
	glTranslatef(0,-0.5,0);
	marisa_common_masterspark_draw(tb-sparktime);

	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(clr[0],clr[1],clr[2],clr[3]);
	glRotatef(t*10,0,0,1);
	draw_texture(0,0,"fairy_circle");
	glScalef(0.6,0.6,1);
	draw_texture(0,0,"part/lightningball");
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);
}


static void marisa_star_bomb(Player *plr) {
	play_sound("bomb_marisa_b");

	int count = 5; // might as well be hard coded. We are talking marisa here.
	for(int i = 0; i < 5; i++) {
		complex dir = cexp(2*I*M_PI/count*i);
		create_enemy2c(plr->pos,ENEMY_IMMUNE,marisa_star_orbit_visual,marisa_star_orbit,i,dir);
	}
}

static double marisa_star_speed_mod(Player *plr, double speed) {
    if(global.frames - plr->recovery < 0) {
        speed /= 5.0;
    }

    return speed;
}

static void marisa_star_bombbg(Player *plr) {
	float t = player_get_bomb_progress(&global.plr, NULL);
	float fade = 1;

	if(t < BOMB_RECOVERY/6)
		fade = t/BOMB_RECOVERY*6;

	if(t > BOMB_RECOVERY/4*3)
		fade = 1-t/BOMB_RECOVERY*4 + 3;

	Shader *s = get_shader("maristar_bombbg");
	glUseProgram(s->prog);
	glUniform1f(uniloc(s,"t"), t/BOMB_RECOVERY);
	glUniform2f(uniloc(s,"plrpos"), creal(global.plr.pos)/VIEWPORT_W,cimag(global.plr.pos)/VIEWPORT_H);
	glColor4f(1,1,1,0.6*fade);
	fill_screen(0,0,1,"marisa_bombbg");
	glColor4f(1,1,1,1);
	glUseProgram(0);
}

static void marisa_star_respawn_slaves(Player *plr, short npow) {
	Enemy *e = plr->slaves, *tmp;
    double dmg = 56;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}

	if(npow / 100 == 1) {
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
	}

	if(npow >= 200) {
		create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I+10, -0.3-2.0*I,  1-0.1*I, dmg);
		create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I-10,  0.3-2.0*I, -1-0.1*I, dmg);
	}

	if(npow / 100 == 3) {
		create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
	}

	if(npow >= 400) {
		create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave,  25+30.0*I, -0.6-2.0*I,  2-0.1*I, dmg);
		create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, -25+30.0*I,  0.6-2.0*I, -2-0.1*I, dmg);
	}
}

static void marisa_star_power(Player *plr, short npow) {
	if(plr->power / 100 == npow / 100) {
		return;
	}

	marisa_star_respawn_slaves(plr, npow);
}

static void marisa_star_init(Player *plr) {
	marisa_star_respawn_slaves(plr, plr->power);
}

static void marisa_star_shot(Player *plr) {
    int p = plr->power / 100;
    marisa_common_shot(plr, 175 - 10*p);
}

static void marisa_star_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_TEXTURE, flags,
		"proj/marisa",
		"proj/maristar",
		"part/maristar_orbit",
		"marisa_bombbg",
		NULL);

	preload_resources(RES_SHADER, flags,
		"maristar_bombbg",
		NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_b",
		NULL);
}

PlayerMode plrmode_marisa_b = {
	.name = "Star Sign",
	.character = &character_marisa,
	.shot_mode = PLR_SHOT_MARISA_STAR,
	.procs = {
		.bomb = marisa_star_bomb,
		.bombbg = marisa_star_bombbg,
        .shot = marisa_star_shot,
		.speed_mod = marisa_star_speed_mod,
		.power = marisa_star_power,
		.preload = marisa_star_preload,
		.init = marisa_star_init,
	},
};
