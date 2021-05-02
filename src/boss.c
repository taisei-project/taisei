/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "boss.h"
#include "global.h"
#include "stage.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "entity.h"
#include "util/glm.h"
#include "portrait.h"
#include "stages/stage5/stage5.h"  // for unlockable bonus BGM
#include "stageobjects.h"

static void ent_draw_boss(EntityInterface *ent);
static DamageResult ent_damage_boss(EntityInterface *ent, const DamageInfo *dmg);

typedef struct SpellBonus {
	int clear;
	int time;
	int survival;
	int endurance;
	float diff_multiplier;
	int total;
	bool failed;
} SpellBonus;

static void calc_spell_bonus(Attack *a, SpellBonus *bonus);

DECLARE_TASK(boss_particles, { BoxedBoss boss; });

Boss *create_boss(char *name, char *ani, cmplx pos) {
	Boss *boss = objpool_acquire(stage_object_pools.bosses);

	boss->name = strdup(name);
	boss->pos = pos;

	char strbuf[strlen(ani) + sizeof("boss/")];
	snprintf(strbuf, sizeof(strbuf), "boss/%s", ani);
	aniplayer_create(&boss->ani, res_anim(strbuf), "main");

	boss->birthtime = global.frames;
	boss->zoomcolor = *RGBA(0.1, 0.2, 0.3, 1.0);

	boss->ent.draw_layer = LAYER_BOSS;
	boss->ent.draw_func = ent_draw_boss;
	boss->ent.damage_func = ent_damage_boss;
	ent_register(&boss->ent, ENT_TYPE_ID(Boss));

	// This is not necessary because the default will be set at the start of every attack.
	// But who knows. Maybe this will be triggered somewhen. If bosses without attacks start
	// taking over the world, I will be the one who put in this weak point to make them vulnerable.
	boss->bomb_damage_multiplier = 1.0;
	boss->shot_damage_multiplier = 1.0;

	COEVENT_INIT_ARRAY(boss->events);
	INVOKE_TASK(boss_particles, ENT_BOX(boss));

	return boss;
}

void boss_set_portrait(Boss *boss, const char *name, const char *variant, const char *face) {
	if(boss->portrait.tex != NULL) {
		r_texture_destroy(boss->portrait.tex);
		boss->portrait.tex = NULL;
	}

	if(name != NULL) {
		assume(face != NULL);
		portrait_render_byname(name, variant, face, &boss->portrait);
	} else {
		assume(face == NULL);
		assume(variant == NULL);
	}
}

static double draw_boss_text(Alignment align, float x, float y, const char *text, Font *fnt, const Color *clr) {
	return text_draw(text, &(TextParams) {
		.shader = "text_hud",
		.pos = { x, y },
		.color = clr,
		.font_ptr = fnt,
		.align = align,
	});
}

void draw_extraspell_bg(Boss *boss, int time) {
	// overlay for all extra spells
	// FIXME: Please replace this with something that doesn't look like shit.

	r_state_push();

	float opacity = 0.7;
	r_color4(0.2 * opacity, 0.1 * opacity, 0, 0);
	fill_viewport(sin(time) * 0.015, time / 50.0, 1, "stage3/wspellclouds");
	r_color4(2000, 2000, 2000, 0);
	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MIN,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_ADD
	));
	fill_viewport(cos(time) * 0.015, time / 70.0, 1, "stage4/kurumibg2");
	fill_viewport(sin(time*1.1+2.1) * 0.015, time / 30.0, 1, "stage4/kurumibg2");

	r_state_pop();
}

static inline bool healthbar_style_is_radial(void) {
	return config_get_int(CONFIG_HEALTHBAR_STYLE) > 0;
}

static const Color *boss_healthbar_color(AttackType atype) {
	static const Color colors[] = {
		[AT_Normal]        = { 0.50, 0.50, 0.60, 1.00 },
		[AT_Move]          = { 1.00, 1.00, 1.00, 1.00 },
		[AT_Spellcard]     = { 1.00, 0.00, 0.00, 1.00 },
		[AT_SurvivalSpell] = { 0.00, 1.00, 0.00, 1.00 },
		[AT_ExtraSpell]    = { 1.00, 0.40, 0.00, 1.00 },
		[AT_Immediate]     = { 1.00, 1.00, 1.00, 1.00 },
	};

	assert(atype >= 0 && atype < sizeof(colors)/sizeof(*colors));
	return colors + atype;
}

static StageProgress *get_spellstage_progress(Attack *a, StageInfo **out_stginfo, bool write) {
	if(!write || (global.replaymode == REPLAY_RECORD && global.stage->type == STAGE_STORY)) {
		StageInfo *i = stageinfo_get_by_spellcard(a->info, global.diff);
		if(i) {
			StageProgress *p = stageinfo_get_progress(i, global.diff, write);

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
	if(a->type == AT_ExtraSpell || (a->info != NULL && a->info->type == AT_ExtraSpell)) {
		if(global.plr.voltage < global.voltage_threshold) {
			return true;
		}
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

attr_unused
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

static void update_healthbar(Boss *boss) {
	bool radial = healthbar_style_is_radial();
	bool player_nearby = radial && cabs(boss->pos - global.plr.pos) < 128;
	float update_speed = 0.1;
	float target_opacity = 1.0;
	float target_fill = 0.0;
	float target_altfill = 0.0;

	if(
		boss_is_dying(boss) ||
		boss_is_fleeing(boss) ||
		!boss->current ||
		boss->current->type == AT_Move ||
		dialog_is_active(global.dialog)
	) {
		target_opacity = 0.0;
	}

	if(player_nearby) {
		target_opacity *= 0.25;
	}

	Attack *a_prev = NULL;
	Attack *a_cur = NULL;
	Attack *a_next = NULL;

	for(uint i = 0; i < boss->acount; ++i) {
		Attack *a = boss->attacks + i;

		if(boss_should_skip_attack(boss, a) || a->type == AT_Move) {
			continue;
		}

		if(a_cur != NULL) {
			a_next = a;
			break;
		}

		if(a == boss->current) {
			a_cur = boss->current;
			continue;
		}

		if(attack_is_over(a)) {
			a_prev = a;
		}
	}

	/*
	log_debug("prev = %s", a_prev ? a_prev->name : NULL);
	log_debug("cur = %s", a_cur ? a_cur->name : NULL);
	log_debug("next = %s", a_next ? a_next->name : NULL);
	*/

	if(a_cur != NULL) {
		float total_maxhp = 0, total_hp = 0;

		Attack *spell, *non;

		if(a_cur->type == AT_Normal) {
			non = a_cur;
			spell = a_next;
		} else {
			non = a_prev;
			spell = a_cur;
		}

		if(non && non->type == AT_Normal && non->maxhp > 0) {
			total_hp += 3 * non->hp;
			total_maxhp += 3 * non->maxhp;
			boss->healthbar.fill_color = *boss_healthbar_color(non->type);
		}

		if(spell && ATTACK_IS_SPELL(spell->type)) {
			float spell_hp;
			float spell_maxhp;

			if(spell->type == AT_SurvivalSpell) {
				spell_hp = spell_maxhp = fmax(1, total_maxhp * 0.1);
			} else {
				spell_hp = spell->hp;
				spell_maxhp = spell->maxhp;
			}

			total_hp += spell_hp;
			total_maxhp += spell_maxhp;
			target_altfill = spell_maxhp / total_maxhp;
			boss->healthbar.fill_altcolor = *boss_healthbar_color(spell->type);
		}

		if(a_cur->type == AT_SurvivalSpell) {
			target_altfill = 1;
			target_fill = 0;

			if(radial) {
				target_opacity = 0.0;
			}
		} else if(total_maxhp > 0) {
			total_maxhp = fmax(0.001, total_maxhp);

			if(total_hp > 0) {
				total_hp = fmax(0.001, total_hp);
			}

			target_fill = total_hp / total_maxhp;
		}
	}

	float opacity_update_speed = update_speed;

	if(boss_is_dying(boss)) {
		opacity_update_speed *= 0.25;
	}

	fapproach_asymptotic_p(&boss->healthbar.opacity, target_opacity, opacity_update_speed, 1e-3);

	if(boss_is_vulnerable(boss) || (a_cur && a_cur->type == AT_SurvivalSpell && global.frames - a_cur->starttime > 0)) {
		update_speed *= (1 + 4 * pow(1 - target_fill, 2));
	}

	fapproach_asymptotic_p(&boss->healthbar.fill_total, target_fill, update_speed, 1e-3);
	fapproach_asymptotic_p(&boss->healthbar.fill_alt, target_altfill, update_speed, 1e-3);
}

static void update_hud(Boss *boss) {
	update_healthbar(boss);

	float update_speed = 0.1;
	float target_opacity = 1.0;
	float target_spell_opacity = 1.0;
	float target_plrproximity_opacity = 1.0;

	if(boss_is_dying(boss) || boss_is_fleeing(boss)) {
		update_speed *= 0.25;
		target_opacity = 0.0;
	}

	if(!boss->current || boss->current->type == AT_Move || dialog_is_active(global.dialog)) {
		target_opacity = 0.0;
	}

	if(!boss->current || !ATTACK_IS_SPELL(boss->current->type) || boss->current->finished) {
		target_spell_opacity = 0.0;
	}

	if(cimag(global.plr.pos) < 128) {
		target_plrproximity_opacity = 0.25;
	}

	if(boss->current && !boss->current->finished && boss->current->type != AT_Move) {
		int frames = boss->current->timeout + imin(0, boss->current->starttime - global.frames);
		boss->hud.attack_timer = clamp((frames)/(double)FPS, 0, 99.999);
	}

	fapproach_asymptotic_p(&boss->hud.global_opacity, target_opacity, update_speed, 1e-2);
	fapproach_asymptotic_p(&boss->hud.spell_opacity, target_spell_opacity, update_speed, 1e-2);
	fapproach_asymptotic_p(&boss->hud.plrproximity_opacity, target_plrproximity_opacity, update_speed, 1e-2);
}

static void draw_radial_healthbar(Boss *boss) {
	if(boss->healthbar.opacity == 0) {
		return;
	}

	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(creal(boss->pos), cimag(boss->pos), 0);
	r_mat_mv_scale(220, 220, 0);
	r_shader("healthbar_radial");
	r_uniform_vec4_rgba("borderColor",   RGBA(0.75, 0.75, 0.75, 0.75));
	r_uniform_vec4_rgba("glowColor",     RGBA(0.5, 0.5, 1.0, 0.75));
	r_uniform_vec4_rgba("fillColor",     &boss->healthbar.fill_color);
	r_uniform_vec4_rgba("altFillColor",  &boss->healthbar.fill_altcolor);
	r_uniform_vec4_rgba("coreFillColor", RGBA(0.8, 0.8, 0.8, 0.5));
	r_uniform_vec2("fill", boss->healthbar.fill_total, boss->healthbar.fill_alt);
	r_uniform_float("opacity", boss->healthbar.opacity);
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();
}

static void draw_linear_healthbar(Boss *boss) {
	float opacity = boss->healthbar.opacity * boss->hud.global_opacity * boss->hud.plrproximity_opacity;

	if(opacity == 0) {
		return;
	}

	const float width = VIEWPORT_W * 0.9;
	const float height = 24;

	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(1 + width/2, height/2 - 3, 0);
	r_mat_mv_scale(width, height, 0);
	r_shader("healthbar_linear");
	r_uniform_vec4_rgba("borderColor",   RGBA(0.75, 0.75, 0.75, 0.75));
	r_uniform_vec4_rgba("glowColor",     RGBA(0.5, 0.5, 1.0, 0.75));
	r_uniform_vec4_rgba("fillColor",     &boss->healthbar.fill_color);
	r_uniform_vec4_rgba("altFillColor",  &boss->healthbar.fill_altcolor);
	r_uniform_vec4_rgba("coreFillColor", RGBA(0.8, 0.8, 0.8, 0.5));
	r_uniform_vec2("fill", boss->healthbar.fill_total, boss->healthbar.fill_alt);
	r_uniform_float("opacity", opacity);
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();
}

static void draw_spell_warning(Font *font, float y_pos, float f, float opacity) {
	static const char *msg = "~ Spell Card Attack! ~";

	float msg_width = text_width(font, msg, 0);
	float flash = 0.2 + 0.8 * pow(psin(M_PI + 5 * M_PI * f), 0.5);
	f = 0.15 * f + 0.85 * (0.5 * pow(2 * f - 1, 3) + 0.5);
	opacity *= 1 - 2 * fabs(f - 0.5);
	cmplx pos = (VIEWPORT_W + msg_width) * f - msg_width * 0.5 + I * y_pos;

	draw_boss_text(ALIGN_CENTER, creal(pos), cimag(pos), msg, font, color_mul_scalar(RGBA(1, flash, flash, 1), opacity));
}

static void draw_spell_name(Boss *b, int time, bool healthbar_radial) {
	Font *font = res_font("standard");

	cmplx x0 = VIEWPORT_W/2+I*VIEWPORT_H/3.5;
	float f = clamp((time - 40.0) / 60.0, 0, 1);
	float f2 = clamp(time / 80.0, 0, 1);
	float y_offset = 26 + healthbar_radial * -15;
	float y_text_offset = 5 - font_get_metrics(font)->descent;
	cmplx x = x0 + ((VIEWPORT_W - 10) + I*(y_offset + y_text_offset) - x0) * f*(f+1)*0.5;
	int strw = text_width(font, b->current->name, 0);

	float opacity_noplr = b->hud.spell_opacity * b->hud.global_opacity;
	float opacity = opacity_noplr * b->hud.plrproximity_opacity;

	r_draw_sprite(&(SpriteParams) {
		.sprite = "spell",
		.pos = { (VIEWPORT_W - 128), y_offset * (1 - pow(1 - f2, 5)) + VIEWPORT_H * pow(1 - f2, 2) },
		.color = color_mul_scalar(RGBA(1, 1, 1, f2 * 0.5), opacity * f2) ,
		.scale.both = 3 - 2 * (1 - pow(1 - f2, 3)),
	});

	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);
	r_disable(RCAP_CULL_FACE);

	int delay = b->current->type == AT_ExtraSpell ? ATTACK_START_DELAY_EXTRA : ATTACK_START_DELAY;
	float warn_progress = clamp((time + delay) / 120.0, 0, 1);

	r_mat_mv_push();
	r_mat_mv_translate(creal(x), cimag(x),0);
	float scale = f+1.*(1-f)*(1-f)*(1-f);
	r_mat_mv_scale(scale,scale,1);
	r_mat_mv_rotate(glm_ease_quad_out(f) * 2 * M_PI, 0.8, -0.2, 0);

	float spellname_opacity_noplr = opacity_noplr * fmin(1, warn_progress/0.6);
	float spellname_opacity = spellname_opacity_noplr * b->hud.plrproximity_opacity;

	draw_boss_text(ALIGN_RIGHT, strw/2*(1-f), 0, b->current->name, font, color_mul_scalar(RGBA(1, 1, 1, 1), spellname_opacity));

	if(spellname_opacity_noplr < 1) {
		r_mat_mv_push();
		r_mat_mv_scale(2 - spellname_opacity_noplr, 2 - spellname_opacity_noplr, 1);
		draw_boss_text(ALIGN_RIGHT, strw/2*(1-f), 0, b->current->name, font, color_mul_scalar(RGBA(1, 1, 1, 1), spellname_opacity_noplr * 0.5));
		r_mat_mv_pop();
	}

	r_mat_mv_pop();

	r_capability(RCAP_CULL_FACE, cullcap_saved);

	if(warn_progress < 1) {
		draw_spell_warning(font, cimag(x0) - font_get_lineskip(font), warn_progress, opacity);
	}

	StageProgress *p = get_spellstage_progress(b->current, NULL, false);

	if(p) {
		char buf[32];
		float a = clamp((global.frames - b->current->starttime - 60) / 60.0, 0, 1);
		font = res_font("small");

		float bonus_ofs = 220;

		r_mat_mv_push();
		r_mat_mv_translate(
			bonus_ofs * pow(1 - a, 2),
			font_get_lineskip(font) + y_offset + y_text_offset + 0.5,
		0);

		bool kern = font_get_kerning_enabled(font);
		font_set_kerning_enabled(font, false);

		snprintf(buf, sizeof(buf), "%u / %u", p->num_cleared, p->num_played);

		draw_boss_text(ALIGN_RIGHT,
			VIEWPORT_W - 10 - text_width(font, buf, 0), 0,
			"History: ", font, color_mul_scalar(RGB(0.75, 0.75, 0.75), a * opacity)
		);

		draw_boss_text(ALIGN_RIGHT,
			VIEWPORT_W - 10, 0,
			buf, font, color_mul_scalar(RGB(0.50, 1.00, 0.50), a * opacity)
		);

		// r_mat_translate(0, 6.5, 0);

		bonus_ofs -= draw_boss_text(ALIGN_LEFT,
			VIEWPORT_W - bonus_ofs, 0,
			"Bonus: ", font, color_mul_scalar(RGB(0.75, 0.75, 0.75), a * opacity)
		);

		SpellBonus bonus;
		calc_spell_bonus(b->current, &bonus);
		format_huge_num(0, bonus.total, sizeof(buf), buf);

		draw_boss_text(ALIGN_LEFT,
			VIEWPORT_W - bonus_ofs, 0,
			buf, font, color_mul_scalar(
				bonus.failed ? RGB(1.00, 0.50, 0.50)
				             : RGB(0.30, 0.60, 1.00)
			, a * opacity)
		);

		font_set_kerning_enabled(font, kern);

		r_mat_mv_pop();
	}
}

static void draw_spell_portrait(Boss *b, int time) {
	const int anim_time = 200;

	if(time <= 0 || time >= anim_time || !b->portrait.tex) {
		return;
	}

	if(b->current->draw_rule == NULL) {
		// No spell background? Assume not cut-in is intended, either.
		return;
	}

	// NOTE: Mostly copypasted from player.c::player_draw_overlay()
	// TODO: Maybe somehow generalize and make it more intelligible

	float a = time / (float)anim_time;

	r_state_push();
	r_shader("sprite_default");

	float char_in = clamp(a * 1.5, 0, 1);
	float char_out = fmin(1, 2 - (2 * a));
	float char_opacity_in = 0.75 * fmin(1, a * 5);
	float char_opacity = char_opacity_in * char_out * char_out;
	float char_xofs = -20 * a;

	Sprite *char_spr = &b->portrait;

	r_mat_mv_push();
	r_mat_mv_scale(-1, 1, 1);
	r_mat_mv_translate(-VIEWPORT_W, 0, 0);
	r_cull(CULL_FRONT);

	for(int i = 1; i <= 3; ++i) {
		float t = a * 200;
		float dur = 20;
		float start = 200 - dur * 5;
		float end = start + dur;
		float ofs = 0.2 * dur * (i - 1);
		float o = 1 - smoothstep(start + ofs, end + ofs, t);

		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = char_spr,
			.pos = { char_spr->w * 0.5 + VIEWPORT_W * pow(1 - char_in, 4 - i * 0.3) - i + char_xofs, VIEWPORT_H - char_spr->h * 0.5 },
			.color = color_mul_scalar(color_add(RGBA(0.2, 0.2, 0.2, 0), RGBA(i==1, i==2, i==3, 0)), char_opacity_in * (1 - char_in * o) * o),
			.flip.x = true,
			.scale.both = 1.0 + 0.02 * (fmin(1, a * 1.2)) + i * 0.5 * pow(1 - o, 2),
		});
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = char_spr,
		.pos = { char_spr->w * 0.5 + VIEWPORT_W * pow(1 - char_in, 4) + char_xofs, VIEWPORT_H - char_spr->h * 0.5 },
		.color = RGBA_MUL_ALPHA(1, 1, 1, char_opacity * fmin(1, char_in * 2) * (1 - fmin(1, (1 - char_out) * 5))),
		.flip.x = true,
		.scale.both = 1.0 + 0.1 * (1 - char_out),
	});

	r_mat_mv_pop();
	r_state_pop();
}

static void boss_glow_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
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

static Projectile *spawn_boss_glow(Boss *boss, const Color *clr, int timeout) {
	return PARTICLE(
		.sprite_ptr = aniplayer_get_frame(&boss->ani),
		// this is in sync with the boss position oscillation
		.pos = boss->pos + 6 * sin(global.frames/25.0) * I,
		.color = clr,
		.draw_rule = boss_glow_draw,
		.timeout = timeout,
		.layer = LAYER_PARTICLE_LOW,
		.shader = "sprite_silhouette",
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
	);
}

DEFINE_TASK(boss_particles) {
	Boss *boss = TASK_BIND(ARGS.boss);
	DECLARE_ENT_ARRAY(Projectile, smoke_parts, 16);

	cmplx prev_pos = boss->pos;

	for(;;YIELD) {
		ENT_ARRAY_FOREACH(&smoke_parts, Projectile *p, {
			p->pos += boss->pos - prev_pos;
		});
		prev_pos = boss->pos;

		Color *glowcolor = &boss->glowcolor;
		Color *shadowcolor = &boss->shadowcolor;

		Attack *cur = boss->current;
		bool is_spell = cur && ATTACK_IS_SPELL(cur->type) && !cur->endtime;
		bool is_extra = cur && cur->type == AT_ExtraSpell && global.frames >= cur->starttime;

		if(!(global.frames % 13) && !is_extra) {
			ENT_ARRAY_COMPACT(&smoke_parts);
			ENT_ARRAY_ADD(&smoke_parts, PARTICLE(
				.sprite = "smoke",
				.pos = cdir(global.frames) + boss->pos,
				.color = RGBA(shadowcolor->r, shadowcolor->g, shadowcolor->b, 0.0),
				.timeout = 180,
				.draw_rule = pdraw_timeout_scale(2, 0.01),
				.angle = rng_angle(),
			));
		}

		if(!(global.frames % (2 + 2 * is_extra)) && (is_spell || boss_is_dying(boss))) {
			float glowstr = 0.5;
			float a = (1.0 - glowstr) + glowstr * psin(global.frames/15.0);
			spawn_boss_glow(boss, color_mul_scalar(COLOR_COPY(glowcolor), a), 24);
		}
	}
}

void draw_boss_background(Boss *boss) {
	r_mat_mv_push();
	r_mat_mv_translate(creal(boss->pos), cimag(boss->pos), 0);
	r_mat_mv_rotate(global.frames * 4.0 * DEG2RAD, 0, 0, -1);

	float f = 0.8+0.1*sin(global.frames/8.0);

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		f -= t*(t-0.7)/fmax(0.01, 1-t);
	}

	r_mat_mv_scale(f, f, 1);
	r_draw_sprite(&(SpriteParams) {
		.sprite = "boss_circle",
		.color = RGBA(1, 1, 1, 0),
	});
	r_mat_mv_pop();
}

static void ent_draw_boss(EntityInterface *ent) {
	Boss *boss = ENT_CAST(ent, Boss);

	float red = 0.5*exp(-0.5*(global.frames-boss->lastdamageframe));
	if(red > 1)
		red = 0;

	float boss_alpha = 1;

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		boss_alpha = (1 - t) + 0.3;
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&boss->ani),
		.pos = { creal(boss->pos), cimag(boss->pos) + 6*sin(global.frames/25.0) },
		.color = RGBA_MUL_ALPHA(1, 1-red, 1-red/2, boss_alpha),
	});
}

void draw_boss_fake_overlay(Boss *boss) {
	if(healthbar_style_is_radial()) {
		draw_radial_healthbar(boss);
	}
}

void draw_boss_overlay(Boss *boss) {
	bool radial_style = healthbar_style_is_radial();

	if(!radial_style) {
		draw_linear_healthbar(boss);
	}

	if(!boss->current) {
		return;
	}

	if(boss->current->type == AT_Move && global.frames - boss->current->starttime > 0 && boss_attack_is_final(boss, boss->current)) {
		return;
	}

	float o = boss->hud.global_opacity * boss->hud.plrproximity_opacity;

	if(o > 0) {
		draw_boss_text(ALIGN_LEFT, 10, 20 + 8 * !radial_style, boss->name, res_font("standard"), RGBA(o, o, o, o));

		if(ATTACK_IS_SPELL(boss->current->type)) {
			int t_portrait, t_spell;
			t_portrait = t_spell = global.frames - boss->current->starttime;

			if(boss->current->type == AT_ExtraSpell) {
				t_portrait += ATTACK_START_DELAY_EXTRA;
			} else {
				t_portrait += ATTACK_START_DELAY;
			}

			draw_spell_portrait(boss, t_portrait);
			draw_spell_name(boss, t_spell, radial_style);
		}

		float remaining = boss->hud.attack_timer;
		Color clr_int, clr_fract;

		if(remaining < 6) {
			clr_int = *RGB(1.0, 0.2, 0.2);
		} else if(remaining < 11) {
			clr_int = *RGB(1.0, 1.0, 0.2);
		} else {
			clr_int = *RGB(1.0, 1.0, 1.0);
		}

		color_mul_scalar(&clr_int, o);
		clr_fract = *RGBA(clr_int.r * 0.5, clr_int.g * 0.5, clr_int.b * 0.5, clr_int.a);

		Font *f_int = res_font("standard");
		Font *f_fract = res_font("small");
		double pos_x, pos_y;

		Alignment align;

		if(radial_style) {
			// pos_x = (VIEWPORT_W - w_total) * 0.5;
			pos_x = VIEWPORT_W / 2;
			pos_y = 64;
			align = ALIGN_CENTER;
		} else {
			// pos_x = VIEWPORT_W - w_total - 10;
			pos_x = VIEWPORT_W - 10;
			pos_y = 12;
			align = ALIGN_RIGHT;
		}

		r_shader("text_hud");
		draw_fraction(remaining, align, pos_x, pos_y, f_int, f_fract, &clr_int, &clr_fract, true);
		r_shader("sprite_default");

		// remaining spells
		Color *clr = RGBA(0.7 * o, 0.7 * o, 0.7 * o, 0.7 * o);
		Sprite *star = res_sprite("star");
		float x = 10 + star->w * 0.5;
		bool spell_found = false;

		for(Attack *a = boss->current; a && a < boss->attacks + boss->acount; ++a) {
			if(
				ATTACK_IS_SPELL(a->type) &&
				(a->type != AT_ExtraSpell) &&
				!boss_should_skip_attack(boss, a)
			) {
				// I guess we can just always skip the first one
				if(spell_found) {
					r_draw_sprite(&(SpriteParams) {
						.sprite_ptr = star,
						.pos = { x, 40 + 8 * !radial_style },
						.color = clr,
					});
					x += star->w * 1.1;
				} else {
					spell_found = true;
				}
			}
		}

		r_color4(1, 1, 1, 1);
	}
}

static void boss_rule_extra(Boss *boss, float alpha) {
	if(global.frames % 5) {
		return;
	}

	int cnt = 5 * fmax(1, alpha);
	alpha = fmin(2, alpha);
	int lt = 1;

	if(alpha == 0) {
		lt += 2;
		alpha = rng_real();
	}

	for(int i = 0; i < cnt; ++i) {
		float a = i*2*M_PI/cnt + global.frames / 100.0;
		cmplx dir = cexp(I*(a+global.frames/50.0));
		cmplx vel = dir * 3;
		float v = fmax(0, alpha - 1);
		float psina = psin(a);

		PARTICLE(
			.sprite = (rng_chance(v * 0.3) || lt > 1) ? "stain" : "arc",
			.pos = boss->pos + dir * (100 + 50 * psin(alpha*global.frames/10.0+2*i)) * alpha,
			.color = color_mul_scalar(RGBA(
				1.0 - 0.5 * psina *    v,
				0.5 + 0.2 * psina * (1-v),
				0.5 + 0.5 * psina *    v,
				0.0
			), 0.8),
			.timeout = 30*lt,
			.draw_rule = pdraw_timeout_scalefade(0, 3.5, 1, 0),
			.move = move_linear(vel * (1 - 2 * !(global.frames % 10))),
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
	return boss->current && boss->current->type != AT_Move && boss->current->type != AT_SurvivalSpell && !boss->current->finished;
}

bool boss_is_player_collision_active(Boss *boss) {
	return boss->current && !boss_is_dying(boss) && !boss_is_fleeing(boss);
}

static DamageResult ent_damage_boss(EntityInterface *ent, const DamageInfo *dmg) {
	Boss *boss = ENT_CAST(ent, Boss);

	if(
		!boss_is_vulnerable(boss) ||
		dmg->type == DMG_ENEMY_SHOT ||
		dmg->type == DMG_ENEMY_COLLISION
	) {
		return DMG_RESULT_IMMUNE;
	}

	float factor;

	if(dmg->type == DMG_PLAYER_SHOT || dmg->type == DMG_PLAYER_DISCHARGE) {
		factor = boss->shot_damage_multiplier;
	} else if(dmg->type == DMG_PLAYER_BOMB) {
		factor = boss->bomb_damage_multiplier;
	} else {
		factor = 1.0f;
	}

	int min_damage_time = 60;
	int max_damage_time = 300;
	int pattern_time = global.frames - NOT_NULL(boss->current)->starttime;

	if(pattern_time < max_damage_time) {
		float span = max_damage_time - min_damage_time;
		factor = clampf((pattern_time - min_damage_time) / span, 0.0f, 1.0f);
	}

	if(factor == 0) {
		return DMG_RESULT_IMMUNE;
	}

	if(dmg->amount > 0 && global.frames-boss->lastdamageframe > 2) {
		boss->lastdamageframe = global.frames;
	}

	boss->current->hp -= dmg->amount * factor;

	if(boss->current->hp < boss->current->maxhp * 0.1) {
		play_sfx_loop("hit1");
	} else {
		play_sfx_loop("hit0");
	}

	return DMG_RESULT_OK;
}

static void calc_spell_bonus(Attack *a, SpellBonus *bonus) {
	bool survival = a->type == AT_SurvivalSpell;
	bonus->failed = a->failtime > 0;

	int time_left = fmax(0, a->starttime + a->timeout - global.frames);

	double piv_factor = global.plr.point_item_value / (double)PLR_START_PIV;
	double base = a->bonus_base * 0.5 * (1 + piv_factor);

	bonus->clear = bonus->failed ? 0 : base;
	bonus->time = survival ? 0 : 0.5 * base * (time_left / (double)a->timeout);
	bonus->endurance = 0;
	bonus->survival = 0;

	if(bonus->failed) {
		bonus->time /= 4;
		bonus->endurance = base * 0.1 * (fmax(0, a->failtime - a->starttime) / (double)a->timeout);
	} else if(survival) {
		bonus->survival = base * (1.0 + 0.02 * (a->timeout / (double)FPS));
	}

	bonus->total = bonus->clear + bonus->time + bonus->endurance + bonus->survival;
	bonus->diff_multiplier = 0.6 + 0.2 * global.diff;
	bonus->total *= bonus->diff_multiplier;
}

static void boss_give_spell_bonus(Boss *boss, Attack *a, Player *plr) {
	SpellBonus bonus = { 0 };
	calc_spell_bonus(a, &bonus);

	const char *title = bonus.failed ? "Spell Card failed..."  : "Spell Card captured!";

	char diff_bonus_text[6];
	snprintf(diff_bonus_text, sizeof(diff_bonus_text), "x%.2f", bonus.diff_multiplier);

	player_add_points(plr, bonus.total, plr->pos);

	StageTextTable tbl;
	stagetext_begin_table(&tbl, title, RGB(1, 1, 1), RGB(1, 1, 1), VIEWPORT_W/2, 0,
		ATTACK_END_DELAY_SPELL * 2, ATTACK_END_DELAY_SPELL / 2, ATTACK_END_DELAY_SPELL);
	stagetext_table_add_numeric_nonzero(&tbl, "Clear bonus", bonus.clear);
	stagetext_table_add_numeric_nonzero(&tbl, "Time bonus", bonus.time);
	stagetext_table_add_numeric_nonzero(&tbl, "Survival bonus", bonus.survival);
	stagetext_table_add_numeric_nonzero(&tbl, "Endurance bonus", bonus.endurance);
	stagetext_table_add_separator(&tbl);
	stagetext_table_add(&tbl, "Diff. multiplier", diff_bonus_text);
	stagetext_table_add_numeric(&tbl, "Total", bonus.total);
	stagetext_end_table(&tbl);

	play_sfx("spellend");

	if(!bonus.failed) {
		play_sfx("spellclear");
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

	return delay;
}

static void boss_call_rule(Boss *boss, int t) {
	if(boss->current->rule) {
		boss->current->rule(boss, t);
	}
}

void boss_finish_current_attack(Boss *boss) {
	AttackType t = boss->current->type;

	boss->current->hp = 0;
	boss->current->finished = true;
	boss_call_rule(boss, EVENT_DEATH);

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

			// HACK
			if(boss->current->info == &stage5_spells.extra.overload) {
				stage_unlock_bgm("bonus0");
			}
		} else if(boss->current->type != AT_ExtraSpell) {
			boss->failed_spells++;
		}

		spawn_items(boss->pos,
			ITEM_POWER,  14,
			ITEM_POINTS, 12,
			ITEM_BOMB_FRAGMENT, (boss->current->failtime ? 0 : 1)
		);
	}

	boss->current->endtime = global.frames + attack_end_delay(boss);
	boss->current->endtime_undelayed = global.frames;

	coevent_signal_once(&boss->current->events.finished);
}

void process_boss(Boss **pboss) {
	Boss *boss = *pboss;

	if(!boss) {
		return;
	}

	move_update(&boss->pos, &boss->move);
	aniplayer_update(&boss->ani);
	update_hud(boss);

	if(boss->global_rule) {
		boss->global_rule(boss, global.frames - boss->birthtime);
	}

	if(!boss->current || dialog_is_active(global.dialog)) {
		return;
	}

	if(boss->current->type == AT_Immediate) {
		boss->current->finished = true;
		boss->current->endtime = global.frames;
		boss->current->endtime_undelayed = global.frames;
	}

	int time = global.frames - boss->current->starttime;
	bool extra = boss->current->type == AT_ExtraSpell;
	bool over = boss->current->finished && global.frames >= boss->current->endtime;

	if(time == 0) {
		coevent_signal_once(&boss->current->events.started);
	}

	if(!boss->current->endtime) {
		int remaining = boss->current->timeout - time;

		if(boss->current->type != AT_Move && remaining <= 11*FPS && remaining > 0 && !(time % FPS)) {
			play_sfx(remaining <= 6*FPS ? "timeout2" : "timeout1");
		}

		boss_call_rule(boss, time);
	}

	if(extra) {
		float base = 0.2;
		float ampl = 0.2;
		float s = sin(time / 90.0 + M_PI*1.2);

		if(boss->current->endtime) {
			float p = (boss->current->endtime - global.frames)/(float)ATTACK_END_DELAY_EXTRA;
			float a = fmax((base + ampl * s) * p * 0.5, 5 * pow(1 - p, 3));
			if(a < 2) {
				stage_shake_view(3 * a);
				boss_rule_extra(boss, a);
				if(a > 1) {
					boss_rule_extra(boss, a * 0.5);
					if(a > 1.3) {
						stage_shake_view(5 * a);
						if(a > 1.7)
							stage_shake_view(2 * a);
						boss_rule_extra(boss, 0);
						boss_rule_extra(boss, 0.1);
					}
				}
			}
		} else if(time < 0) {
			boss_rule_extra(boss, 1+time/(float)ATTACK_START_DELAY_EXTRA);
		} else {
			float o = fmin(0, -5 + time/30.0);
			float q = (time <= 150? 1 - pow(time/250.0, 2) : fmin(1, time/60.0));

			boss_rule_extra(boss, fmax(1-time/300.0, base + ampl * s) * q);
			if(o) {
				boss_rule_extra(boss, fmax(1-time/300.0, base + ampl * s) - o);

				stage_shake_view(5);
				if(o > -0.05) {
					stage_shake_view(10);
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

	bool dying = boss_is_dying(boss);
	bool fleeing = boss_is_fleeing(boss);

	if(dying || fleeing) {
		coevent_signal_once(&boss->events.defeated);
	}

	if(dying) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		RNG_ARRAY(rng, 2);

		Color *clr = RGBA_MUL_ALPHA(0.1 + sin(10*t), 0.1 + cos(10*t), 0.5, t);
		clr->a = 0;

		PARTICLE(
			.sprite = "petal",
			.pos = boss->pos,
			.draw_rule = pdraw_petal_random(),
			.color = clr,
			.move = move_asymptotic_simple(
				vrng_sign(rng[0]) * (3 + t * 5 * vrng_real(rng[1])) * cdir(M_PI*8*t),
				5
			),
			.layer = LAYER_PARTICLE_PETAL,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		if(!extra) {
			stage_shake_view(5 * (t + t*t + t*t*t));
		}

		if(t == 1) {
			for(int i = 0; i < 10; ++i) {
				spawn_boss_glow(boss, &boss->glowcolor, 60 + 20 * i);
			}

			for(int i = 0; i < 256; i++) {
				RNG_ARRAY(rng, 3);
				PARTICLE(
					.sprite = "flare",
					.pos = boss->pos,
					.timeout = vrng_range(rng[2], 60, 70),
					.draw_rule = pdraw_timeout_fade(1, 0),
					.move = move_linear(vrng_range(rng[0], 3, 13) * vrng_dir(rng[1])),
				);
			}

			PARTICLE(
				.proto = pp_blast,
				.pos = boss->pos,
				.timeout = 60,
				.draw_rule = pdraw_timeout_scalefade(0, 4, 1, 0),
			);

			PARTICLE(
				.proto = pp_blast,
				.pos = boss->pos,
				.timeout = 70,
				.draw_rule = pdraw_timeout_scalefade(0, 3.5, 1, 0),
			);
		}

		play_sfx_ex("bossdeath", BOSS_DEATH_DELAY * 2, false);
	}

	if(boss_is_player_collision_active(boss) && cabs(boss->pos - global.plr.pos) < BOSS_HURT_RADIUS) {
		ent_damage(&global.plr.ent, &(DamageInfo) { .type = DMG_ENEMY_COLLISION });
	}

	#ifdef DEBUG
	if(env_get("TAISEI_SKIP_TO_DIALOG", 0) && gamekeypressed(KEY_FOCUS)) {
		over = true;
	}
	#endif

	if(over) {
		if(global.stage->type == STAGE_SPELL && boss->current->type != AT_Move && boss->current->failtime) {
			stage_gameover();
		}

		log_debug("Current attack [%s] is over", boss->current->name);

		boss_reset_motion(boss);
		coevent_signal_once(&boss->current->events.completed);
		COEVENT_CANCEL_ARRAY(boss->current->events);

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
				boss_call_rule(boss, EVENT_BIRTH);

				coevent_signal_once(&boss->current->events.initiated);
				coevent_signal_once(&boss->current->events.started);
				coevent_signal_once(&boss->current->events.finished);
				coevent_signal_once(&boss->current->events.completed);
				COEVENT_CANCEL_ARRAY(boss->current->events);

				if(dialog_is_active(global.dialog)) {
					break;
				}

				continue;
			}

			if(boss_should_skip_attack(boss, boss->current)) {
				COEVENT_CANCEL_ARRAY(boss->current->events);
				continue;
			}

			boss_start_attack(boss, boss->current);
			break;
		}
	}
}

void boss_reset_motion(Boss *boss) {
	boss->move = move_stop(0.8);
}

static void boss_death_effect_draw_overlay(Projectile *p, int t, ProjDrawRuleArgs args) {
	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);
	r_framebuffer(framebuffers->front);
	r_uniform_sampler("noise_tex", "static");
	r_uniform_int("frames", global.frames);
	r_uniform_float("progress", t / p->timeout);
	r_uniform_vec2("origin", creal(p->pos), VIEWPORT_H - cimag(p->pos));
	r_uniform_vec2("clear_origin", creal(p->pos), VIEWPORT_H - cimag(p->pos));
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_float("size", hypotf(VIEWPORT_W, VIEWPORT_H));
	draw_framebuffer_tex(framebuffers->back, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(framebuffers);

	// This change must propagate, hence the r_state salsa. Yes, pop then push, I know what I'm doing.
	r_state_pop();
	r_framebuffer(framebuffers->back);
	r_state_push();
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
		stage_shake_view(100);

		PARTICLE(
			.pos = (*boss)->pos,
			.size = 1+I,
			.timeout = 120,
			.draw_rule = boss_death_effect_draw_overlay,
			.blend = BLEND_NONE,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
			.layer = LAYER_OVERLAY,
			.shader = "boss_death",
		);
	}

	free_boss(*boss);
	*boss = NULL;
}

static void free_attack(Attack *a) {
	COEVENT_CANCEL_ARRAY(a->events);
	free(a->name);
}

void free_boss(Boss *boss) {
	COEVENT_CANCEL_ARRAY(boss->events);

	for(int i = 0; i < boss->acount; i++) {
		free_attack(&boss->attacks[i]);
	}

	ent_unregister(&boss->ent);
	boss_set_portrait(boss, NULL, NULL, NULL);
	aniplayer_free(&boss->ani);
	free(boss->name);
	objpool_release(stage_object_pools.bosses, boss);
}

void boss_start_attack(Boss *b, Attack *a) {
	b->current = a;
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
	boss_call_rule(b, EVENT_BIRTH);

	if(ATTACK_IS_SPELL(a->type)) {
		play_sfx(a->type == AT_ExtraSpell ? "charge_extra" : "charge_generic");

		for(int i = 0; i < 10+5*(a->type == AT_ExtraSpell); i++) {
			RNG_ARRAY(rng, 4);

			PARTICLE(
				.sprite = "stain",
				.pos = CMPLX(VIEWPORT_W/2 + vrng_sreal(rng[0]) * VIEWPORT_W/4, VIEWPORT_H/2 + vrng_sreal(rng[1]) * 30),
				.color = RGBA(0.2, 0.3, 0.4, 0.0),
				.timeout = 50,
				.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
				.move = move_linear(vrng_sign(rng[2]) * 10 * vrng_range(rng[3], 1, 4)),
			);
		}
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
	boss_reset_motion(b);
	coevent_signal_once(&a->events.initiated);
}

Attack *boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule) {
	assert(boss->acount < BOSS_MAX_ATTACKS);

	Attack *a = &boss->attacks[boss->acount];
	boss->acount += 1;
	memset(a, 0, sizeof(Attack));

	a->type = type;
	a->name = strdup(name);
	a->timeout = timeout * FPS;

	a->maxhp = hp;
	a->hp = hp;

	a->rule = rule;
	a->draw_rule = draw_rule;

	a->starttime = global.frames;

	COEVENT_INIT_ARRAY(a->events);

	return a;
}

TASK(attack_task_helper, {
	BossAttackTask task;
	BossAttackTaskArgs task_args;
}) {
	// NOTE: We could do INVOKE_TASK_INDIRECT here, but that's a bit wasteful.
	// A better idea than both that and this contraption would be to come up
	// with an INVOKE_TASK_INDIRECT_WHEN.
	ARGS.task._cotask_BossAttack_thunk(&ARGS.task_args);
}

static void setup_attack_task(Boss *boss, Attack *a, BossAttackTask task) {
	INVOKE_TASK_WHEN(&a->events.initiated, attack_task_helper,
		.task = task,
		.task_args = {
			.boss = ENT_BOX(boss),
			.attack = a
		}
	);
}

Attack *boss_add_attack_task(Boss *boss, AttackType type, char *name, float timeout, int hp, BossAttackTask task, BossRule draw_rule) {
	Attack *a = boss_add_attack(boss, type, name, timeout, hp, NULL, draw_rule);
	setup_attack_task(boss, a, task);
	return a;
}

void boss_set_attack_bonus(Attack *a, int rank) {
	if(!ATTACK_IS_SPELL(a->type)) {
		return;
	}

	if(rank == 0) {
		log_warn("Bonus rank was not set for this spell!");
	}

	if(a->type == AT_SurvivalSpell) {
		a->bonus_base = (2000.0 + 600.0 * (a->timeout / (double)FPS)) * (9 + rank);
	} else {
		a->bonus_base = (2000.0 + a->hp * 0.6) * (9 + rank);
	}

	if(a->type == AT_ExtraSpell) {
		a->bonus_base *= 1.5;
	}
}

static void boss_generic_move(Boss *b, int time) {
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

Attack *boss_add_attack_from_info(Boss *boss, AttackInfo *info, char move) {
	if(move) {
		boss_add_attack(boss, AT_Move, "Generic Move", 0.5, 0, boss_generic_move, NULL)->info = info;
	}

	Attack *a = boss_add_attack(boss, info->type, info->name, info->timeout, info->hp, info->rule, info->draw_rule);
	a->info = info;
	boss_set_attack_bonus(a, info->bonus_rank);

	if(info->task._cotask_BossAttack_thunk) {
		setup_attack_task(boss, a, info->task);
	}

	return a;
}

Boss *init_boss_attack_task(BoxedBoss boss, Attack *attack) {
	Boss *pboss = TASK_BIND(boss);
	CANCEL_TASK_AFTER(&attack->events.finished, THIS_TASK);
	return pboss;
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
		"boss_circle",
		"boss_indicator",
		"boss_spellcircle0",
		"part/arc",
		"part/boss_shadow",
		"spell",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"boss_zoom",
		"boss_death",
		"spellcard_intro",
		"spellcard_outro",
		"spellcard_walloftext",
		"sprite_silhouette",
		"healthbar_linear",
		"healthbar_radial",
	NULL);

	StageInfo *s = global.stage;

	if(s->type != STAGE_SPELL || s->spell->type == AT_ExtraSpell) {
		preload_resources(RES_TEXTURE, RESF_DEFAULT,
			"stage3/wspellclouds",
			"stage4/kurumibg2",
		NULL);
	}
}
