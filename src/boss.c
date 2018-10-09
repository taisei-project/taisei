/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "boss.h"
#include "global.h"
#include "stage.h"
#include "stagetext.h"
#include "entity.h"

static void ent_draw_boss(EntityInterface *ent);
static DamageResult ent_damage_boss(EntityInterface *ent, const DamageInfo *dmg);

Boss* create_boss(char *name, char *ani, char *dialog, complex pos) {
	Boss *buf = calloc(1, sizeof(Boss));

	buf->name = strdup(name);
	buf->pos = pos;

	char strbuf[strlen(ani) + sizeof("boss/")];
	snprintf(strbuf, sizeof(strbuf), "boss/%s", ani);
	aniplayer_create(&buf->ani, get_ani(strbuf), "main");

	if(dialog) {
		buf->dialog = get_sprite(dialog);
	}

	buf->birthtime = global.frames;
	buf->zoomcolor = *RGBA(0.1, 0.2, 0.3, 1.0);

	buf->ent.draw_layer = LAYER_BOSS;
	buf->ent.draw_func = ent_draw_boss;
	buf->ent.damage_func = ent_damage_boss;
	ent_register(&buf->ent, ENT_BOSS);

	// This is not necessary because the default will be set at the start of every attack.
	// But who knows. Maybe this will be triggered somewhen. If bosses without attacks start
	// taking over the world, I will be the one who put in this weak point to make them vulnerable.
	buf->bomb_damage_multiplier = 1.0;
	buf->shot_damage_multiplier = 1.0;

	return buf;
}

void draw_boss_text(Alignment align, float x, float y, const char *text, const char *fnt, const Color *clr) {
	ShaderProgram *sh_prev = r_shader_current();
	r_shader("text_default");
	text_draw(text, &(TextParams) {
		.pos = { x + 1, y + 1 },
		.color = RGBA(0, 0, 0, clr->a),
		.font = fnt,
		.align = align,
	});

	text_draw(text, &(TextParams) {
		.pos = { x, y },
		.color = clr,
		.font = fnt,
		.align = align,
	});
	r_shader_ptr(sh_prev);
}

void spell_opening(Boss *b, int time) {
	complex x0 = VIEWPORT_W/2+I*VIEWPORT_H/3.5;
	float f = clamp((time-40.)/60.,0,1);
	complex x = x0 + (VIEWPORT_W+I*35 - x0) * f*(f+1)*0.5;
	int strw = text_width(get_font("standard"), b->current->name, 0);

	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);
	r_disable(RCAP_CULL_FACE);

	r_mat_push();
	r_mat_translate(creal(x),cimag(x),0);
	float scale = f+1.*(1-f)*(1-f)*(1-f);
	r_mat_scale(scale,scale,1);
	r_mat_rotate_deg(360*f,1,1,0);
	draw_boss_text(ALIGN_RIGHT, strw/2*(1-f), 0, b->current->name, "standard", RGB(1, 1, 1));
	r_mat_pop();

	r_capability(RCAP_CULL_FACE, cullcap_saved);
}

void draw_extraspell_bg(Boss *boss, int time) {
	// overlay for all extra spells
	// FIXME: Please replace this with something that doesn't look like shit.

	float opacity = 0.7;
	r_color4(0.2 * opacity, 0.1 * opacity, 0, 0);
	fill_viewport(sin(time) * 0.015, time / 50.0, 1, "stage3/wspellclouds");
	r_color4(2000, 2000, 2000, 0);
	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MIN,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_MIN
	));
	fill_viewport(cos(time) * 0.015, time / 70.0, 1, "stage4/kurumibg2");
	fill_viewport(sin(time*1.1+2.1) * 0.015, time / 30.0, 1, "stage4/kurumibg2");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

const Color* boss_healthbar_color(AttackType atype) {
	static const Color colors[] = {
		[AT_Normal]        = { 1.00, 1.00, 1.00, 1.00 },
		[AT_Move]          = { 1.00, 1.00, 1.00, 1.00 },
		[AT_Spellcard]     = { 1.00, 0.65, 0.65, 1.00 },
		[AT_SurvivalSpell] = { 0.50, 0.50, 1.00, 1.00 },
		[AT_ExtraSpell]    = { 1.00, 0.30, 0.20, 1.00 },
		[AT_Immediate]     = { 1.00, 1.00, 1.00, 1.00 },
	};

	assert(atype >= 0 && atype < sizeof(colors)/sizeof(*colors));
	return colors + atype;
}

static StageProgress* get_spellstage_progress(Attack *a, StageInfo **out_stginfo, bool write) {
	if(!write || (global.replaymode == REPLAY_RECORD && global.stage->type == STAGE_STORY)) {
		StageInfo *i = stage_get_by_spellcard(a->info, global.diff);
		if(i) {
			StageProgress *p = stage_get_progress_from_info(i, global.diff, write);

			if(out_stginfo) {
				*out_stginfo = i;
			}

			if(p) {
				return p;
			}
		}
#if DEBUG
		else if((a->type == AT_Spellcard || a->type == AT_ExtraSpell) && global.stage->type != STAGE_SPECIAL) {
			log_warn("FIXME: spellcard '%s' is not available in spell practice mode!", a->name);
		}
#endif
	}

	return NULL;
}

static bool boss_should_skip_attack(Boss *boss, Attack *a) {
	// if we failed any spells on this boss up to this point, skip any extra spells,
	// as well as any attacks associated with them.
	//
	// (for example, the "Generic move" that might have been automatically added by
	// boss_add_attack_from_info. this is what the a->info->type check is for.)
	if(boss->failed_spells && (a->type == AT_ExtraSpell || (a->info && a->info->type == AT_ExtraSpell))) {
		return true;
	}

	// Immediates are handled in a special way by process_boss,
	// but may be considered skipped/nonexistent for other purposes
	if(a->type == AT_Immediate) {
		return true;
	}

	// Skip zero-length spells. Zero-length AT_Move and AT_Normal attacks are ok.
	if(ATTACK_IS_SPELL(a->type) && a->timeout <= 0) {
		return true;
	}

	return false;
}

static Attack* boss_get_final_attack(Boss *boss) {
	Attack *final;
	for(final = boss->attacks + boss->acount - 1; final >= boss->attacks && boss_should_skip_attack(boss, final); --final);
	return final >= boss->attacks ? final : NULL;
}

static Attack* boss_get_next_attack(Boss *boss) {
	Attack *next;
	for(next = boss->current + 1; next < boss->attacks + boss->acount && boss_should_skip_attack(boss, next); ++next);
	return next < boss->attacks + boss->acount ? next : NULL;
}

static bool boss_attack_is_final(Boss *boss, Attack *a) {
	return boss_get_final_attack(boss) == a;
}

static bool attack_is_over(Attack *a) {
	return a->hp <= 0 && global.frames > a->endtime;
}

static void BossGlow(Projectile *p, int t) {
	float s = 1.0+t/(double)p->timeout*0.5;
	float fade = 1 - (1.5 - s);
	float deform = 5 - 10 * fade * fade;
	Color c = p->color;

	c.a = 0;
	color_mul_scalar(&c, 1.5 - s);

	r_draw_sprite(&(SpriteParams) {
		.pos = { creal(p->pos), cimag(p->pos) },
		.sprite_ptr = p->sprite,
		.scale.both = s,
		.color = &c,
		.shader_params = &(ShaderCustomParams){{ deform }},
		.shader_ptr = p->shader,
	});
}

static int boss_glow(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free(p->sprite);
	}

	return linear(p, t);
}

static Projectile* spawn_boss_glow(Boss *boss, const Color *clr, int timeout) {
	// XXX: memdup is required because the Sprite returned by animation_get_frame is only temporarily valid
	return PARTICLE(
		.sprite_ptr = memdup(aniplayer_get_frame(&boss->ani), sizeof(Sprite)),
		// this is in sync with the boss position oscillation
		.pos = boss->pos + 6 * sin(global.frames/25.0) * I,
		.color = clr,
		.rule = boss_glow,
		.draw_rule = BossGlow,
		.timeout = timeout,
		.layer = LAYER_PARTICLE_LOW,
		.shader = "sprite_silhouette",
	);
}

static void spawn_particle_effects(Boss *boss) {
	Color *glowcolor = &boss->glowcolor;
	Color *shadowcolor = &boss->shadowcolor;

	Attack *cur = boss->current;
	bool is_spell = cur && ATTACK_IS_SPELL(cur->type) && !cur->endtime;
	bool is_extra = cur && cur->type == AT_ExtraSpell && global.frames >= cur->starttime;

	if(!(global.frames % 13) && !is_extra) {
		PARTICLE(
			.sprite = "smoke",
			.pos = cexp(I*global.frames),
			.color = RGBA(shadowcolor->r, shadowcolor->g, shadowcolor->b, 0.0),
			.rule = enemy_flare,
			.timeout = 180,
			.draw_rule = Shrink,
			.args = { 0, add_ref(boss), },
			.angle = M_PI * 2 * frand(),
		);
	}

	if(!(global.frames % (2 + 2 * is_extra)) && (is_spell || boss_is_dying(boss))) {
		float glowstr = 0.5;
		float a = (1.0 - glowstr) + glowstr * psin(global.frames/15.0);
		spawn_boss_glow(boss, color_mul_scalar(COLOR_COPY(glowcolor), a), 24);
	}
}

void draw_boss_background(Boss *boss) {
	r_mat_push();
	r_mat_translate(creal(boss->pos), cimag(boss->pos), 0);
	r_mat_rotate_deg(global.frames*4.0, 0, 0, -1);

	float f = 0.8+0.1*sin(global.frames/8.0);

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		f -= t*(t-0.7)/max(0.01, 1-t);
	}

	r_mat_scale(f, f, 1);
	r_draw_sprite(&(SpriteParams) {
		.sprite = "boss_circle",
		.color = RGBA(1, 1, 1, 0),
	});
	r_mat_pop();
}

static void ent_draw_boss(EntityInterface *ent) {
	// TODO: separate overlay from this

	Boss *boss = ENT_CAST(ent, Boss);

	float red = 0.5*exp(-0.5*(global.frames-boss->lastdamageframe));
	if(red > 1)
		red = 0;

	float boss_alpha = 1;

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		boss_alpha = (1 - t) + 0.3;
	}

	r_color(RGBA_MUL_ALPHA(1, 1-red, 1-red/2, boss_alpha));
	draw_sprite_batched_p(creal(boss->pos), cimag(boss->pos) + 6*sin(global.frames/25.0), aniplayer_get_frame(&boss->ani));
	r_color4(1, 1, 1, 1);
}

void draw_boss_hud(Boss *boss) {
	if(!boss->current)
		return;

	if(boss->current->type == AT_Move && global.frames - boss->current->starttime > 0 && boss_attack_is_final(boss, boss->current))
		return;

	draw_boss_text(ALIGN_LEFT, 10, 20, boss->name, "standard", RGB(1, 1, 1));

	if(ATTACK_IS_SPELL(boss->current->type))
		spell_opening(boss, global.frames - boss->current->starttime);

	if(boss->current->type != AT_Move && boss->current->type != AT_Immediate) {
		char buf[16];
		float remaining = max(0, (boss->current->timeout - global.frames + boss->current->starttime)/(float)FPS);
		Color textclr;

		if(remaining < 6) {
			textclr = *RGB(1.0, 0.2, 0.2);
		} else if(remaining < 11) {
			textclr = *RGB(1.0, 1.0, 0.2);
		} else {
			textclr = *RGB(1.0, 1.0, 1.0);
		}

		snprintf(buf, sizeof(buf),  "%.2f", remaining);
		draw_boss_text(ALIGN_CENTER, VIEWPORT_W - 24, 10, buf, "standard", &textclr);

		StageProgress *p = get_spellstage_progress(boss->current, NULL, false);
		if(p) {
			float a = clamp((global.frames - boss->current->starttime - 60) / 60.0, 0, 1);
			snprintf(buf, sizeof(buf), "%u / %u", p->num_cleared, p->num_played);

			Font *font = get_font("small");

			draw_boss_text(ALIGN_RIGHT,
				VIEWPORT_W + text_width(font, buf, 0) * pow(1 - a, 2),
				35 + text_height(font, buf, 0),
				buf, "small", RGBA(1, 1, 1, a)
			);
		}

		int maxhpspan = 0;
		int hpspan = 0;
		int nextspell, prevspell;

		// FIXME: this does not work when the boss has just one attack!

		for(nextspell = 0; nextspell < boss->acount - 1; nextspell++) {
			if(boss_should_skip_attack(boss,&boss->attacks[nextspell]))
				continue;
			int t = boss->attacks[nextspell].type;
			if(ATTACK_IS_SPELL(t) && !attack_is_over(&boss->attacks[nextspell]))
				break;
		}

		for(prevspell = nextspell; prevspell > 0; prevspell--) {
			if(boss_should_skip_attack(boss,&boss->attacks[prevspell]))
				continue;

			int t = boss->attacks[prevspell].type;
			if(ATTACK_IS_SPELL(t) && attack_is_over(&boss->attacks[prevspell]))
				break;
			maxhpspan += boss->attacks[prevspell].maxhp;
			hpspan += boss->attacks[prevspell].hp;
		}

		if(!maxhpspan) {
			return;
		}

		r_shader_standard_notex();
		r_mat_push();
		r_mat_translate(10,2,0);
		r_mat_scale((VIEWPORT_W-60)/(float)maxhpspan,1,1);

		// background shadow
		r_mat_push();
		r_color4(0,0,0,0.65);
		r_mat_scale(hpspan+2, 4, 1);
		r_mat_translate(0.5, 0.5, 0);
		r_draw_quad();
		r_mat_pop();

		// actual health bar
		for(int i = nextspell; i > prevspell; i--) {
			if(boss_should_skip_attack(boss,&boss->attacks[i]))
				continue;

			r_color(boss_healthbar_color(boss->attacks[i].type));

			r_mat_push();
			r_mat_scale(boss->attacks[i].hp, 2, 1);
			r_mat_translate(+0.5, 0.5, 0);
			r_draw_quad();
			r_mat_pop();
			r_mat_translate(boss->attacks[i].hp, 0, 0);
		}

		r_mat_pop();
		r_shader("sprite_default");

		// remaining spells
		r_color4(0.7, 0.7, 0.7, 0.7);
		Sprite *star = get_sprite("star");

		for(int x = 0, i = boss->acount-1; i > nextspell; i--) {
			if(
				ATTACK_IS_SPELL(boss->attacks[i].type) &&
				(boss->attacks[i].type != AT_ExtraSpell) &&
				!boss_should_skip_attack(boss, &boss->attacks[i])
			) {
				draw_sprite_batched_p(x += star->w * 1.1, 40, star);
			}
		}

		r_color4(1, 1, 1, 1);
	}
}

void boss_rule_extra(Boss *boss, float alpha) {
	if(global.frames % 5) {
		return;
	}

	int cnt = 10 * max(1, alpha);
	alpha = min(2, alpha);
	int lt = 1;

	if(alpha == 0) {
		lt += 2;
		alpha = 1 * frand();
	}

	for(int i = 0; i < cnt; ++i) {
		float a = i*2*M_PI/cnt + global.frames / 100.0;
		complex dir = cexp(I*(a+global.frames/50.0));
		complex vel = dir * 3;
		float v = max(0, alpha - 1);
		float psina = psin(a);

		PARTICLE(
			.sprite = (frand() < v*0.3 || lt > 1) ? "stain" : "arc",
			.pos = boss->pos + dir * (100 + 50 * psin(alpha*global.frames/10.0+2*i)) * alpha,
			.color = RGBA(
				1.0 - 0.5 * psina *    v,
				0.5 + 0.2 * psina * (1-v),
				0.5 + 0.5 * psina *    v,
				0.0
			),
			.rule = linear,
			.timeout = 30*lt,
			.draw_rule = GrowFade,
			.args = { vel * (1 - 2 * !(global.frames % 10)), 2.5 },
		);
	}
}

bool boss_is_dying(Boss *boss) {
	return boss->current && boss->current->endtime && boss->current->type != AT_Move && boss_attack_is_final(boss, boss->current);
}

bool boss_is_fleeing(Boss *boss) {
	return boss->current && boss->current->type == AT_Move && boss_attack_is_final(boss, boss->current);
}

bool boss_is_vulnerable(Boss *boss) {
	return boss->current && boss->current->type != AT_Move && boss->current->type != AT_SurvivalSpell && boss->current->starttime < global.frames && !boss->current->finished;
}

static DamageResult ent_damage_boss(EntityInterface *ent, const DamageInfo *dmg) {
	Boss *boss = ENT_CAST(ent, Boss);

	float factor = 1.0;
	if(dmg->type == DMG_PLAYER_SHOT)
		factor = boss->shot_damage_multiplier;
	if(dmg->type == DMG_PLAYER_BOMB)
		factor = boss->bomb_damage_multiplier;

	if(!boss_is_vulnerable(boss) || dmg->type == DMG_ENEMY_SHOT || dmg->type == DMG_ENEMY_COLLISION || factor == 0) {
		return DMG_RESULT_IMMUNE;
	}

	if(dmg->amount > 0 && global.frames-boss->lastdamageframe > 2) {
		boss->lastdamageframe = global.frames;
	}

	boss->current->hp -= dmg->amount*factor;

	if(boss->current->hp < boss->current->maxhp * 0.1) {
		play_loop("hit1");
	} else {
		play_loop("hit0");
	}

	return DMG_RESULT_OK;
}

static void boss_give_spell_bonus(Boss *boss, Attack *a, Player *plr) {
	bool fail = a->failtime, extra = a->type == AT_ExtraSpell;

	const char *title = extra ?
	                    (fail ? "Extra Spell failed..." : "Extra Spell cleared!"):
	                    (fail ?       "Spell failed..." :       "Spell cleared!");

	int time_left = max(0, a->starttime + a->timeout - global.frames);

	double sv = a->scorevalue;

	int clear_bonus = 0.5 * sv * !fail;
	int time_bonus = sv * (time_left / (double)a->timeout);
	int endurance_bonus = 0;
	int surv_bonus = 0;

	if(fail) {
		time_bonus /= 4;
		endurance_bonus = sv * 0.1 * (max(0, a->failtime - a->starttime) / (double)a->timeout);
	} else if(a->type == AT_SurvivalSpell) {
		surv_bonus = sv * (1.0 + 0.02 * (a->timeout / (double)FPS));
	}

	int total = time_bonus + surv_bonus + endurance_bonus + clear_bonus;
	float diff_bonus = 0.6 + 0.2 * global.diff;
	total *= diff_bonus;

	char diff_bonus_text[6];
	snprintf(diff_bonus_text, sizeof(diff_bonus_text), "x%.2f", diff_bonus);

	player_add_points(plr, total);

	StageTextTable tbl;
	stagetext_begin_table(&tbl, title, RGB(1, 1, 1), RGB(1, 1, 1), VIEWPORT_W/2, 0,
		ATTACK_END_DELAY_SPELL * 2, ATTACK_END_DELAY_SPELL / 2, ATTACK_END_DELAY_SPELL);
	stagetext_table_add_numeric_nonzero(&tbl, "Clear bonus", clear_bonus);
	stagetext_table_add_numeric_nonzero(&tbl, "Time bonus", time_bonus);
	stagetext_table_add_numeric_nonzero(&tbl, "Survival bonus", surv_bonus);
	stagetext_table_add_numeric_nonzero(&tbl, "Endurance bonus", endurance_bonus);
	stagetext_table_add_separator(&tbl);
	stagetext_table_add(&tbl, "Diff. multiplier", diff_bonus_text);
	stagetext_table_add_numeric(&tbl, "Total", total);
	stagetext_end_table(&tbl);

	play_sound("spellend");

	if(!fail) {
		play_sound("spellclear");
	}
}

static int attack_end_delay(Boss *boss) {
	if(boss_attack_is_final(boss, boss->current)) {
		return BOSS_DEATH_DELAY;
	}

	int delay = 0;

	switch(boss->current->type) {
		case AT_Spellcard:      delay = ATTACK_END_DELAY_SPELL; break;
		case AT_SurvivalSpell:  delay = ATTACK_END_DELAY_SURV;  break;
		case AT_ExtraSpell:     delay = ATTACK_END_DELAY_EXTRA; break;
		case AT_Move:           delay = ATTACK_END_DELAY_MOVE;  break;
		default:                delay = ATTACK_END_DELAY;       break;
	}

	if(delay) {
		Attack *next = boss_get_next_attack(boss);

		if(next && next->type == AT_ExtraSpell) {
			delay += ATTACK_END_DELAY_PRE_EXTRA;
		}
	}

	return delay;
}

void boss_finish_current_attack(Boss *boss) {
	AttackType t = boss->current->type;

	boss->current->hp = 0;
	boss->current->finished = true;
	boss->current->rule(boss, EVENT_DEATH);

	aniplayer_soft_switch(&boss->ani,"main",0);

	if(t != AT_Move) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
	}

	if(ATTACK_IS_SPELL(t)) {
		boss_give_spell_bonus(boss, boss->current, &global.plr);

		if(!boss->current->failtime) {
			StageProgress *p = get_spellstage_progress(boss->current, NULL, true);

			if(p) {
				++p->num_cleared;
			}
		} else if(boss->current->type != AT_ExtraSpell) {
			boss->failed_spells++;
		}

		if(t != AT_SurvivalSpell) {
			double i_base = 6.0, i_pwr = 1.0, i_pts = 1.0;

			if(t == AT_ExtraSpell) {
				i_pwr *= 1.25;
				i_pts *= 2.0;
			}

			if(!boss->current->failtime) {
				i_base *= 2.0;
			}

			spawn_items(boss->pos,
				Power, (int)(i_base * i_pwr),
				Point, (int)(i_base * i_pts),
			NULL);
		}
	}

	boss->current->endtime = global.frames + attack_end_delay(boss);
}

void process_boss(Boss **pboss) {
	Boss *boss = *pboss;

	if(!boss) {
		return;
	}

	aniplayer_update(&boss->ani);

	if(boss->global_rule) {
		boss->global_rule(boss, global.frames - boss->birthtime);
	}

	spawn_particle_effects(boss);

	if(!boss->current || global.dialog) {
		return;
	}

	if(boss->current->type == AT_Immediate) {
		boss->current->finished = true;
		boss->current->endtime = global.frames;
	}

	int time = global.frames - boss->current->starttime;
	bool extra = boss->current->type == AT_ExtraSpell;
	bool over = boss->current->finished && global.frames >= boss->current->endtime;

	if(!boss->current->endtime) {
		int remaining = boss->current->timeout - time;

		if(boss->current->type != AT_Move && remaining <= 11*FPS && remaining > 0 && !(time % FPS)) {
			play_sound(remaining <= 6*FPS ? "timeout2" : "timeout1");
		}

		boss->current->rule(boss, time);
	}

	if(extra) {
		float base = 0.2;
		float ampl = 0.2;
		float s = sin(time / 90.0 + M_PI*1.2);

		if(boss->current->endtime) {
			float p = (boss->current->endtime - global.frames)/(float)ATTACK_END_DELAY_EXTRA;
			float a = max((base + ampl * s) * p * 0.5, 5 * pow(1 - p, 3));
			if(a < 2) {
				global.shake_view = 3 * a;
				boss_rule_extra(boss, a);
				if(a > 1) {
					boss_rule_extra(boss, a * 0.5);
					if(a > 1.3) {
						global.shake_view = 5 * a;
						if(a > 1.7)
							global.shake_view += 2 * a;
						boss_rule_extra(boss, 0);
						boss_rule_extra(boss, 0.1);
					}
				}
			} else {
				global.shake_view_fade = 0.15;
			}
		} else if(time < 0) {
			boss_rule_extra(boss, 1+time/(float)ATTACK_START_DELAY_EXTRA);
		} else {
			float o = min(0, -5 + time/30.0);
			float q = (time <= 150? 1 - pow(time/250.0, 2) : min(1, time/60.0));

			boss_rule_extra(boss, max(1-time/300.0, base + ampl * s) * q);
			if(o) {
				boss_rule_extra(boss, max(1-time/300.0, base + ampl * s) - o);
				if(!global.shake_view) {
					global.shake_view = 5;
					global.shake_view_fade = 0.9;
				} else if(o > -0.05) {
					global.shake_view = 10;
					global.shake_view_fade = 0.5;
				}
			}
		}
	}

	bool timedout = time > boss->current->timeout;

	if((boss->current->type != AT_Move && boss->current->hp <= 0) || timedout) {
		if(!boss->current->endtime) {
			if(timedout && boss->current->type != AT_SurvivalSpell) {
				boss->current->failtime = global.frames;
			}

			boss_finish_current_attack(boss);
		} else if(boss->current->type != AT_Move || !boss_attack_is_final(boss, boss->current)) {
			// XXX: do we actually need to call this for AT_Move attacks at all?
			//      it should be harmless, but probably unnecessary.
			//      i'll be conservative and leave it in for now.
			stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
		}
	}

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		tsrand_fill(6);

		Color *clr = RGBA_MUL_ALPHA(0.1 + sin(10*t), 0.1 + cos(10*t), 0.5, t);
		clr->a = 0;

		PARTICLE(
			.sprite = "petal",
			.pos = boss->pos,
			.rule = asymptotic,
			.draw_rule = Petal,
			.color = clr,
			.args = {
				sign(anfrand(5))*(3+t*5*afrand(0))*cexp(I*M_PI*8*t),
				5+I,
				afrand(2) + afrand(3)*I,
				afrand(4) + 360.0*I*afrand(1)
			},
		);

		if(!extra) {
			if(t == 1) {
				global.shake_view_fade = 0.2;
			} else {
				global.shake_view = 5 * (t + t*t + t*t*t);
			}
		}

		if(t == 1) {
			for(int i = 0; i < 10; ++i) {
				spawn_boss_glow(boss, &boss->glowcolor, 60 + 20 * i);
			}

			for(int i = 0; i < 256; i++) {
				tsrand_fill(3);
				PARTICLE(
					.sprite = "flare",
					.pos = boss->pos,
					.timeout = 60 + 10 * afrand(2),
					.rule = linear,
					.draw_rule = Fade,
					.args = { (3+afrand(0)*10)*cexp(I*tsrand_a(1)) },
				);
			}

			PARTICLE("blast", boss->pos, 0, .timeout = 60, .args = { 0, 3.0 }, .draw_rule = GrowFade);
			PARTICLE("blast", boss->pos, 0, .timeout = 70, .args = { 0, 2.5 }, .draw_rule = GrowFade);
		}

		play_sound_ex("bossdeath", BOSS_DEATH_DELAY * 2, false);
	} else {
		if(cabs(boss->pos - global.plr.pos) < 16) {
			ent_damage(&global.plr.ent, &(DamageInfo) { .type = DMG_ENEMY_COLLISION });
		}
	}

	if(over) {
		if(global.stage->type == STAGE_SPELL && boss->current->type != AT_Move && boss->current->failtime) {
			stage_gameover();
		}

		log_debug("Current attack [%s] is over", boss->current->name);

		for(;;) {
			if(boss->current == boss->attacks + boss->acount - 1) {
				// no more attacks, die
				boss->current = NULL;
				boss_death(pboss);
				break;
			}

			boss->current++;
			assert(boss->current != NULL);

			if(boss->current->type == AT_Immediate) {
				boss->current->starttime = global.frames;
				boss->current->rule(boss, EVENT_BIRTH);

				if(global.dialog) {
					break;
				}

				continue;
			}

			if(boss_should_skip_attack(boss, boss->current)) {
				continue;
			}

			boss_start_attack(boss, boss->current);
			break;
		}
	}
}

void boss_death(Boss **boss) {
	Attack *a = boss_get_final_attack(*boss);
	bool fleed = false;

	if(!a) {
		// XXX: why does this happen?
		log_debug("FIXME: boss had no final attacK?");
	} else {
		fleed = a->type == AT_Move;
	}

	if((*boss)->acount && !fleed) {
		petal_explosion(35, (*boss)->pos);
	}

	if(!fleed) {
		stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
	}

	free_boss(*boss);
	*boss = NULL;
}

static void free_attack(Attack *a) {
	free(a->name);
}

void free_boss(Boss *boss) {
	del_ref(boss);
	ent_unregister(&boss->ent);

	for(int i = 0; i < boss->acount; i++)
		free_attack(&boss->attacks[i]);

	aniplayer_free(&boss->ani);
	free(boss->name);
	free(boss->attacks);
	free(boss);
}

void boss_start_attack(Boss *b, Attack *a) {
	log_debug("%s", a->name);

	StageInfo *i;
	StageProgress *p = get_spellstage_progress(a, &i, true);

	if(p) {
		++p->num_played;

		if(!p->unlocked) {
			log_info("Spellcard unlocked! %s: %s", i->title, i->subtitle);
			p->unlocked = true;
		}
	}

	// This should go before a->rule(b,EVENT_BIRTH), so it doesn’t reset values set by the attack rule.
	b->bomb_damage_multiplier = 1.0;
	b->shot_damage_multiplier = 1.0;

	a->starttime = global.frames + (a->type == AT_ExtraSpell? ATTACK_START_DELAY_EXTRA : ATTACK_START_DELAY);
	a->rule(b, EVENT_BIRTH);
	if(ATTACK_IS_SPELL(a->type)) {
		play_sound(a->type == AT_ExtraSpell ? "charge_extra" : "charge_generic");

		for(int i = 0; i < 10+5*(a->type == AT_ExtraSpell); i++) {
			tsrand_fill(4);

			PARTICLE(
				.sprite = "stain",
				.pos = VIEWPORT_W/2 + VIEWPORT_W/4*anfrand(0)+I*VIEWPORT_H/2+I*anfrand(1)*30,
				.color = RGBA(0.2, 0.3, 0.4, 0.0),
				.rule = linear,
				.timeout = 50,
				.draw_rule = GrowFade,
				.args = { sign(anfrand(2))*10*(1+afrand(3)) },
			);
		}

		// schedule a bomb cancellation for when the spell actually starts
		// we don't want an ongoing bomb to immediately ruin the spell bonus
		player_cancel_bomb(&global.plr, a->starttime - global.frames);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
}

Attack* boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule) {
	boss->attacks = realloc(boss->attacks, sizeof(Attack)*(++boss->acount));
	Attack *a = &boss->attacks[boss->acount-1];
	memset(a, 0, sizeof(Attack));

	boss->current = &boss->attacks[0];

	a->type = type;
	a->name = strdup(name);
	a->timeout = timeout * FPS;

	a->maxhp = hp;
	a->hp = hp;

	a->rule = rule;
	a->draw_rule = draw_rule;

	a->starttime = global.frames;

	// FIXME: figure out a better value/formula, i pulled this out of my ass
	a->scorevalue = 2000.0 + hp * 0.6;

	if(a->type == AT_ExtraSpell) {
		a->scorevalue *= 1.25;
	}

	return a;
}

void boss_generic_move(Boss *b, int time) {
	Attack *atck = b->current;

	if(atck->info->pos_dest == BOSS_NOMOVE) {
		return;
	}

	if(time == EVENT_DEATH) {
		b->pos = atck->info->pos_dest;
		return;
	}

	// because GO_TO is unreliable and linear is just not hip and cool enough

	float f = (time + ATTACK_START_DELAY) / ((float)atck->timeout + ATTACK_START_DELAY);
	float x = f;
	float a = 0.3;
	f = 1 - pow(f - 1, 4);
	f = f * (1 + a * pow(1 - x, 2));
	b->pos = atck->info->pos_dest * f + BOSS_DEFAULT_SPAWN_POS * (1 - f);
}

Attack* boss_add_attack_from_info(Boss *boss, AttackInfo *info, char move) {
	if(move) {
		boss_add_attack(boss, AT_Move, "Generic Move", 0.5, 0, boss_generic_move, NULL)->info = info;
	}

	Attack *a = boss_add_attack(boss, info->type, info->name, info->timeout, info->hp, info->rule, info->draw_rule);
	a->info = info;
	return a;
}

void boss_preload(void) {
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"charge_generic",
		"charge_extra",
		"spellend",
		"spellclear",
		"timeout1",
		"timeout2",
		"bossdeath",
	NULL);

	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"boss_indicator",
		"part/boss_shadow",
		"part/arc",
		"boss_spellcircle0",
		"boss_circle",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"boss_zoom",
		"spellcard_intro",
		"spellcard_outro",
		"spellcard_walloftext",
		"sprite_silhouette",
	NULL);

	StageInfo *s = global.stage;

	if(s->type != STAGE_SPELL || s->spell->type == AT_ExtraSpell) {
		preload_resources(RES_TEXTURE, RESF_DEFAULT,
			"stage3/wspellclouds",
			"stage4/kurumibg2",
		NULL);
	}
}
