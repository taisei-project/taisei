/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "boss.h"

#include "audio/audio.h"
#include "dynstage.h"
#include "entity.h"
#include "global.h"
#include "i18n/i18n.h"
#include "portrait.h"
#include "stage.h"
#include "stagedraw.h"
#include "stageobjects.h"
#include "stages/stage5/stage5.h"  // for unlockable bonus BGM
#include "stagetext.h"
#include "util/env.h"
#include "util/glm.h"
#include "util/graphics.h"

#define DAMAGE_PER_POWER_POINT 500.0f
#define DAMAGE_PER_POWER_ITEM  (DAMAGE_PER_POWER_POINT * POWER_VALUE)

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

TASK(boss_damage_to_power, { BoxedBoss boss; }) {
	auto boss = TASK_BIND(ARGS.boss);

	for(;;WAIT(2)) {
		if(boss->damage_to_power_accum >= DAMAGE_PER_POWER_ITEM) {
			spawn_item(boss->pos, ITEM_POWER);
			boss->damage_to_power_accum -= DAMAGE_PER_POWER_ITEM;
		}
	}
}

Boss *create_boss(const char *name, char *ani, cmplx pos) {
	auto boss = STAGE_ACQUIRE_OBJ(Boss);

	boss->name = name;
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
	INVOKE_TASK(boss_damage_to_power, ENT_BOX(boss));

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
	};

	assert(atype >= 0 && atype < sizeof(colors)/sizeof(*colors));
	return colors + atype;
}

static StageProgress *get_spellstage_progress(Attack *a, StageInfo **out_stginfo, bool write) {
	if(!write || (global.replay.input.replay == NULL && global.stage->type == STAGE_STORY)) {
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
	// Skip zero-length spells. Zero-length AT_Move and AT_Normal attacks are ok.
	// FIXME: I'm really not sure what was the purpose of this, but for now I'm abusing this to
	// conditionally flag the extra spell as skipped. Investigate whether we can remove simplify
	// things a bit and remove this function.
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

static bool boss_attack_is_final(Boss *boss, Attack *a) {
	return boss_get_final_attack(boss) == a;
}

static void update_hud_attack_ptrs(Boss *boss) {
	boss->hud.a_cur = NULL;
	boss->hud.a_next = NULL;
	boss->hud.a_prev = NULL;

	for(Attack *a = boss->attacks, *a_end = boss->attacks + boss->acount; a < a_end; ++a) {
		if(boss_should_skip_attack(boss, a) || a->type == AT_Move) {
			continue;
		}

		if(boss->hud.a_cur != NULL) {
			boss->hud.a_next = a;
			break;
		}

		if(a == boss->current) {
			boss->hud.a_cur = boss->current;
			continue;
		}

		if(a->hp <= 0 && attack_has_finished(a)) {
			if(boss->current == NULL) {
				boss->hud.a_prev = boss->hud.a_cur;
				boss->hud.a_cur = a;
			} else {
				boss->hud.a_prev = a;
			}
		}
	}

// 	log_debug("prev = %s", boss->hud.a_prev ? boss->hud.a_prev->name : NULL);
// 	log_debug("cur  = %s", boss->hud.a_cur  ? boss->hud.a_cur->name  : NULL);
// 	log_debug("next = %s", boss->hud.a_next ? boss->hud.a_next->name : NULL);
}

static void update_healthbar(Boss *boss) {
	bool radial = healthbar_style_is_radial();
	bool player_nearby = radial && cabs(boss->pos - global.plr.pos) < 128;
	float update_speed = 0.1;
	float target_opacity = 1.0;
	float target_fill = 0.0;
	float target_altfill = 0.0;

	Attack *a_prev = boss->hud.a_prev;
	Attack *a_cur = boss->hud.a_cur;
	Attack *a_next = boss->hud.a_next;

	if(
		boss_is_dying(boss) ||
		boss_is_fleeing(boss) ||
		!a_cur ||
		a_cur->type == AT_Move ||
		dialog_is_active(global.dialog)
	) {
		target_opacity = 0.0;
	}

	if(player_nearby || boss->in_background) {
		target_opacity *= 0.25;
	}

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
				spell_hp = spell_maxhp = max(1, total_maxhp * 0.1f);
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
			total_maxhp = max(0.001f, total_maxhp);

			if(total_hp > 0) {
				total_hp = max(0.001f, total_hp);
			}

			target_fill = total_hp / total_maxhp;
		}
	}

	float opacity_update_speed = update_speed;

	if(boss_is_dying(boss)) {
		opacity_update_speed *= 0.25;
	}

	fapproach_asymptotic_p(&boss->healthbar.opacity, target_opacity, opacity_update_speed, 1e-3);

	if(
		boss_is_vulnerable(boss) ||
		(a_cur && a_cur->type == AT_SurvivalSpell && attack_has_started(a_cur))
	) {
		update_speed *= (1 + 4 * pow(1 - target_fill, 2));
	}

	fapproach_asymptotic_p(&boss->healthbar.fill_total, target_fill, update_speed, 1e-3);
	fapproach_asymptotic_p(&boss->healthbar.fill_alt, target_altfill, update_speed, 1e-3);
}

static void update_hud(Boss *boss) {
	update_hud_attack_ptrs(boss);
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

	if(!boss->current || !ATTACK_IS_SPELL(boss->current->type) || attack_has_finished(boss->current)) {
		target_spell_opacity = 0.0;
	}

	if(im(global.plr.pos) < 128) {
		target_plrproximity_opacity = 0.25;
	}

	if(boss->current && !attack_has_finished(boss->current) && boss->current->type != AT_Move) {
		int frames = boss->current->timeout + min(0, boss->current->starttime - global.frames);
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
	r_mat_mv_translate(re(boss->pos), im(boss->pos), 0);
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
	const char *msg = _("~ Spell Card Attack! ~");

	float msg_width = text_width(font, msg, 0);
	float flash = 0.2 + 0.8 * pow(psin(M_PI + 5 * M_PI * f), 0.5);
	f = 0.15 * f + 0.85 * (0.5 * pow(2 * f - 1, 3) + 0.5);
	opacity *= 1 - 2 * fabs(f - 0.5);
	cmplx pos = (VIEWPORT_W + msg_width) * f - msg_width * 0.5 + I * y_pos;

	draw_boss_text(ALIGN_CENTER, re(pos), im(pos), msg, font, color_mul_scalar(RGBA(1, flash, flash, 1), opacity));
}

static void draw_spell_name(Boss *b, int time, bool healthbar_radial) {
	Font *font = res_font("standard");

	cmplx x0 = VIEWPORT_W/2+I*VIEWPORT_H/3.5;
	float f = clamp((time - 40.0) / 60.0, 0, 1);
	float f2 = clamp(time / 80.0, 0, 1);
	float y_offset = 26 + healthbar_radial * -15;
	float y_text_offset = 5 - font_get_metrics(font)->descent;
	cmplx x = x0 + ((VIEWPORT_W - 10) + I*(y_offset + y_text_offset) - x0) * f*(f+1)*0.5;
	int strw = text_width(font, _(b->current->name), 0);

	float opacity_noplr = b->hud.spell_opacity * b->hud.global_opacity;
	float opacity = opacity_noplr * b->hud.plrproximity_opacity;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = res_sprite("spell"),
		.shader_ptr = res_shader("sprite_default"),
		.pos = { (VIEWPORT_W - 128), y_offset * (1 - pow(1 - f2, 5)) + VIEWPORT_H * pow(1 - f2, 2) },
		.color = color_mul_scalar(RGBA(1, 1, 1, f2 * 0.5), opacity * f2) ,
		.scale.both = 3 - 2 * (1 - pow(1 - f2, 3)),
	});

	int delay = attacktype_start_delay(b->current->type);
	float warn_progress = clamp((time + delay) / 120.0, 0, 1);

	r_mat_mv_push();
	r_mat_mv_translate(re(x), im(x),0);
	float scale = f+1.*(1-f)*(1-f)*(1-f);
	r_mat_mv_scale(scale,scale,1);
	r_mat_mv_rotate(glm_ease_quad_out(f) * 2 * M_PI, 0.8, -0.2, 0);

	float spellname_opacity_noplr = opacity_noplr * min(1, warn_progress/0.6f);
	float spellname_opacity = spellname_opacity_noplr * b->hud.plrproximity_opacity;

	draw_boss_text(ALIGN_RIGHT, strw/2*(1-f), 0, _(b->current->name), font, color_mul_scalar(RGBA(1, 1, 1, 1), spellname_opacity));

	if(spellname_opacity_noplr < 1) {
		r_mat_mv_push();
		r_mat_mv_scale(2 - spellname_opacity_noplr, 2 - spellname_opacity_noplr, 1);
		draw_boss_text(ALIGN_RIGHT, strw/2*(1-f), 0, _(b->current->name), font, color_mul_scalar(RGBA(1, 1, 1, 1), spellname_opacity_noplr * 0.5));
		r_mat_mv_pop();
	}

	r_mat_mv_pop();

	if(warn_progress < 1) {
		draw_spell_warning(font, im(x0) - font_get_lineskip(font), warn_progress, opacity);
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

		// TODO: display plrmode-specific data?
		snprintf(buf, sizeof(buf), "%u / %u", p->global.num_cleared, p->global.num_played);

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

	float char_in = clamp(a * 1.5f, 0, 1);
	float char_out = min(1, 2 - (2 * a));
	float char_opacity_in = 0.75f * min(1, a * 5);
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
			.pos = { char_spr->w * 0.5 + VIEWPORT_W * powf(1 - char_in, 4 - i * 0.3f) - i + char_xofs, VIEWPORT_H - char_spr->h * 0.5 },
			.color = color_mul_scalar(color_add(RGBA(0.2, 0.2, 0.2, 0), RGBA(i==1, i==2, i==3, 0)), char_opacity_in * (1 - char_in * o) * o),
			.flip.x = true,
			.scale.both = 1.0f + 0.02f * (min(1, a * 1.2f)) + i * 0.5 * powf(1 - o, 2),
		});
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = char_spr,
		.pos = { char_spr->w * 0.5f + VIEWPORT_W * powf(1 - char_in, 4) + char_xofs, VIEWPORT_H - char_spr->h * 0.5f },
		.color = RGBA_MUL_ALPHA(1, 1, 1, char_opacity * min(1, char_in * 2) * (1 - min(1, (1 - char_out) * 5))),
		.flip.x = true,
		.scale.both = 1.0f + 0.1f * (1 - char_out),
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
		.pos = { re(p->pos), im(p->pos) },
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
		.pos = boss->pos + boss_get_sprite_offset(boss),
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
		bool is_spell = cur && ATTACK_IS_SPELL(cur->type) && !attack_has_finished(cur);
		bool is_extra = cur && cur->type == AT_ExtraSpell && attack_has_started(cur);

		if(!(global.frames % 13) && !is_extra) {
			ENT_ARRAY_COMPACT(&smoke_parts);
			ENT_ARRAY_ADD(&smoke_parts, PARTICLE(
				.sprite = "smoke",
				.pos = cdir(global.frames) + boss->pos,
				.color = RGBA(shadowcolor->r, shadowcolor->g, shadowcolor->b, 0.0),
				.timeout = 180,
				.draw_rule = pdraw_timeout_scale(2, 0.01),
				.angle = rng_angle(),
				.flags = PFLAG_MANUALANGLE,
			));
		}

		if(
			!(global.frames % (2 + 2 * is_extra)) &&
			(is_spell || boss_is_dying(boss)) &&
			!boss->in_background
		) {
			float glowstr = 0.5;
			float a = (1.0 - glowstr) + glowstr * psin(global.frames/15.0);
			spawn_boss_glow(boss, color_mul_scalar(COLOR_COPY(glowcolor), a), 24);
		}
	}
}

void draw_boss_background(Boss *boss) {
	r_mat_mv_push();
	r_mat_mv_translate(re(boss->pos), im(boss->pos), 0);
	r_mat_mv_rotate(global.frames * 4.0 * DEG2RAD, 0, 0, -1);

	float f = 0.8+0.1*sin(global.frames/8.0);

	if(boss_is_dying(boss)) {
		float t = (global.frames - boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		f -= t * (t - 0.7f) / max(0.01f, 1-t);
	}

	r_mat_mv_scale(f, f, 1);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = res_sprite("boss_circle"),
		.shader_ptr = res_shader("sprite_particle"),
		.shader_params = &(ShaderCustomParams) { 1.0f },
		.color = RGBA(1, 1, 1, 0),
	});
	r_mat_mv_pop();
}

cmplx boss_get_sprite_offset(Boss *boss) {
	return 6 * sin(global.frames/25.0) * I;
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

	Color *c = RGB(1.0f, 1.0f - red, 1.0f - red * 0.5f);
	color_lerp(c, RGB(0.2f, 0.2f, 0.2f), boss->background_transition);
	color_mul_scalar(c, boss_alpha);

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&boss->ani),
		.shader_ptr = res_shader("sprite_particle"),
		.shader_params = &(ShaderCustomParams) { 1.0f },
		.pos.as_cmplx = boss->pos + boss_get_sprite_offset(boss),
		.color = c,
	});
}

void draw_boss_fake_overlay(Boss *boss) {
	if(healthbar_style_is_radial()) {
		draw_radial_healthbar(boss);
	}
}

void draw_boss_overlay(Boss *boss) {
	bool radial_style = healthbar_style_is_radial();

	float o = boss->hud.global_opacity * boss->hud.plrproximity_opacity;

	if(o > 0) {
		if(boss->current && ATTACK_IS_SPELL(boss->current->type)) {
			int t_portrait, t_spell;
			t_portrait = t_spell = global.frames - boss->current->starttime;
			t_portrait += attacktype_start_delay(boss->current->type);

			draw_spell_portrait(boss, t_portrait);
			draw_spell_name(boss, t_spell, radial_style);
		}

		draw_boss_text(ALIGN_LEFT, 10, 20 + 8 * !radial_style, _(boss->name), res_font("standard"), RGBA(o, o, o, o));

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

		for(Attack *a = boss->hud.a_cur, *a_end = boss->attacks + boss->acount; a && a < a_end; ++a) {
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

	if(!radial_style) {
		draw_linear_healthbar(boss);
	}
}

static void boss_rule_extra(Boss *boss, float alpha) {
	if(global.frames % 5 || boss->in_background) {
		return;
	}

	// XXX: not sure why, but the cast is needed to not desync 1.4 replays
	int cnt = 5 * (double)max(1, alpha);
	alpha = min(2, alpha);
	int lt = 1;

	if(alpha == 0) {
		lt += 2;
		alpha = rng_real();
	}

	for(int i = 0; i < cnt; ++i) {
		float a = i*2*M_PI/cnt + global.frames / 100.0;
		cmplx dir = cdir(a+global.frames/50.0);
		cmplx vel = dir * 3;
		float v = max(0, alpha - 1);
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
	return
		boss->current &&
		attack_has_finished(boss->current) &&
		boss->current->type != AT_Move &&
		boss_attack_is_final(boss, boss->current);
}

bool boss_is_fleeing(Boss *boss) {
	return
		boss->current &&
		boss->current->type == AT_Move &&
		boss_attack_is_final(boss, boss->current);
}

bool boss_is_vulnerable(Boss *boss) {
	Attack *atk = boss->current;

	return
		!boss->in_background &&
		atk &&
		atk->type != AT_Move &&
		atk->type != AT_SurvivalSpell &&
		!attack_has_finished(atk);
}

bool boss_is_player_collision_active(Boss *boss) {
	return
		boss->current &&
		boss->current->type != AT_Move &&
		attack_is_active(boss->current) &&
		!boss->in_background &&
		boss->background_transition < 0.5f &&
		!boss_is_dying(boss) &&
		!boss_is_fleeing(boss);
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
		factor = clamp((pattern_time - min_damage_time) / span, 0.0f, 1.0f);
	}

	if(factor == 0) {
		return DMG_RESULT_IMMUNE;
	}

	if(dmg->amount > 0 && global.frames-boss->lastdamageframe > 2) {
		boss->lastdamageframe = global.frames;
	}

	float damage = min(boss->current->hp, dmg->amount * factor);
	boss->current->hp -= damage;
	boss->damage_to_power_accum += damage;

	if(boss->current->hp < boss->current->maxhp * 0.1) {
		play_sfx_loop("hit1");
	} else {
		play_sfx_loop("hit0");
	}

	return DMG_RESULT_OK;
}

static void calc_spell_bonus(Attack *a, SpellBonus *bonus) {
	bool survival = a->type == AT_SurvivalSpell;
	bonus->failed = attack_was_failed(a);

	int time_left = clamp(a->starttime + a->timeout - global.frames, 0, a->timeout);

	double piv_factor = global.plr.point_item_value / (double)PLR_START_PIV;
	double base = a->bonus_base * 0.5 * (1 + piv_factor);

	bonus->clear = bonus->failed ? 0 : base;
	bonus->time = survival ? 0 : 0.5 * base * (time_left / (double)a->timeout);
	bonus->endurance = 0;
	bonus->survival = 0;

	if(bonus->failed) {
		bonus->time /= 4;
		bonus->endurance = base * 0.1 * (max(0, a->failtime - a->starttime) / (double)a->timeout);
	} else if(survival) {
		bonus->survival = base * (1.0 + 0.02 * (a->timeout / (double)FPS));
	}

	bonus->total = bonus->clear + bonus->time + bonus->endurance + bonus->survival;
	bonus->diff_multiplier = 0.6 + 0.2 * global.diff;
	bonus->total *= bonus->diff_multiplier;
}

static void boss_give_spell_bonus(Boss *boss, Attack *a, Player *plr) {
	SpellBonus bonus = {};
	calc_spell_bonus(a, &bonus);

	const char *title = bonus.failed ? _("Spell Card failed…")  : _("Spell Card captured!");

	char diff_bonus_text[6];
	snprintf(diff_bonus_text, sizeof(diff_bonus_text), "x%.2f", bonus.diff_multiplier);

	player_add_points(plr, bonus.total, plr->pos);

	StageTextTable tbl;
	stagetext_begin_table(&tbl, title, RGB(1, 1, 1), RGB(1, 1, 1), VIEWPORT_W/2, 0,
		ATTACK_END_DELAY_SPELL * 2, ATTACK_END_DELAY_SPELL / 2, ATTACK_END_DELAY_SPELL);
	stagetext_table_add_numeric_nonzero(&tbl, _("Clear bonus"), bonus.clear);
	stagetext_table_add_numeric_nonzero(&tbl, _("Time bonus"), bonus.time);
	stagetext_table_add_numeric_nonzero(&tbl, _("Survival bonus"), bonus.survival);
	stagetext_table_add_numeric_nonzero(&tbl, _("Endurance bonus"), bonus.endurance);
	stagetext_table_add_separator(&tbl);
	stagetext_table_add(&tbl, _("Diff. multiplier"), diff_bonus_text);
	stagetext_table_add_numeric(&tbl, _("Total"), bonus.total);
	stagetext_end_table(&tbl);

	play_sfx("spellend");

	if(!bonus.failed) {
		play_sfx("spellclear");
	}
}

const char *attacktype_name(AttackType t) {
	switch(t) {
		case AT_Spellcard:      return _("Normal");
		case AT_SurvivalSpell:  return _("Survival");
		case AT_ExtraSpell:     return _("Extra");
		case AT_Move:           return _("Move");
		default:                return _("Unknown");
	}
}

int attacktype_start_delay(AttackType t) {
	switch(t) {
		case AT_ExtraSpell: return ATTACK_START_DELAY_EXTRA;
		default: return ATTACK_START_DELAY;
	}
}

int attacktype_end_delay(AttackType t) {
	switch(t) {
		case AT_Spellcard:      return ATTACK_END_DELAY_SPELL;
		case AT_SurvivalSpell:  return ATTACK_END_DELAY_SURV;
		case AT_ExtraSpell:     return ATTACK_END_DELAY_EXTRA;
		case AT_Move:           return ATTACK_END_DELAY_MOVE;
		default:                return ATTACK_END_DELAY;
	}
}

static int attack_end_delay(Boss *boss) {
	if(boss_attack_is_final(boss, boss->current)) {
		return BOSS_DEATH_DELAY;
	}

	return attacktype_end_delay(boss->current->type);
}

static bool spell_is_overload(AttackInfo *spell) {
	// HACK HACK HACK
	StagesExports *e = dynstage_get_exports();
	struct stage5_spells_s *stage5_spells = (struct stage5_spells_s*)e->stage5.spells;
	return spell == &stage5_spells->extra.overload;
}

static void clear_hazards_and_enemies(void) {
	enemy_kill_all(&global.enemies);
	stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_FORCE);
}

void boss_finish_current_attack(Boss *boss) {
	AttackType t = boss->current->type;

	boss->in_background = false;
	boss->current->hp = 0;

	aniplayer_soft_switch(&boss->ani,"main",0);

	if(t != AT_Move) {
		clear_hazards_and_enemies();
	}

	if(ATTACK_IS_SPELL(t)) {
		boss_give_spell_bonus(boss, boss->current, &global.plr);

		if(!attack_was_failed(boss->current)) {
			StageProgress *p = get_spellstage_progress(boss->current, NULL, true);

			if(p) {
				progress_register_stage_cleared(p, global.plr.mode);
			}

			// HACK
			if(spell_is_overload(boss->current->info)) {
				stage_unlock_bgm("bonus0");
			}
		} else if(boss->current->type != AT_ExtraSpell) {
			boss->failed_spells++;
		}

		spawn_items(boss->pos,
			ITEM_POWER,  14,
			ITEM_POINTS, 12,
			ITEM_BOMB_FRAGMENT, (attack_was_failed(boss->current) ? 0 : 1)
		);
	}

	if(boss->current < boss->attacks + boss->acount - 1) {
		// If the next attack is an extra spell, determine whether we have enough voltage now, and
		// if not, skip it. This can't be done any later, because we have to know whether to start
		// the death sequence this frame (since the extra spell is usually the final one).
		Attack *next = boss->current + 1;
		if(next->type == AT_ExtraSpell && global.plr.voltage < global.voltage_threshold) {
			// see boss_should_skip_attack()
			next->timeout = 0;
		}
	}

	boss->current->endtime = global.frames + attack_end_delay(boss);
	boss->current->endtime_undelayed = global.frames;

	boss_reset_motion(boss);
	coevent_signal_once(&boss->current->events.finished);
}

static void boss_schedule_next_attack(Boss *b, Attack *a);

void process_boss(Boss **pboss) {
	Boss *boss = *pboss;

	if(!boss) {
		return;
	}

	move_update(&boss->pos, &boss->move);
	aniplayer_update(&boss->ani);
	update_hud(boss);

	fapproach_asymptotic_p(&boss->background_transition, boss->in_background ? 1.0f : 0.0f, 0.15, 1e-3);
	boss->ent.draw_layer = lerp(LAYER_BOSS, LAYER_BACKGROUND, boss->background_transition);

	if(!boss->current || dialog_is_active(global.dialog)) {
		return;
	}

	int time = global.frames - boss->current->starttime;
	bool extra = boss->current->type == AT_ExtraSpell;
	bool over = attack_has_finished(boss->current) && global.frames >= boss->current->endtime;

	if(time == 0) {
		coevent_signal_once(&boss->current->events.started);
	}

	if(!attack_has_finished(boss->current)) {
		int remaining = boss->current->timeout - time;

		if(boss->current->type != AT_Move && remaining <= 11*FPS && remaining > 0 && !(time % FPS)) {
			play_sfx(remaining <= 6*FPS ? "timeout2" : "timeout1");
		}
	}

	if(extra) {
		float base = 0.2;
		float ampl = 0.2;
		float s = sin(time / 90.0 + M_PI*1.2);

		if(attack_has_finished(boss->current)) {
			float p = (boss->current->endtime - global.frames)/(float)ATTACK_END_DELAY_EXTRA;
			float a = max((base + ampl * s) * p * 0.5f, 5 * powf(1 - p, 3));
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
			float o = min(0, -5 + time/30.0f);
			float q = (time <= 150? 1 - powf(time/250.0f, 2) : min(1, time/60.0f));

			boss_rule_extra(boss, max(1-time/300.0f, base + ampl * s) * q);
			if(o) {
				boss_rule_extra(boss, max(1-time/300.0f, base + ampl * s) - o);

				stage_shake_view(5);
				if(o > -0.05) {
					stage_shake_view(10);
				}
			}
		}
	}

	bool timedout = time > boss->current->timeout;

	if((boss->current->type != AT_Move && boss->current->hp <= 0) || timedout) {
		if(!attack_has_finished(boss->current)) {
			if(timedout && boss->current->type != AT_SurvivalSpell) {
				boss->current->failtime = global.frames;
			}

			boss_finish_current_attack(boss);
		} else if(boss->current->type != AT_Move || !boss_attack_is_final(boss, boss->current)) {
			// XXX: do we actually need to call this for AT_Move attacks at all?
			//      it should be harmless, but probably unnecessary.
			//      i'll be conservative and leave it in for now.
			clear_hazards_and_enemies();
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
		if(
			global.stage->type == STAGE_SPELL &&
			boss->current->type != AT_Move &&
			attack_was_failed(boss->current)
		) {
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

			if(boss_should_skip_attack(boss, boss->current)) {
				COEVENT_CANCEL_ARRAY(boss->current->events);
				continue;
			}

			Attack *next = boss->current;
			boss->current = NULL;
			boss_schedule_next_attack(boss, next);
			break;
		}
	}
}

void boss_reset_motion(Boss *boss) {
	boss->move = move_dampen(boss->move.velocity, 0.8);
}

static void boss_death_effect_draw_overlay(Projectile *p, int t, ProjDrawRuleArgs args) {
	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);
	r_framebuffer(framebuffers->front);
	r_uniform_sampler("noise_tex", "static");
	r_uniform_int("frames", global.frames);
	r_uniform_float("progress", t / p->timeout);
	r_uniform_vec2("origin", re(p->pos), VIEWPORT_H - im(p->pos));
	r_uniform_vec2("clear_origin", re(p->pos), VIEWPORT_H - im(p->pos));
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
	bool fled = false;

	if(!a) {
		// XXX: why does this happen?
		log_debug("FIXME: boss had no final attacK?");
	} else {
		fled = a->type == AT_Move;
	}

	if((*boss)->acount && !fled) {
		petal_explosion(35, (*boss)->pos);
	}

	if(!fled) {
		clear_hazards_and_enemies();
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
}

void free_boss(Boss *boss) {
	COEVENT_CANCEL_ARRAY(boss->events);

	for(int i = 0; i < boss->acount; i++) {
		free_attack(&boss->attacks[i]);
	}

	ent_unregister(&boss->ent);
	boss_set_portrait(boss, NULL, NULL, NULL);
	aniplayer_free(&boss->ani);
	STAGE_RELEASE_OBJ(boss);
}

static void boss_schedule_next_attack(Boss *b, Attack *a) {
	assert(b->current == NULL);
	assert(!a->events.initiated.num_signaled);
	log_debug("[%i] %s", global.frames, a->name);
	coevent_signal_once(&a->events.initiated);
}

void boss_engage(Boss *b) {
	boss_schedule_next_attack(b, b->attacks);
}

void boss_start_next_attack(Boss *b, Attack *a) {
	assert(b->current == NULL);
	assert(a->events.initiated.num_signaled > 0);
	assert(a->events.started.num_signaled == 0);

	b->current = a;
	log_debug("[%i] %s", global.frames, a->name);

	StageInfo *i;
	StageProgress *p = get_spellstage_progress(a, &i, true);

	if(p) {
		if(!p->unlocked) {
			char title[STAGE_MAX_TITLE_SIZE];
			stageinfo_format_localized_title(i, sizeof(title), title);
			log_info("Spellcard unlocked! %s: %s", title, _(i->subtitle));
			p->unlocked = true;
		}

		progress_register_stage_played(p, global.plr.mode);
	}

	// This should go before a->rule(b,EVENT_BIRTH), so it doesn’t reset values set by the attack rule.
	b->bomb_damage_multiplier = 1.0;
	b->shot_damage_multiplier = 1.0;

	a->starttime = global.frames + attacktype_start_delay(a->type);

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

	clear_hazards_and_enemies();
}

Attack *boss_add_attack(Boss *boss, AttackType type, const char *name, float timeout, int hp, BossRule draw_rule) {
	assert(boss->acount < BOSS_MAX_ATTACKS);

	Attack *a = &boss->attacks[boss->acount++];
	*a = (typeof(*a)) {
		.type = type,
		.name = name,
		.timeout = timeout * FPS,
		.maxhp = hp,
		.hp = hp,
		.draw_rule = draw_rule,
	};

	COEVENT_INIT_ARRAY(a->events);

	return a;
}

static Attack *_boss_add_attack_from_info(Boss *boss, AttackInfo *info){
	Attack *a = boss_add_attack(
		boss,
		info->type,
		info->name,
		info->timeout,
		info->hp,
		info->draw_rule
	);

	a->info = info;
	boss_set_attack_bonus(a, info->bonus_rank);

	return a;
}

TASK(attack_task_helper_custom, {
	CoEvent *evt;
	BossAttackTask task;
	BossAttackTaskArgs *task_args;
	size_t task_args_size;
	BossAttackTaskArgs task_args_header;
}) {
	size_t args_size = ARGS.task_args_size;
	BossAttackTaskArgs *orig_args = NOT_NULL(ARGS.task_args);
	char *args = TASK_MALLOC(args_size);
	size_t header_ofs = sizeof(ARGS.task_args_header);
	assume(args_size >= header_ofs);

	memcpy(args, &ARGS.task_args_header, sizeof(ARGS.task_args_header));
	memcpy(args + header_ofs, (char*)orig_args + header_ofs, args_size - header_ofs);

	WAIT_EVENT_OR_DIE(ARGS.evt);

	ARGS.task._cotask_BossAttack_thunk(args, args_size);
}

static void setup_attack_task_with_custom_args(
	Boss *boss, Attack *a, BossAttackTask task,
	BossAttackTaskArgs *args, size_t args_size
) {
	INVOKE_TASK(attack_task_helper_custom,
		.evt = &a->events.initiated,
		.task = task,
		.task_args = args,
		.task_args_size = args_size,
		.task_args_header = {
			.boss = ENT_BOX(boss),
			.attack = a
		}
	);
}

TASK(attack_task_helper, {
	BossAttackTask task;
	BossAttackTaskArgs task_args;
}) {
	ARGS.task._cotask_BossAttack_thunk(&ARGS.task_args, sizeof(ARGS.task_args));
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

Attack *boss_add_attack_task(Boss *boss, AttackType type, const char *name, float timeout, int hp, BossAttackTask task, BossRule draw_rule) {
	Attack *a = boss_add_attack(boss, type, name, timeout, hp, draw_rule);
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

TASK(boss_generic_move_finish, { BoxedBoss boss; cmplx dest; }) {
	Boss *b = TASK_BIND(ARGS.boss);
	b->pos = ARGS.dest;
	b->move = move_linear(0);
}

TASK_WITH_INTERFACE(boss_generic_move, BossAttack) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	cmplx dest = ARGS.attack->info->pos_dest;
	b->move = move_from_towards(b->pos, dest, 0.07);
	INVOKE_TASK_WHEN(&ARGS.attack->events.completed, boss_generic_move_finish, ENT_BOX(b), dest);
	BEGIN_BOSS_ATTACK(&ARGS);
}

Attack *boss_add_attack_from_info(Boss *boss, AttackInfo *info, bool move) {
	if(move) {
		Attack *m = boss_add_attack_task(
			boss, AT_Move, "Generic Move", 0.5, 0,
			TASK_INDIRECT(BossAttack, boss_generic_move), NULL
		);
		m->info = info;
	}

	Attack *a = _boss_add_attack_from_info(boss, info);

	if(info->task._cotask_BossAttack_thunk) {
		setup_attack_task(boss, a, info->task);
	}

	return a;
}

Attack *boss_add_attack_from_info_with_args(
	Boss *boss, AttackInfo *info, BossAttackTaskCustomArgs args
) {
	Attack *a = _boss_add_attack_from_info(boss, info);
	assume(info->task._cotask_BossAttack_thunk != NULL);
	setup_attack_task_with_custom_args(boss, a, info->task, args.ptr, args.size);
	return a;
}

Attack *boss_add_attack_task_with_args(
	Boss *boss, AttackType type, const char *name, float timeout, int hp,
	BossAttackTask task, BossRule draw_rule, BossAttackTaskCustomArgs args
) {
	Attack *a = boss_add_attack(boss, type, name, timeout, hp, draw_rule);
	setup_attack_task_with_custom_args(boss, a, task, args.ptr, args.size);
	return a;
}

Boss *_init_boss_attack_task(const BossAttackTaskArgs *args) {
	Boss *pboss = TASK_BIND(args->boss);
	CANCEL_TASK_AFTER(&args->attack->events.finished, THIS_TASK);
	return pboss;
}

void _begin_boss_attack_task(const BossAttackTaskArgs *args) {
	boss_start_next_attack(NOT_NULL(ENT_UNBOX(args->boss)), args->attack);
	WAIT_EVENT_OR_DIE(&args->attack->events.started);
}

void boss_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_SFX, RESF_OPTIONAL,
		"charge_generic",
		"charge_extra",
		"discharge",
		"spellend",
		"spellclear",
		"timeout1",
		"timeout2",
		"bossdeath",
	NULL);

	res_group_preload(rg, RES_SPRITE, RESF_DEFAULT,
		"boss_circle",
		"boss_indicator",
		"boss_spellcircle0",
		"part/arc",
		"part/blast_huge_rays",
		"part/boss_shadow",
		"spell",
	NULL);

	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
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
		res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
			"stage3/wspellclouds",
			"stage4/kurumibg2",
		NULL);
	}
}
