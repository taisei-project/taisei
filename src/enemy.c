/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "enemy.h"

#include <stdlib.h>
#include "global.h"
#include "projectile.h"
#include "list.h"
#include "aniplayer.h"
#include "stageobjects.h"
#include "glm.h"
#include "entity.h"

#ifdef create_enemy_p
#undef create_enemy_p
#endif

#ifdef DEBUG
Enemy* _enemy_attach_dbginfo(Enemy *e, DebugInfo *dbg) {
	memcpy(&e->debug, dbg, sizeof(DebugInfo));
	set_debug_info(dbg);
	return e;
}
#endif

static void ent_draw_enemy(EntityInterface *ent);

Enemy *create_enemy_p(Enemy **enemies, complex pos, int hp, EnemyVisualRule visual_rule, EnemyLogicRule logic_rule,
				  complex a1, complex a2, complex a3, complex a4) {
	if(IN_DRAW_CODE) {
		log_fatal("Tried to spawn an enemy while in drawing code");
	}

	// XXX: some code relies on the insertion logic
	Enemy *e = (Enemy*)list_insert(enemies, objpool_acquire(stage_object_pools.enemies));
	e->moving = false;
	e->dir = 0;

	e->birthtime = global.frames;
	e->pos = pos;
	e->pos0 = pos;

	e->hp = hp;
	e->alpha = 1.0;

	e->logic_rule = logic_rule;
	e->visual_rule = visual_rule;

	e->args[0] = a1;
	e->args[1] = a2;
	e->args[2] = a3;
	e->args[3] = a4;

	e->ent.draw_layer = LAYER_ENEMY;
	e->ent.draw_func = ent_draw_enemy;
	ent_register(&e->ent, ENT_ENEMY);

	e->logic_rule(e, EVENT_BIRTH);
	return e;
}

void* _delete_enemy(List **enemies, List* enemy, void *arg) {
	Enemy *e = (Enemy*)enemy;

	if(e->hp <= 0 && e->hp > ENEMY_IMMUNE) {
		play_sound("enemydeath");

		for(int i = 0; i < 10; i++) {
			tsrand_fill(2);

			PARTICLE("flare", e->pos, 0, timeout_linear, .draw_rule = Fade,
				.args = { 10, (3+afrand(0)*10)*cexp(I*afrand(1)*2*M_PI) },
			);
		}

		PARTICLE("blast", e->pos, 0, blast_timeout, { 20 }, .draw_rule = Blast, .flags = PFLAG_REQUIREDPARTICLE);
		PARTICLE("blast", e->pos, 0, blast_timeout, { 20 }, .draw_rule = Blast, .flags = PFLAG_REQUIREDPARTICLE);
		PARTICLE("blast", e->pos, 0, blast_timeout, { 15 }, .draw_rule = GrowFade, .flags = PFLAG_REQUIREDPARTICLE);
	}

	e->logic_rule(e, EVENT_DEATH);
	del_ref(enemy);
	ent_unregister(&e->ent);
	objpool_release(stage_object_pools.enemies, (ObjectInterface*)list_unlink(enemies, enemy));

	return NULL;
}

void delete_enemy(Enemy **enemies, Enemy* enemy) {
	_delete_enemy((List**)enemies, (List*)enemy, NULL);
}

void delete_enemies(Enemy **enemies) {
	list_foreach(enemies, _delete_enemy, NULL);
}

static void ent_draw_enemy(EntityInterface *ent) {
	Enemy *e = ENT_CAST(ent, Enemy);

	if(!e->visual_rule) {
		return;
	}

#ifdef ENEMY_DEBUG
	static Enemy prev_state;
	memcpy(&prev_state, e, sizeof(Enemy));
#endif

	if(e->alpha < 1) {
		r_color4(1, 1, 1, e->alpha);
	}

	e->visual_rule(e, global.frames - e->birthtime, true);

#ifdef ENEMY_DEBUG
	if(memcmp(&prev_state, e, sizeof(Enemy))) {
		set_debug_info(&e->debug);
		log_fatal("Enemy modified its own state in draw rule");
	}
#endif
}

void killall(Enemy *enemies) {
	Enemy *e;

	for(e = enemies; e; e = e->next)
		e->hp = 0;
}

int enemy_flare(Projectile *p, int t) { // a[0] timeout, a[1] velocity, a[2] ref to enemy
	if(t >= creal(p->args[0]) || REF(p->args[2]) == NULL) {
		return ACTION_DESTROY;
	} if(t == EVENT_DEATH) {
		free_ref(p->args[2]);
		return 1;
	} else if(t < 0) {
		return 1;
	}

	p->pos += p->args[1];

	return 1;
}

void EnemyFlareShrink(Projectile *p, int t) {
	Enemy *e = (Enemy *)REF(p->args[2]);

	if(e == NULL) {
		return;
	}

	r_mat_push();
	float s = 2.0-t/p->args[0]*2;

	r_mat_translate(creal(e->pos + p->pos), cimag(e->pos + p->pos), 0);

	if(p->angle != M_PI*0.5) {
		r_mat_rotate_deg(p->angle*180/M_PI+90, 0, 0, 1);
	}

	if(s != 1) {
		r_mat_scale(s, s, 1);
	}

	ProjDrawCore(p, p->color);
	r_mat_pop();
}

void BigFairy(Enemy *e, int t, bool render) {
	if(!render) {
		if(!(t % 5)) {
			complex offset = (frand()-0.5)*30 + (frand()-0.5)*20.0*I;

			PARTICLE("smoothdot", offset, rgb(0,0.2,0.3), enemy_flare,
				.draw_rule = EnemyFlareShrink,
				.args = { 50, (-50.0*I-offset)/50.0, add_ref(e) },
				.flags = PFLAG_DRAWADD,
			);
		}

		return;
	}

	float s = sin((float)(global.frames-e->birthtime)/10.f)/6 + 0.8;

	r_draw_sprite(&(SpriteParams) {
		.sprite = "fairy_circle",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = global.frames * 10 * DEG2RAD,
		.scale.both = s,
	});

	const char *seqname = !e->moving ? "main" : (e->dir ? "left" : "right");
	Animation *ani = get_ani("enemy/bigfairy");
	Sprite *spr = animation_get_frame(ani,get_ani_sequence(ani, seqname),global.frames);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = spr,
		.pos = { creal(e->pos), cimag(e->pos) },
	});
}

void Fairy(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	float s = sin((float)(global.frames-e->birthtime)/10.f)/6 + 0.8;

	r_draw_sprite(&(SpriteParams) {
		.sprite = "fairy_circle",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = global.frames * 10 * DEG2RAD,
		.scale.both = s,
	});

	const char *seqname = !e->moving ? "main" : (e->dir ? "left" : "right");
	Animation *ani = get_ani("enemy/fairy");
	Sprite *spr = animation_get_frame(ani,get_ani_sequence(ani, seqname),global.frames);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = spr,
		.pos = { creal(e->pos), cimag(e->pos) },
	});
}

void Swirl(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite = "enemy/swirl",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = t * 10 * DEG2RAD,
	});
}

void process_enemies(Enemy **enemies) {
	Enemy *enemy = *enemies, *del = NULL;

	while(enemy != NULL) {
		int action = enemy->logic_rule(enemy, global.frames - enemy->birthtime);

		if(enemy->hp > ENEMY_IMMUNE && enemy->alpha >= 1.0 && cabs(enemy->pos - global.plr.pos) < 7) {
			player_death(&global.plr);
		}

		enemy->alpha = approach(enemy->alpha, 1.0, 1.0/60.0);

		if((enemy->hp > ENEMY_IMMUNE
		&& (creal(enemy->pos) < -20 || creal(enemy->pos) > VIEWPORT_W + 20
		|| cimag(enemy->pos) < -20 || cimag(enemy->pos) > VIEWPORT_H + 20
		|| enemy->hp <= 0)) || action == ACTION_DESTROY) {
			del = enemy;
			enemy = enemy->next;
			delete_enemy(enemies, del);
		} else {
			if(enemy->visual_rule) {
				enemy->visual_rule(enemy, global.frames - enemy->birthtime, false);
			}

			enemy = enemy->next;
		}
	}
}

void enemies_preload(void) {
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"enemy/fairy",
		"enemy/bigfairy",
	NULL);

	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"fairy_circle",
		"enemy/swirl",
	NULL);

	preload_resources(RES_SFX, RESF_OPTIONAL,
		"enemydeath",
	NULL);
}
