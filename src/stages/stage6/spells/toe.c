/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../extra.h"
#include "../elly.h"
#include "stagetext.h"
#include "common_tasks.h"

static bool elly_toe_its_yukawatime(cmplx pos) {
	int t = global.frames-global.boss->current->starttime;

	if(pos == 0)
		return t>YUKAWATIME;

	int sectors = 4;

	pos -= global.boss->pos;
	if(((int)(carg(pos)/2/M_PI*sectors+sectors+0.005*t))%2)
		return t > YUKAWATIME+0.2*cabs(pos);

	return false;
}

static int elly_toe_fermion_yukawa_effect(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Projectile *master = REF(p->args[0]);

	if(master) {
		p->pos = master->pos;
	}

	return ACTION_NONE;
}

static int elly_toe_fermion(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		p->pos0 = 0;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int speeduptime = creal(p->args[2]);
	if(t > speeduptime)
		p->pos0 += p->pos0/cabs(p->pos0)*(t-speeduptime)*0.01;
	else
		p->pos0 += (p->args[0]-p->pos0)*0.01;
	p->pos = global.boss->pos+p->pos0*cexp(I*0.01*t)+5*cexp(I*t*0.05+I*p->args[1]);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	if(t > 0 && t % 5 == 0) {
		double particle_scale = fmin(1.0, 0.5 * p->sprite->w / 28.0);

		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = &thiscolor_additive,
			.timeout = fmin(t / 6.0, 10),
			.draw_rule = ScaleFade,
			.args = { 0, 0, particle_scale * (0.5 + 2 * I) },
			.angle = M_PI*2*frand(),
		);
	}

	if(creal(p->args[3]) < 0) // add a way to disable the yukawa phase
		return ACTION_NONE;

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_bigball);
			p->args[3]=1;
			play_sfx_ex("shot_special1", 5, false);

			PARTICLE(
				.sprite = "blast",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.rule = elly_toe_fermion_yukawa_effect,
				.timeout = 60,
				.draw_rule = ScaleFade,
				.args = { add_ref(p), 0, 0 + 1 * I },
				.angle = M_PI*2*frand(),
			);

		}

		p->pos0*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_ball);
		p->args[3]=0;
	}

	return ACTION_NONE;
}

static int elly_toe_higgs(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_rice);
			p->args[3]=1;
		}
		p->args[0]*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_flea);
		p->args[3]=0;
	}

	int global_time = global.frames - global.boss->current->starttime - HIGGSTIME;
	int max_time = SYMMETRYTIME - HIGGSTIME;
	double rotation = 0.5 * M_PI * fmax(0, (int)global.diff - D_Normal);

	if(global_time > max_time) {
		global_time = max_time;
	}

	cmplx vel = p->args[0] * cexp(I*rotation*global_time/(float)max_time);
	p->pos = p->pos0 + t * vel;
	p->angle = carg(vel);

	return ACTION_NONE;
}

static cmplx elly_toe_laser_pos(Laser *l, float t) { // a[0]: direction, a[1]: type, a[2]: width
	int type = creal(l->args[1]+0.5);

	if(t == EVENT_BIRTH) {
		switch(type) {
		case 0:
			l->shader = r_shader_get_optional("lasers/elly_toe_fermion");
			l->color = *RGBA(0.4, 0.4, 1.0, 0.0);
			break;
		case 1:
			l->shader = r_shader_get_optional("lasers/elly_toe_photon");
			l->color = *RGBA(1.0, 0.4, 0.4, 0.0);
			break;
		case 2:
			l->shader = r_shader_get_optional("lasers/elly_toe_gluon");
			l->color = *RGBA(0.4, 1.0, 0.4, 0.0);
			break;
		case 3:
			l->shader = r_shader_get_optional("lasers/elly_toe_higgs");
			l->color = *RGBA(1.0, 0.4, 1.0, 0.0);
			break;
		default:
			log_fatal("Unknown Elly laser type.");
		}
		return 0;
	}

	double width = creal(l->args[2]);
	switch(type) {
	case 0:
		return l->pos+l->args[0]*t;
	case 1:
		return l->pos+l->args[0]*(t+width*I*sin(t/width));
	case 2:
		return l->pos+l->args[0]*(t+width*(0.6*(cos(3*t/width)-1)+I*sin(3*t/width)));
	case 3:
		return l->pos+l->args[0]*(t+floor(t/width)*width);
	}

	return 0;
}


// This spell contains some obscure (and some less obscure) physics references.
// However the first priority was good looks and playability, so most of the
// references are pretty bad.
//
// I am waiting for the day somebody notices this and writes a 10-page comment
// about it on reddit. ;_; So I add a disclaimer beforehand (2018). Maybe it can also
// serve as entertainment for the interested hacker stumbling across these
// lines.
//
// Disclaimer:
//
// 1. This spell is based on the Standard Model of Particle physics, which is
//	  very complicated, so the picture here is always very very very dumbed down.
//
// 2. The first two phases stand for the bosons and fermions in the model. My
//	  way of representing them was to choose a heavily overlapping pattern for
//	  the bosons and a non-overlapping one for the fermions (nod to the Pauli
//	  principle). The rest of them is not a good reference at all.
//
// 3. Especially this rgb rotating thing the fermions do has not much to do
//	  with physics. Maybe it represents color charge? And the rotation
//	  represents the way fermions transform strangely under rotations?
//
// 4. The symmetry thing is one fm closer to the truth. In reality the symmetry
//	  that is broken in the phase we live in is continuous, but that is not easy
//	  to do in danmaku, so I took a mirror symmetry and broke it.
//	  Then in reality, the higgs gets a funny property that makes the other
//	  things which interact with it get a mass.
//	  In the spell, I represented that by adding those sectors where the fermions
//	  become big (massive ;)).
//
// 5. The last part is a nod to Feynman diagrams and perturbation theory. Have
//	  been planning to add those to the game for quite some time. They are one
//	  of the iconic sights in modern physics and they can be used as laser danmaku.
//	  The rules here are again simplified:
//
//	  I only use QED and QCD (not the whole SM) and leave out ghosts and the 4
//	  gluon vertex (because I can’t draw dashed lasers and the 4-vertex doesn’t
//	  fit into the pattern).
//
//	  Of course, drawing laser danmaku is easy. The actual calculations behind
//	  the diagrams are one of the most complicated fields in Physics (fun fact:
//	  they are a driving force for CAS software and high precision integration
//	  methods). This whole thing is called perturbation theory and one of its
//	  problems is that it doesn’t really converge, so that’s the reason behind
//	  “Perturbation theory breaking down”
//
// (9). I’m only human though and this is not exactly the stuff I do, so I’m
//		not an expert and more things might be off.
//

void elly_theory(Boss *b, int time) {
	if(time == EVENT_BIRTH) {
		stage_shake_view(50);
		boss_set_portrait(b, "elly", "beaten", "shouting");
		return;
	}

	if(time < 0) {
		GO_TO(b, ELLY_TOE_TARGET_POS, 0.1);
		return;
	}

	TIMER(&time);

	AT(0) {
		assert(cabs(b->pos - ELLY_TOE_TARGET_POS) < 1);
		b->pos = ELLY_TOE_TARGET_POS;
		elly_clap(b,50000);
	}

	int fermiontime = FERMIONTIME;
	int higgstime = HIGGSTIME;
	int symmetrytime = SYMMETRYTIME;
	int yukawatime = YUKAWATIME;
	int breaktime = BREAKTIME;

	{
		int count = 30;
		int step = 1;
		int start_time = 0;
		int end_time = fermiontime + 1000;
		int cycle_time = count * step - 1;
		int cycle_step = 200;
		int solo_cycles = (fermiontime - start_time) / cycle_step - global.diff + 1;
		int warp_cycles = (higgstime - start_time) / cycle_step - 1;

		if(time - start_time <= cycle_time) {
			--count; // get rid of the spawn safe spot on first cycle
			cycle_time -= step;
		}

		FROM_TO_INT(start_time, end_time, cycle_step, cycle_time, step) {
			play_sfx("shot2");

			int pnum = _ni;
			int num_warps = 0;

			if(_i < solo_cycles) {
				num_warps = global.diff;
			} else if(_i < warp_cycles && global.diff > D_Normal) {
				num_warps = 1;
			}

			// The fact that there are 4 bullets per trail is actually one of the physics references.
			for(int i = 0; i < 4; i++) {
				pnum = (count - pnum - 1);

				cmplx dir = I*cexp(I*(2*M_PI/count*(pnum+0.5)));
				dir *= cexp(I*0.15*sign(creal(dir))*sin(_i));

				cmplx bpos = b->pos + 18 * dir * i;

				PROJECTILE(
					.proto = pp_rice,
					.pos = b->pos,
					.color = boson_color(&(Color){0}, i, 0),
					.rule = elly_toe_boson,
					.args = {
						2.5*dir,
						num_warps * (1 + I),
						42*2 - step * _ni + i*I, // real: activation delay, imag: pos in trail (0 is furtherst from head)
						bpos,
					},
					.max_viewport_dist = 20,
				);
			}
		}
	}

	FROM_TO(fermiontime,yukawatime+250,2) {
		// play_loop("noise1");
		play_sfx_ex("shot1", 5, false);

		cmplx dest = 100*cexp(I*1*_i);
		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = b->pos,
				.color = RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					dest,
					clr*2*M_PI/3,
					 40,
				},
				.max_viewport_dist = 50,
				.flags = PFLAG_NOSPAWNFLARE,
			);
		}
	}

	AT(fermiontime) {
		play_sfx("charge_generic");
		stage_shake_view(10);
	}

	AT(symmetrytime) {
		play_sfx("charge_generic");
		play_sfx("boom");
		stagetext_add("Symmetry broken!", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		stage_shake_view(10);

		PARTICLE(
			.sprite = "blast",
			.pos = b->pos,
			.color = RGBA(1.0, 0.3, 0.3, 0.0),
			.timeout = 60,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 5*I },
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		for(int i = 0; i < 5; ++i) {
			PARTICLE(
				.sprite = "stain",
				.pos = b->pos,
				.color = RGBA(0.3, 0.3, 1.0, 0.0),
				.timeout = 100 + 20 * nfrand(),
				.draw_rule = ScaleFade,
				.args = { 0, 0, 2 * (0.4 + 2 * I) },
				.angle = M_PI*2*frand(),
			);
		}
	}

	AT(yukawatime) {
		play_sfx("charge_generic");
		stagetext_add("Coupling the Higgs!", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		stage_shake_view(10);
	}

	AT(breaktime) {
		play_sfx("charge_generic");
		stagetext_add("Perturbation theory", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		stagetext_add("breaking down!", VIEWPORT_W/2+I*VIEWPORT_H/4+30*I, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		stage_shake_view(10);
	}

	FROM_TO(higgstime,yukawatime+100,4+4*(time>symmetrytime)) {
		play_sfx_loop("shot1_loop");

		int arms;

		switch(global.diff) {
			case D_Hard:	arms = 5; break;
			case D_Lunatic: arms = 7; break;
			default:		arms = 4; break;
		}

		for(int dir = 0; dir < 2; dir++) {
			for(int arm = 0; arm < arms; arm++) {
				cmplx v = -2*I*cexp(I*M_PI/(arms+1)*(arm+1)+0.1*I*sin(time*0.1+arm));
				if(dir)
					v = -conj(v);
				if(time>symmetrytime) {
					int t = time-symmetrytime;
					v*=cexp(-I*0.001*t*t+0.01*frand()*dir);
				}
				PROJECTILE(
					.proto = pp_flea,
					.pos = b->pos,
					.color = RGB(dir*(time>symmetrytime),0,1),
					.rule = elly_toe_higgs,
					.args = { v },
					.flags = PFLAG_NOSPAWNFLARE,
				);
			}
		}
	}

	FROM_TO(breaktime,breaktime+10000,100) {
		play_sfx_ex("laser1", 0, true);

		cmplx phase = cexp(2*I*M_PI*frand());
		int count = 8;
		for(int i = 0; i < count; i++) {
			create_laser(b->pos, LASER_LENGTH, LASER_LENGTH/2, RGBA(1, 1, 1, 0),
				elly_toe_laser_pos, elly_toe_laser_logic,
				2*cexp(2*I*M_PI/count*i)*phase,
				0,
				LASER_EXTENT,
				0
			);
		}
	}

	FROM_TO(breaktime+35, breaktime+10000, 14) {
		play_sfx("redirect");
		// play_sound("shot_special1");

		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE(
				.proto = pp_soul,
				.pos = b->pos,
				.color = RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					50*cexp(1.3*I*_i),
					clr*2*M_PI/3,
					40,
					-1,
				},
				.max_viewport_dist = 50,
				.flags = PFLAG_NOSPAWNFLARE,
			);
		}
	}

	FROM_TO_SND("noise1", yukawatime - 60, breaktime + 10000, 1) {};
}

