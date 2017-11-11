/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "projectile.h"

#include <stdlib.h>
#include "global.h"
#include "list.h"
#include "vbo.h"

static ProjArgs defaults_proj = {
	.texture = "proj/",
	.draw_rule = ProjDraw,
	.dest = &global.projs,
	.type = EnemyProj,
	.color = RGB(1, 1, 1),
	.color_transform_rule = proj_clrtransform_bullet,
};

static ProjArgs defaults_part = {
	.texture = "part/",
	.draw_rule = ProjDraw,
	.dest = &global.particles,
	.type = Particle,
	.color = RGB(1, 1, 1),
	.color_transform_rule = proj_clrtransform_particle,
};

static void process_projectile_args(ProjArgs *args, ProjArgs *defaults) {
	int texargs = (bool)args->texture + (bool)args->texture_ptr + (bool)args->size;

	if(texargs != 1) {
		log_fatal("Exactly one of .texture, .texture_ptr, or .size is required");
	}

	if(!args->rule) {
		log_fatal(".rule is required");
	}

	if(args->texture) {
		args->texture_ptr = prefix_get_tex(args->texture, defaults->texture);
	}

	if(!args->draw_rule) {
		args->draw_rule = ProjDraw;
	}

	if(!args->dest) {
		args->dest = defaults->dest;
	}

	if(!args->type) {
		args->type = defaults->type;
	}

	if(!args->color) {
		args->color = defaults->color;
	}

	if(!args->color_transform_rule) {
		args->color_transform_rule = defaults->color_transform_rule;
	}

	if(!args->max_viewport_dist && (args->type == Particle || args->type >= PlrProj)) {
		args->max_viewport_dist = 300;
	}
}

static complex projectile_size2(Texture *tex, complex size) {
	if(tex) {
		return tex->w + I*tex->h;
	}

	return size;
}

static complex projectile_size(Projectile *p) {
	return projectile_size2(p->tex, p->size);
}

static void projectile_size_split(Projectile *p, double *w, double *h) {
	assert(w != NULL);
	assert(h != NULL);

	complex c = projectile_size(p);
	*w = creal(c);
	*h = cimag(c);
}

int projectile_prio_rawfunc(Texture *tex, complex size) {
	complex s = projectile_size2(tex, size);
	return -rint(creal(s) * cimag(s));
}

int projectile_prio_func(void *vproj) {
	Projectile *proj = vproj;
	return projectile_prio_rawfunc(proj->tex, proj->size);
}

static Projectile* _create_projectile(ProjArgs *args) {
	Projectile *p = create_element_at_priority(
		(void**)args->dest, sizeof(Projectile),
		projectile_prio_rawfunc(args->texture_ptr, args->size),
		projectile_prio_func
	);

	p->birthtime = global.frames;
	p->pos = p->pos0 = args->pos;
	p->angle = args->angle;
	p->rule = args->rule;
	p->draw_rule = args->draw_rule;
	p->color_transform_rule = args->color_transform_rule;
	p->tex = args->texture_ptr;
	p->type = args->type;
	p->color = args->color;
	p->grazed = (bool)(args->flags & PFLAG_NOGRAZE);
	p->max_viewport_dist = args->max_viewport_dist;
	p->size = args->size;
	p->flags = args->flags;

	memcpy(p->args, args->args, sizeof(p->args));

	// BUG: this currently breaks some projectiles
	//		enable this when they're fixed
	// assert(rule != NULL);
	// rule(p, EVENT_BIRTH);

	return p;
}

Projectile* create_projectile(ProjArgs *args) {
	process_projectile_args(args, &defaults_proj);
	return _create_projectile(args);
}

Projectile* create_particle(ProjArgs *args) {
	process_projectile_args(args, &defaults_part);
	return _create_projectile(args);
}

#ifdef PROJ_DEBUG
Projectile* _proj_attach_dbginfo(Projectile *p, ProjDebugInfo *dbg) {
	memcpy(&p->debug, dbg, sizeof(ProjDebugInfo));
	return p;
}
#endif


void _delete_projectile(void **projs, void *proj) {
	Projectile *p = proj;
	p->rule(p, EVENT_DEATH);

	del_ref(proj);
	delete_element(projs, proj);
}

void delete_projectile(Projectile **projs, Projectile *proj) {
	_delete_projectile((void **)projs, proj);
}

void delete_projectiles(Projectile **projs) {
	delete_all_elements((void **)projs, _delete_projectile);
}

int collision_projectile(Projectile *p) {
	if(p->type == EnemyProj) {
		double w, h;
		projectile_size_split(p, &w, &h);

		double angle = carg(global.plr.pos - p->pos) + p->angle;
		double projr = sqrt(pow(w/2*cos(angle), 2) + pow(h/2*sin(angle), 2)) * 0.45;
		double grazer = max(w, h);
		double dst = cabs(global.plr.pos - p->pos);
		grazer = (0.9 * sqrt(grazer) + 0.1 * grazer) * 6;

		if(dst < projr + 1)
			return 1;

		if(!p->grazed && dst < grazer && global.frames - abs(global.plr.recovery) > 0) {
			p->grazed = true;
			player_graze(&global.plr, p->pos - grazer * 0.3 * cexp(I*carg(p->pos - global.plr.pos)), 50);
		}
	} else if(p->type >= PlrProj) {
		Enemy *e = global.enemies;
		int damage = p->type - PlrProj;

		while(e != NULL) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 30) {
				player_add_points(&global.plr, damage * 0.5);

				#ifdef PLR_DPS_STATS
					global.plr.total_dmg += min(e->hp, damage);
				#endif

				e->hp -= damage;
				return 2;
			}
			e = e->next;
		}

		if(global.boss && cabs(global.boss->pos - p->pos) < 42) {
			if(boss_damage(global.boss, damage)) {
				player_add_points(&global.plr, damage * 0.2);

				#ifdef PLR_DPS_STATS
					global.plr.total_dmg += damage;
				#endif

				return 2;
			}
		}
	}
	return 0;
}

void static_clrtransform_bullet(Color c, ColorTransform *out) {
	memcpy(out, &(ColorTransform) {
		.B[0] = c & ~CLRMASK_A,
		.B[1] = rgba(1, 1, 1, 0),
		.A[1] = c &  CLRMASK_A,
	}, sizeof(ColorTransform));
}

void static_clrtransform_particle(Color c, ColorTransform *out) {
	memcpy(out, &(ColorTransform) {
		.R[1] = c & CLRMASK_R,
		.G[1] = c & CLRMASK_G,
		.B[1] = c & CLRMASK_B,
		.A[1] = c & CLRMASK_A,
	}, sizeof(ColorTransform));
}

void proj_clrtransform_bullet(Projectile *p, int t, Color c, ColorTransform *out) {
	static_clrtransform_bullet(c, out);
}

void proj_clrtransform_particle(Projectile *p, int t, Color c, ColorTransform *out) {
	static_clrtransform_particle(c, out);
}

static inline void draw_projectile(Projectile *proj) {
	proj->draw_rule(proj, global.frames - proj->birthtime);

#ifdef PROJ_DEBUG
	int cur_shader;
    glGetIntegerv(GL_CURRENT_PROGRAM, &cur_shader); // NOTE: this can be really slow!

    if(cur_shader != recolor_get_shader()->prog) {
    	log_fatal("Bad shader after drawing projectile. Offending spawn: %s:%u:%s",
    		proj->debug.file, proj->debug.line, proj->debug.func
    	);
    }
#endif
}

void draw_projectiles(Projectile *projs, ProjPredicate predicate) {
	glUseProgram(recolor_get_shader()->prog);

	if(predicate) {
		for(Projectile *proj = projs; proj; proj = proj->next) {
			if(predicate(proj)) {
				draw_projectile(proj);
			}
		}
	} else {
		for(Projectile *proj = projs; proj; proj = proj->next) {
			draw_projectile(proj);
		}
	}

	glUseProgram(0);
}

bool projectile_in_viewport(Projectile *proj) {
	double w, h;
	int e = proj->max_viewport_dist;
	projectile_size_split(proj, &w, &h);

	return !(creal(proj->pos) + w/2 + e < 0 || creal(proj->pos) - w/2 - e > VIEWPORT_W
		  || cimag(proj->pos) + h/2 + e < 0 || cimag(proj->pos) - h/2 - e > VIEWPORT_H);
}

void process_projectiles(Projectile **projs, bool collision) {
	Projectile *proj = *projs, *del = NULL;

	char killed = 0;
	char col = 0;
	int action;
	while(proj != NULL) {
		action = proj->rule(proj, global.frames - proj->birthtime);

		if(proj->type == DeadProj && killed < 5) {
			killed++;
			action = ACTION_DESTROY;
			create_bpoint(proj->pos);
		}

		if(collision)
			col = collision_projectile(proj);

		if(col && proj->type != Particle) {
			PARTICLE(
				.texture_ptr = proj->tex,
				.pos = proj->pos,
				.color = proj->color,
				.flags = proj->flags,
				.color_transform_rule = proj->color_transform_rule,
				.rule = timeout_linear,
				.draw_rule = DeathShrink,
				.args = { 10, 5*cexp(proj->angle*I) },
				.type = proj->type >= PlrProj ? PlrProj : Particle,
			);
		}

		if(col == 1 && global.frames - abs(global.plr.recovery) >= 0)
			player_death(&global.plr);

		if(action == ACTION_DESTROY || col || !projectile_in_viewport(proj)) {
			del = proj;
			proj = proj->next;
			delete_projectile(projs, del);
			if(proj == NULL) break;
		} else {
			proj = proj->next;
		}
	}
}

complex trace_projectile(complex origin, complex size, ProjRule rule, float angle, complex a0, complex a1, complex a2, complex a3, ProjType type, int *out_col) {
	complex target = origin;
	Projectile *p = NULL;

	PROJECTILE(
		.dest = &p,
		.type = type,
		.size = size,
		.pos = origin,
		.rule = rule,
		.angle = angle,
		.args = { a0, a1, a2, a3 },
	);

	for(int t = 0; p; ++t) {
		int action = p->rule(p, t);
		int col = collision_projectile(p);

		if(col || action == ACTION_DESTROY || !projectile_in_viewport(p)) {
			target = p->pos;

			if(out_col) {
				*out_col = col;
			}

			delete_projectile(&p, p);
		}
	}

	return target;
}

int linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t;

	return 1;
}

int accelerated(Projectile *p, int t) {
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);

	return 1;
}

static inline void apply_common_transforms(Projectile *proj, int t) {
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	if(t >= 16) {
		return;
	}

	if(proj->flags & PFLAG_NOSPAWNZOOM) {
		return;
	}

	if(proj->type != EnemyProj && proj->type != FakeProj) {
		return;
	}

	float s = 2.0-t/16.0;
	if(s != 1) {
		glScalef(s, s, 1);
	}
}

static inline void apply_color(Projectile *proj, Color c) {
	static ColorTransform ct;
	proj->color_transform_rule(proj, proj->birthtime - global.frames, c, &ct);
	recolor_apply_transform(&ct);
}

void ProjDrawCore(Projectile *proj, Color c) {
	apply_color(proj, c);

	if(proj->flags & PFLAG_DRAWADD) {
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		draw_texture_p(0, 0, proj->tex);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} else if(proj->flags & PFLAG_DRAWSUB) {
		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		draw_texture_p(0, 0, proj->tex);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
	} else {
		draw_texture_p(0, 0, proj->tex);
	}
}

void ProjDraw(Projectile *proj, int t) {
	glPushMatrix();
	apply_common_transforms(proj, t);
	ProjDrawCore(proj, proj->color);
	glPopMatrix();
}

void ProjNoDraw(Projectile *proj, int t) {
}

void Blast(Projectile *p, int t) {
	if(t == 1) {
		p->args[1] = frand()*360 + frand()*I;
		p->args[2] = frand() + frand()*I;
	}

	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(creal(p->args[1]), cimag(p->args[1]), creal(p->args[2]), cimag(p->args[2]));
	if(t != p->args[0] && p->args[0] != 0)
		glScalef(t/p->args[0], t/p->args[0], 1);

	apply_color(p, rgba(0.3, 0.6, 1.0, 1.0 - t/p->args[0]));

	draw_texture_p(0,0,p->tex);
	glScalef(0.5+creal(p->args[2]),0.5+creal(p->args[2]),1);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	draw_texture_p(0,0,p->tex);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();
}

void Shrink(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = 2.0-t/p->args[0]*2;
	if(s != 1) {
		glScalef(s, s, 1);
	}

	ProjDrawCore(p, p->color);
	glPopMatrix();
}

void DeathShrink(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = 2.0-t/p->args[0]*2;
	if(s != 1) {
		glScalef(s, 1, 1);
	}

	ProjDrawCore(p, p->color);
	glPopMatrix();
}

void GrowFade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);

	float s = t/p->args[0]*(1 + (creal(p->args[2])? p->args[2] : p->args[1]));
	if(s != 1) {
		glScalef(s, s, 1);
	}

	ProjDrawCore(p, multiply_colors(p->color, rgba(1, 1, 1, 1 - t/p->args[0])));
	glPopMatrix();
}

void Fade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);
	ProjDrawCore(p, multiply_colors(p->color, rgba(1, 1, 1, 1 - t/p->args[0])));
	glPopMatrix();
}

void ScaleFade(Projectile *p, int t) {
	glPushMatrix();
	apply_common_transforms(p, t);
	glScalef(creal(p->args[1]), creal(p->args[1]), 1);

	float a = (1.0 - t/creal(p->args[0])) * (1.0 - cimag(p->args[1]));
	Color c = rgba(1, 1, 1, a);

	ProjDrawCore(p, c);
	glPopMatrix();
}

int timeout(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;

	return 1;
}

int timeout_linear(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	p->angle = carg(p->args[1]);
	p->pos = p->pos0 + p->args[1]*t;

	return 1;
}

void Petal(Projectile *p, int t) {
	float x = creal(p->args[2]);
	float y = cimag(p->args[2]);
	float z = creal(p->args[3]);

	float r = sqrt(x*x+y*y+z*z);
	x /= r; y /= r; z /= r;

	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos),0);
	glRotatef(t*4.0 + cimag(p->args[3]), x, y, z);
	ProjDrawCore(p, p->color);
	glPopMatrix();
	glEnable(GL_CULL_FACE);
}

void petal_explosion(int n, complex pos) {
	int i;
	for(i = 0; i < n; i++) {
		tsrand_fill(6);
		float t = frand();
		Color c = rgba(sin(5*t),cos(5*t),0.5,t);

		PARTICLE("petal", pos, c, asymptotic,
			.draw_rule = Petal,
			.args = {
				(3+5*afrand(2))*cexp(I*M_PI*2*afrand(3)),
				5,
				afrand(4) + afrand(5)*I, afrand(1) + 360.0*I*afrand(0),
			},
			.flags = PFLAG_DRAWADD,
		);
	}
}

void projectiles_preload(void) {
	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		// XXX: maybe split this up into stage-specific preloads too?
		// some of these are ubiquitous, but some only appear in very specific parts.

		"part/blast",
		"part/flare",
		"part/lasercurve",
		"part/petal",
		"part/smoke",
		"part/stain",
		"part/lightning0",
		"part/lightning1",
		"part/lightningball",
		"proj/ball",
		"proj/bigball",
		"proj/bullet",
		"proj/card",
		"proj/crystal",
		"proj/flea",
		"proj/hghost",
		"proj/plainball",
		"proj/rice",
		"proj/soul",
		"proj/thickrice",
		"proj/wave",
		"proj/youhoming",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT,
		"shot1",
		"shot2",
		"shot1_loop",
		"shot_special1",
		"redirect",
	NULL);
}
