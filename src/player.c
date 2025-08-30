/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "player.h"

#include "audio/audio.h"
#include "entity.h"
#include "gamepad.h"
#include "global.h"
#include "intl/intl.h"
#include "plrmodes.h"
#include "projectile.h"
#include "replay/stage.h"
#include "replay/struct.h"
#include "stage.h"
#include "stagedraw.h"
#include "stagetext.h"
#include "stats.h"
#include "util/glm.h"
#include "util/graphics.h"

DEFINE_ENTITY_TYPE(PlayerIndicators, {
	Player *plr;

	struct {
		Sprite *focus;
	} sprites;

	float focus_alpha;
	int focus_time;
});

void player_init(Player *plr) {
	*plr = (typeof(*plr)) {
		.pos = PLR_SPAWN_POS,
		.lives = PLR_START_LIVES,
		.bombs = PLR_START_BOMBS,
		.point_item_value = PLR_START_PIV,
		.power_stored = 100,
		.deathtime = -1,
		.continuetime = -1,
		.bomb_triggertime = -1,
		.bomb_endtime = 0,
		.mode = plrmode_find(0, 0),
	};
}

void player_stage_pre_init(Player *plr) {
	plr->recoverytime = 0;
	plr->respawntime = 0;
	plr->bomb_triggertime = -1;
	plr->bomb_endtime = 0;
	plr->deathtime = -1;
	plr->axis_lr = 0;
	plr->axis_ud = 0;
}

double player_property(Player *plr, PlrProperty prop) {
	return plr->mode->procs.property(plr, prop);
}

static void ent_draw_player(EntityInterface *ent);
static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg);

DECLARE_TASK(player_logic, { BoxedPlayer plr; });
DECLARE_TASK(player_indicators, { BoxedPlayer plr; });

void player_stage_post_init(Player *plr) {
	assert(plr->mode != NULL);

	// ensure the essential callbacks are there. other code tests only for the optional ones
	assert(plr->mode->procs.property != NULL);
	assert(plr->mode->character != NULL);
	assert(plr->mode->dialog != NULL);

	plrchar_render_bomb_portrait(plr->mode->character, &plr->bomb_portrait);
	aniplayer_create(&plr->ani, plrchar_player_anim(plr->mode->character), "main");

	plr->ent.draw_layer = LAYER_PLAYER;
	plr->ent.draw_func = ent_draw_player;
	plr->ent.damage_func = ent_damage_player;
	ent_register(&plr->ent, ENT_TYPE_ID(Player));

	COEVENT_INIT_ARRAY(plr->events);

	INVOKE_TASK_DELAYED(1, player_logic, ENT_BOX(plr));
	INVOKE_TASK_DELAYED(1, player_indicators, ENT_BOX(plr));

	if(plr->mode->procs.init != NULL) {
		plr->mode->procs.init(plr);
	}

	plr->extralife_threshold = player_next_extralife_threshold(plr->extralives_given);

	while(plr->points >= plr->extralife_threshold) {
		plr->extralife_threshold = player_next_extralife_threshold(++plr->extralives_given);
	}
}

void player_free(Player *plr) {
	COEVENT_CANCEL_ARRAY(plr->events);
	r_texture_destroy(plr->bomb_portrait.tex);
	aniplayer_free(&plr->ani);
	ent_unregister(&plr->ent);
}

static void player_full_power(Player *plr) {
	stage_clear_hazards(CLEAR_HAZARDS_ALL);
	stagetext_add(_("Full Power!"), VIEWPORT_W * 0.5 + VIEWPORT_H * 0.33 * I, ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 60, 20, 20);
}

static int player_track_effective_power_change(Player *plr) {
	int old_effective = plr->_prev_effective_power;
	int new_effective = player_get_effective_power(plr);

	if(old_effective != new_effective) {
		plr->_prev_effective_power = new_effective;
		coevent_signal(&plr->events.effective_power_changed);
	}

	return new_effective;
}

bool player_set_power(Player *plr, short npow) {
	int old_stored = plr->power_stored;
	int new_stored = clamp(npow, 0, PLR_MAX_POWER_STORED);
	plr->power_stored = new_stored;

	if(old_stored / 100 < new_stored / 100) {
		play_sfx("powerup");
	}

	bool change = old_stored != new_stored;

	if(change) {
		if(new_stored >= PLR_MAX_POWER_EFFECTIVE && old_stored < PLR_MAX_POWER_EFFECTIVE) {
			player_full_power(plr);
		}

		coevent_signal(&plr->events.stored_power_changed);
	}

	player_track_effective_power_change(plr);

	return change;
}

bool player_add_power(Player *plr, short pdelta) {
	return player_set_power(plr, plr->power_stored + pdelta);
}

int player_get_effective_power(Player *plr) {
	int p;

	if(player_is_powersurge_active(plr)) {
		p = plr->powersurge.player_power;
	} else {
		p = plr->power_stored;
	}

	return clamp(p, 0, PLR_MAX_POWER_EFFECTIVE);
}

void player_move(Player *plr, cmplx delta) {
	delta *= player_property(plr, PLR_PROP_SPEED);
	plr->uncapped_velocity = delta;
	cmplx lastpos = plr->pos;
	cmplx ofs = CMPLX(PLR_MIN_BORDER_DIST, PLR_MIN_BORDER_DIST);
	cmplx vp = CMPLX(VIEWPORT_W, VIEWPORT_H);
	plr->pos = cwclamp(lastpos + delta, ofs, vp - ofs);
	plr->velocity = plr->pos - lastpos;
}

void player_draw_overlay(Player *plr) {
	float a = 1 - plr->bomb_cutin_alpha;

	if(a <= 0 || a >= 1) {
		return;
	}

	r_state_push();
	r_shader("sprite_default");

	float char_in = clamp(a * 1.5f, 0, 1);
	float char_out = min(1, 2 - (2 * a));
	float char_opacity_in = 0.75 * min(1, a * 5);
	float char_opacity = char_opacity_in * char_out * char_out;
	float char_xofs = -20 * a;

	Sprite *char_spr = &plr->bomb_portrait;

	for(int i = 1; i <= 3; ++i) {
		float t = a * 200;
		float dur = 20;
		float start = 200 - dur * 5;
		float end = start + dur;
		float ofs = 0.2 * dur * (i - 1);
		float o = 1 - smoothstep(start + ofs, end + ofs, t);

		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = char_spr,
			.pos = { char_spr->w * 0.5 + VIEWPORT_W * powf(1 - char_in, 4 - i * 0.3f) - i + char_xofs, VIEWPORT_H - char_spr->h * 0.5f },
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
		.scale.both = 1.0 + 0.1 * (1 - char_out),
	});

	float spell_in = min(1, a * 3.0);
	float spell_out = min(1, 3 - (3 * a));
	float spell_opacity = min(1, a * 5) * spell_out * spell_out;

	float spell_x = 128 * (1 - powf(1 - spell_in, 5)) + (VIEWPORT_W + 256) * powf(1 - spell_in, 3);
	float spell_y = VIEWPORT_H - 128 * sqrtf(a);

	Sprite *spell_spr = res_sprite("spell");

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = spell_spr,
		.pos = { spell_x, spell_y },
		.color = color_mul_scalar(RGBA(1, 1, 1, spell_in * 0.5), spell_opacity),
		.scale.both = 3 - 2 * (1 - pow(1 - spell_in, 3)) + 2 * (1 - spell_out),
	});

	Font *font = res_font("standard");

	r_mat_mv_push();
	r_mat_mv_translate(spell_x - spell_spr->w * 0.5 + 10, spell_y + 5 - font_get_metrics(font)->descent, 0);

	TextParams tp = {
		// .pos = { spell_x - spell_spr->w * 0.5 + 10, spell_y + 5 - font_get_metrics(font)->descent },
		.shader = "text_hud",
		.font_ptr = font,
		.color = color_mul_scalar(RGBA(1, 1, 1, spell_in), spell_opacity),
	};

	r_mat_mv_push();
	r_mat_mv_scale(2 - 1 * spell_opacity, 2 - 1 * spell_opacity, 1);
	text_draw(plr->mode->spellcard_name, &tp);
	r_mat_mv_pop();

	r_mat_mv_pop();
	r_state_pop();
}

static void ent_draw_player(EntityInterface *ent) {
	Player *plr = ENT_CAST(ent, Player);

	if(plr->deathtime >= global.frames) {
		return;
	}

	ShaderCustomParams shader_params = { 1.0f };
	ShaderProgram *shader = res_shader("sprite_particle");

	if(plr->focus_circle_alpha) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = res_sprite("fairy_circle"),
			.shader_ptr = shader,
			.shader_params = &shader_params,
			.rotation.angle = DEG2RAD * global.frames * 10,
			.color = RGBA_MUL_ALPHA(1, 1, 1, 0.2 * plr->focus_circle_alpha),
			.pos = { re(plr->pos), im(plr->pos) },
		});
	}

	Color c;

	if(!player_is_vulnerable(plr)) {
		float f = 0.3*sin(0.1*global.frames);
		c = *RGBA_MUL_ALPHA(1.0+f, 1.0, 1.0-f, 0.7+f);
	} else {
		c = *RGBA_MUL_ALPHA(1.0, 1.0, 1.0, 1.0);
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.shader_ptr = shader,
		.shader_params = &shader_params,
		.pos.as_cmplx = plr->pos,
		.color = &c,
	});
}

static void player_draw_indicators(EntityInterface *ent) {
	PlayerIndicators *indicators = ENT_CAST(ent, PlayerIndicators);
	Player *plr = indicators->plr;

	float focus_opacity = indicators->focus_alpha;
	int t = global.frames - indicators->focus_time;
	cmplxf pos = plr->pos;

	if(focus_opacity > 0) {
		float trans_frames = 12;
		float trans_factor = 1.0f - min(trans_frames, t) / trans_frames;
		float rot_speed = DEG2RAD * global.frames * (1.0f + 3.0f * trans_factor);
		float scale = 1.0f + trans_factor;

		SpriteParams sp = {
			.sprite_ptr = indicators->sprites.focus,
			.rotation.angle = rot_speed,
			.color = RGBA_MUL_ALPHA(1, 1, 1, focus_opacity),
			.pos.as_cmplx = pos,
			.scale.both = scale,
		};

		r_draw_sprite(&sp);
		sp.rotation.angle *= -1;
		r_draw_sprite(&sp);
	}

	float ps_opacity = 1.0;
	float ps_fill_factor = 1.0;

	if(player_is_powersurge_active(plr)) {
		ps_opacity *= clamp((global.frames - plr->powersurge.time.activated) / 30.0, 0, 1);
	} else if(plr->powersurge.time.expired == 0) {
		ps_opacity = 0;
	} else {
		ps_opacity *= (1 - clamp((global.frames - plr->powersurge.time.expired) / 40.0, 0, 1));
		ps_fill_factor = pow(ps_opacity, 2);
	}

	if(ps_opacity > 0) {
		r_state_push();
		r_mat_mv_push();
		r_mat_mv_translate(re(pos), im(pos), 0);
		r_mat_mv_scale(140, 140, 0);
		r_shader("healthbar_radial");
		r_uniform_vec4_rgba("borderColor",   RGBA(0.5, 0.5, 0.5, 0.5));
		r_uniform_vec4_rgba("glowColor",     RGBA(0.5, 0.5, 0.5, 0.75));
		r_uniform_vec4_rgba("fillColor",     RGBA(1.5, 0.5, 0.0, 0.75));
		r_uniform_vec4_rgba("altFillColor",  RGBA(0.0, 0.5, 1.5, 0.75));
		r_uniform_vec4_rgba("coreFillColor", RGBA(0.8, 0.8, 0.8, 0.25));
		r_uniform_vec2("fill", plr->powersurge.positive * ps_fill_factor, plr->powersurge.negative * ps_fill_factor);
		r_uniform_float("opacity", ps_opacity);
		r_draw_quad();
		r_mat_mv_pop();
		r_state_pop();

		char buf[64];
		format_huge_num(0, plr->powersurge.bonus.baseline, sizeof(buf), buf);
		Font *fnt = res_font("monotiny");

		float x = re(pos);
		float y = im(pos) + 80;

		float text_opacity = ps_opacity * 0.75;

		x += text_draw(buf, &(TextParams) {
			.shader = "text_hud",
			.font_ptr = fnt,
			.pos = { x, y },
			.color = RGBA_MUL_ALPHA(1.0, 1.0, 1.0, text_opacity),
			.align = ALIGN_CENTER,
		});

		snprintf(buf, sizeof(buf), " +%u", plr->powersurge.bonus.gain_rate);

		text_draw(buf, &(TextParams) {
			.shader = "text_hud",
			.font_ptr = fnt,
			.pos = { x, y },
			.color = RGBA_MUL_ALPHA(0.3, 0.6, 1.0, text_opacity),
			.align = ALIGN_LEFT,
		});

		r_shader("sprite_filled_circle");
		r_uniform_vec4("color_inner", 0, 0, 0, 0);
		r_uniform_vec4("color_outer", 0, 0, 0.1 * ps_opacity, 0.1 * ps_opacity);
	}
}

DEFINE_TASK(player_indicators) {
	PlayerIndicators *indicators = TASK_HOST_ENT(PlayerIndicators);
	indicators->plr = TASK_BIND(ARGS.plr);
	indicators->ent.draw_layer = LAYER_PLAYER_FOCUS;
	indicators->ent.draw_func = player_draw_indicators;
	indicators->sprites.focus = res_sprite("focus");

	Player *plr = indicators->plr;

	bool was_focused = false;
	bool is_focused = false;

	for(;;) {
		is_focused = (plr->inputflags & INFLAG_FOCUS) && player_is_alive(plr);

		if(is_focused && !was_focused) {
			indicators->focus_time = global.frames;
			indicators->focus_alpha = min(indicators->focus_alpha, 0.1f);
		}

		was_focused = is_focused;
		fapproach_p(&indicators->focus_alpha, is_focused, 1.0f/30.0f);

		YIELD;
	}
}

static void player_fail_spell(Player *plr) {
	Boss *boss = global.boss;

	if(!boss || global.stage->type == STAGE_SPELL) {
		return;
	}

	Attack *atk = boss->current;

	if(!atk || !attack_is_active(atk) || attack_was_failed(atk)) {
		return;
	}

	global.boss->current->failtime = global.frames;

	if(global.boss->current->type == AT_ExtraSpell) {
		boss_finish_current_attack(global.boss);
	}
}

bool player_should_shoot(Player *plr) {
	return
		(plr->inputflags & INFLAG_SHOT) &&
		!dialog_is_active(global.dialog) &&
		player_is_alive(&global.plr);
}

void player_placeholder_bomb_logic(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	DamageInfo dmg;
	dmg.amount = 100;
	dmg.type = DMG_PLAYER_BOMB;

	for(Enemy *en = global.enemies.first; en; en = en->next) {
		ent_damage(&en->ent, &dmg);
	}

	if(global.boss) {
		ent_damage(&global.boss->ent, &dmg);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);
}

static void player_powersurge_expired(Player *plr);

static void _powersurge_trail_draw(Projectile *p, float t, float cmul) {
	float nt = t / p->timeout;
	float s = 1 + (2 + 0.5 * psin((t+global.frames*1.23)/5.0)) * nt * nt;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.scale.both = s,
		.pos = { re(p->pos), im(p->pos) },
		.color = color_mul_scalar(RGBA(0.8, 0.1 + 0.2 * psin((t+global.frames)/5.0), 0.1, 0.0), 0.5 * (1 - nt) * cmul),
		.shader_params = &(ShaderCustomParams){{ -2 * nt * nt }},
		.shader_ptr = p->shader,
	});
}

static void powersurge_trail_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	if(t > 0) {
		_powersurge_trail_draw(p, t - 0.5, 0.25);
		_powersurge_trail_draw(p, t, 0.25);
	} else {
		_powersurge_trail_draw(p, t, 0.5);
	}
}

TASK(powersurge_player_particles, { BoxedPlayer plr; }) {
	Player *plr = TASK_BIND(ARGS.plr);

	DECLARE_ENT_ARRAY(Projectile, trails, 32);
	DECLARE_ENT_ARRAY(Projectile, fields, 4);

	ShaderProgram *trail_shader = res_shader("sprite_silhouette");
	Sprite *field_sprite = res_sprite("part/powersurge_field");

	for(int t = 0; player_is_powersurge_active(plr); ++t, YIELD) {
		ENT_ARRAY_COMPACT(&trails);
		ENT_ARRAY_ADD(&trails, PARTICLE(
			.sprite_ptr = aniplayer_get_frame(&plr->ani),
			.shader_ptr = trail_shader,
			.pos = plr->pos,
			.color = RGBA(1, 1, 1, 0.5),
			.draw_rule = powersurge_trail_draw,
			.move = move_towards(0, plr->pos, 0),
			.timeout = 15,
			.layer = LAYER_PARTICLE_HIGH,
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
		));

		if(t % 6 == 0 && plr->powersurge.bonus.discharge_range > 0) {
			real scale = 2 * plr->powersurge.bonus.discharge_range / field_sprite->w;
			real angle = rng_angle();
			Color *color = color_mul_scalar(rng_bool() ? RGBA(1.5, 0.5, 0.0, 0.1) : RGBA(0.0, 0.5, 1.5, 0.1), 0.25);

			ENT_ARRAY_COMPACT(&fields);

			ENT_ARRAY_ADD(&fields, PARTICLE(
				.sprite_ptr = field_sprite,
				.pos = plr->pos,
				.color = color,
				.draw_rule = pdraw_timeout_fade(1, 0),
				.timeout = 14,
				.angle = angle,
				.layer = LAYER_PLAYER - 1,
				.flags = PFLAG_NOREFLECT | PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE,
				.scale = scale,
			));

			ENT_ARRAY_ADD(&fields, PARTICLE(
				.sprite_ptr = field_sprite,
				.pos = plr->pos,
				.color = RGBA(0.5, 0.5, 0.5, 0),
				.draw_rule = pdraw_timeout_fade(1, 0),
				.timeout = 3,
				.angle = angle,
				.layer = LAYER_PLAYER - 1,
				.flags = PFLAG_NOREFLECT | PFLAG_NOMOVE,
				.scale = scale,
			));
		}

		ENT_ARRAY_FOREACH(&trails, Projectile *p, {
			p->move.attraction_point = plr->pos;
			p->move.attraction = 1 - (global.frames - p->birthtime) / p->timeout;
		});

		ENT_ARRAY_FOREACH(&fields, Projectile *p, {
			p->pos = plr->pos;
		});
	}
}

void player_powersurge_calc_bonus(Player *plr, PowerSurgeBonus *b) {
	b->gain_rate = round(1000 * plr->powersurge.negative * plr->powersurge.negative);
	b->baseline = plr->powersurge.total_charge + plr->powersurge.damage_done * 0.4;
	b->score = b->baseline;
	b->discharge_power = sqrtf(0.2 * b->baseline + 1024 * log1pf(b->baseline)) * smoothstep(0, 1, 0.0001 * b->baseline);
	b->discharge_range = 1.2 * b->discharge_power;
	b->discharge_damage = 10 * pow(b->discharge_power, 1.1);
}

static void player_powersurge_logic(Player *plr) {
	if(dialog_is_active(global.dialog)) {
		return;
	}

	plr->powersurge.positive = max(0, plr->powersurge.positive - lerp(PLR_POWERSURGE_POSITIVE_DRAIN_MIN, PLR_POWERSURGE_POSITIVE_DRAIN_MAX, plr->powersurge.positive));
	plr->powersurge.negative = max(0, plr->powersurge.negative - lerp(PLR_POWERSURGE_NEGATIVE_DRAIN_MIN, PLR_POWERSURGE_NEGATIVE_DRAIN_MAX, plr->powersurge.negative));

	if(stage_is_cleared()) {
		player_cancel_powersurge(plr);
		return;
	}

	if(plr->powersurge.positive <= plr->powersurge.negative) {
		player_powersurge_expired(plr);
		return;
	}

	player_powersurge_calc_bonus(plr, &plr->powersurge.bonus);

	plr->powersurge.total_charge += plr->powersurge.bonus.gain_rate;
}

DEFINE_TASK(player_logic) {
	Player *plr = TASK_BIND(ARGS.plr);
	uint prev_inputflags = 0;

	plr->_prev_effective_power = player_get_effective_power(plr);

	for(;;) {
		YIELD;

		if(prev_inputflags != plr->inputflags) {
			coevent_signal(&plr->events.inputflags_changed);
			prev_inputflags = plr->inputflags;
		}

		fapproach_p(&plr->bomb_cutin_alpha, 0, 1/200.0);

		if(
			plr->respawntime - PLR_RESPAWN_TIME/2 == global.frames &&
			plr->lives < 0 && global.replay.input.replay == NULL
		) {
			stage_gameover();
		}

		if(plr->continuetime == global.frames) {
			plr->lives = PLR_START_LIVES;
			plr->bombs = PLR_START_BOMBS;
			plr->point_item_value = PLR_START_PIV;
			plr->life_fragments = 0;
			plr->bomb_fragments = 0;
			stats_track_continue_used(&plr->stats);
			player_set_power(plr, 0);
			stage_clear_hazards(CLEAR_HAZARDS_ALL);
			spawn_items(plr->deathpos, ITEM_POWER, (int)ceil(PLR_MAX_POWER_EFFECTIVE/(real)POWER_VALUE));
		}

		aniplayer_update(&plr->ani);

		if(plr->lives < 0) {
			continue;
		}

		if(plr->respawntime > global.frames) {
			double x = PLR_SPAWN_POS_X;
			double y = lerp(PLR_SPAWN_POS_Y, VIEWPORT_H + 64, smoothstep(0.0, 1.0, (plr->respawntime - global.frames) / (double)PLR_RESPAWN_TIME));
			plr->pos = CMPLX(x, y);
			stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
			continue;
		}

		if(player_is_powersurge_active(plr)) {
			player_powersurge_logic(plr);
		}

		fapproach_p(&plr->focus_circle_alpha, !!(plr->inputflags & INFLAG_FOCUS), 1.0f/15.0f);

		if(player_should_shoot(plr)) {
			coevent_signal(&plr->events.shoot);
		}

		if(global.frames == plr->deathtime) {
			player_realdeath(plr);
		} else if(plr->deathtime > global.frames) {
			stage_clear_hazards(CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		}

		player_track_effective_power_change(plr);
	}
}

static bool player_can_bomb(Player *plr) {
	return (
		!player_is_bomb_active(plr)
		&& (
			plr->bombs > 0 ||
			plr->iddqd
		)
		&& global.frames >= plr->respawntime
	);

}

static bool player_bomb(Player *plr) {
	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell)
		return false;

	int bomb_time = floor(player_property(plr, PLR_PROP_BOMB_TIME));

	if(bomb_time <= 0) {
		return false;
	}

	if(player_can_bomb(plr)) {
		stats_track_bomb_used(&plr->stats);
		player_fail_spell(plr);
		// player_cancel_powersurge(plr);
		// stage_clear_hazards(CLEAR_HAZARDS_ALL);

		plr->bombs--;

		if(plr->deathtime >= global.frames) {
			// death bomb - unkill the player!
			plr->deathtime = -1;

			if(plr->bombs) {
				plr->bombs--;
			}
		}

		if(plr->bombs < 0) {
			plr->bombs = 0;
		}

		plr->bomb_triggertime = global.frames;
		plr->bomb_endtime = plr->bomb_triggertime + bomb_time;
		plr->bomb_cutin_alpha = 1;

		assert(player_is_alive(plr));
		collect_all_items(1);

		coevent_signal(&plr->events.bomb_used);
		return true;
	}

	return false;
}

static bool player_powersurge(Player *plr) {
	if(
		!player_is_alive(plr) ||
		player_is_bomb_active(plr) ||
		player_is_powersurge_active(plr) ||
		plr->power_stored < PLR_POWERSURGE_POWERCOST
	) {
		return false;
	}

	plr->powersurge.positive = 1.0;
	plr->powersurge.negative = 0.0;
	plr->powersurge.time.activated = global.frames;
	plr->powersurge.total_charge = 0;
	plr->powersurge.damage_accum = 0;
	plr->powersurge.player_power = plr->power_stored;
	player_powersurge_calc_bonus(plr, &plr->powersurge.bonus);
	player_add_power(plr, -PLR_POWERSURGE_POWERCOST);

	play_sfx("powersurge_start");

	collect_all_items(1);
	stagetext_add(_("Power Surge!"), plr->pos - 64 * I, ALIGN_CENTER, res_font("standard"), RGBA(0.75, 0.75, 0.75, 0.75), 0, 45, 10, 20);

	INVOKE_TASK(powersurge_player_particles, ENT_BOX(plr));

	return true;
}

bool player_is_recovering(Player *plr) {
	return global.frames < plr->recoverytime;
}

bool player_is_powersurge_active(Player *plr) {
	return plr->powersurge.positive > plr->powersurge.negative;
}

bool player_is_bomb_active(Player *plr) {
	return global.frames < plr->bomb_endtime;
}

bool player_is_vulnerable(Player *plr) {
	return
		player_is_alive(plr) &&
		!player_is_recovering(plr) &&
		!player_is_bomb_active(plr) &&
		!plr->iddqd;
}

bool player_is_alive(Player *plr) {
	return plr->deathtime < global.frames && global.frames >= plr->respawntime;
}

static void powersurge_distortion_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	if(config_get_int(CONFIG_POSTPROCESS) < 1) {
		return;
	}

	double radius = args[0].as_float[0] * pow(1 - t / p->timeout, 8) * (2 * t / 10.0);

	Framebuffer *fb_aux = stage_get_fbpair(FBPAIR_FG_AUX)->front;
	Framebuffer *fb_main = r_framebuffer_current();

	r_framebuffer(fb_aux);
	r_shader("circle_distort");
	r_uniform_vec3("distortOriginRadius", re(p->pos), VIEWPORT_H - im(p->pos), radius);
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_blend(BLEND_NONE);
	draw_framebuffer_tex(fb_main, VIEWPORT_W, VIEWPORT_H);

	r_framebuffer(fb_main);
	r_shader_standard();
	r_color4(1.0, 0.9, 0.8, 1.0);
	r_blend(BLEND_PREMUL_ALPHA);
	stage_draw_begin_noshake();
	draw_framebuffer_tex(fb_aux, VIEWPORT_W, VIEWPORT_H);
	stage_draw_end_noshake();
}

TASK(powersurge_discharge_clearhazards, { cmplx pos; real range; int timeout; }) {
	for(int t = 0; t < ARGS.timeout; ++t, YIELD) {
		stage_clear_hazards_at(ARGS.pos, ARGS.range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW | CLEAR_HAZARDS_SPAWN_VOLTAGE);
	}
}

static void player_powersurge_expired(Player *plr) {
	plr->powersurge.time.expired = global.frames;

	PowerSurgeBonus bonus;
	player_powersurge_calc_bonus(plr, &bonus);

	Sprite *blast = res_sprite("part/blast_huge_halo");
	float scale = 2 * bonus.discharge_range / blast->w;

	play_sfx("powersurge_end");

	PARTICLE(
		.size = 1+I,
		.pos = plr->pos,
		.timeout = 60,
		.draw_rule = {
			powersurge_distortion_draw,
			.args[0].as_float = { bonus.discharge_range },
		},
		.layer = LAYER_PLAYER,
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_NOREFLECT,
	);

	PARTICLE(
		.sprite_ptr = blast,
		.pos = plr->pos,
		.color = RGBA(0.6, 1.0, 4.4, 0.0),
		.draw_rule = pdraw_timeout_scalefade(2, 0, 1, 0),
		.timeout = 20,
		.angle = rng_angle(),
		.scale = scale,
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_NOREFLECT,
	);

	player_add_points(&global.plr, bonus.score, plr->pos);
	ent_area_damage(plr->pos, bonus.discharge_range, &(DamageInfo) { bonus.discharge_damage, DMG_PLAYER_DISCHARGE }, NULL, NULL);

	INVOKE_TASK(powersurge_discharge_clearhazards, plr->pos, bonus.discharge_range, 10);

	log_debug(
		"Power Surge expired at %i (duration: %i); baseline = %u; score = %u; discharge = %g, dmg = %g, range = %g",
		plr->powersurge.time.expired,
		plr->powersurge.time.expired - plr->powersurge.time.activated,
		bonus.baseline,
		bonus.score,
		bonus.discharge_power,
		bonus.discharge_damage,
		bonus.discharge_range
	);

	plr->powersurge.damage_done = 0;
}

void player_cancel_powersurge(Player *plr) {
	if(player_is_powersurge_active(plr)) {
		plr->powersurge.positive = plr->powersurge.negative;
		player_powersurge_expired(plr);
	}
}

void player_extend_powersurge(Player *plr, float pos, float neg) {
	if(player_is_powersurge_active(plr)) {
		plr->powersurge.positive = clamp(plr->powersurge.positive + pos, 0, 1);
		plr->powersurge.negative = clamp(plr->powersurge.negative + neg, 0, 1);

		if(plr->powersurge.positive <= plr->powersurge.negative) {
			player_powersurge_expired(plr);
		}
	}
}

double player_get_bomb_progress(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return 1;
	}

	int begin = plr->bomb_triggertime;
	int end = plr->bomb_endtime;
	assert(begin <= end);
	real bomb_time = end - begin;
	return (bomb_time - (end - global.frames)) / bomb_time;
}

void player_realdeath(Player *plr) {
	int deathtime = plr->deathtime;
	assert(global.frames == deathtime);

	plr->respawntime = deathtime + PLR_RESPAWN_TIME;
	plr->recoverytime = deathtime + PLR_RECOVERY_TIME;
	plr->inputflags &= ~INFLAGS_MOVE;
	plr->deathpos = plr->pos;
	plr->pos = VIEWPORT_W/2 + VIEWPORT_H*I+30.0*I;
	stage_clear_hazards(CLEAR_HAZARDS_ALL);
	player_fail_spell(plr);

	if(global.stage->type != STAGE_SPELL && global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		// deaths in extra spells "don't count"
		return;
	}

	int total_power = plr->power_stored;

	int drop = max(2, (total_power * 0.15) / POWER_VALUE);
	spawn_items(plr->deathpos, ITEM_POWER, drop);

	player_set_power(plr, total_power * 0.7);
	plr->bombs = PLR_START_BOMBS;
	plr->bomb_fragments = 0;
	plr->voltage *= 0.9;
	plr->lives--;
	stats_track_life_used(&plr->stats);
}

static void player_death_effect_draw_overlay(Projectile *p, int t, ProjDrawRuleArgs args) {
	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);
	r_framebuffer(framebuffers->front);
	r_uniform_sampler("noise_tex", "static");
	r_uniform_int("frames", global.frames);
	r_uniform_float("progress", t / p->timeout);
	r_uniform_vec2("origin", re(p->pos), VIEWPORT_H - im(p->pos));
	r_uniform_vec2("clear_origin", re(global.plr.pos), VIEWPORT_H - im(global.plr.pos));
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_float("size", hypotf(VIEWPORT_W, VIEWPORT_H));
	draw_framebuffer_tex(framebuffers->back, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(framebuffers);

	// This change must propagate, hence the r_state salsa. Yes, pop then push, I know what I'm doing.
	r_state_pop();
	r_framebuffer(framebuffers->back);
	r_state_push();
}

static void player_death_effect_draw_sprite(Projectile *p, int t, ProjDrawRuleArgs args) {
	float s = t / p->timeout;
	float stretch_range = 3, sx, sy;

	s = glm_ease_quad_in(s);

	sx = (1 - pow(2 * pow(1 - s, 4) - 1, 4));
	sx = lerp(1 + (stretch_range - 1) * sx, stretch_range * sx, s);
	sy = 1 + 2 * (stretch_range - 1) * pow(s, 4);

	if(sx <= 0 || sy <= 0) {
		return;
	}

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	sp.scale.x *= sx;
	sp.scale.y *= sy;
	sp.rotation.angle = 0;
	r_draw_sprite(&sp);
}

TASK(player_death_blastspam, { cmplx pos; }) {
	for(int i = 0; i < 12; ++i) {
		RNG_ARRAY(R, 4);
		PARTICLE(
			.proto = pp_blast,
			.pos = ARGS.pos + vrng_range(R[0], 2, 3) * vrng_dir(R[1]),
			.color = RGBA(0.15, 0.2, 0.5, 0),
			.timeout = i + vrng_range(R[2], 10, 14),
			.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
			.angle = vrng_angle(R[3]),
			.flags = PFLAG_NOREFLECT,
			.layer = LAYER_OVERLAY,
		);
	}
}

void player_death(Player *plr) {
	if(!player_is_vulnerable(plr) || stage_is_cleared()) {
		return;
	}

	play_sfx("death");

	for(int i = 0; i < 60; i++) {
		RNG_ARRAY(R, 2);
		PARTICLE(
			.sprite = "flare",
			.pos = plr->pos,
			.timeout = 40,
			.draw_rule = pdraw_timeout_scale(2, 0.01),
			.move = move_linear(vrng_range(R[0], 3, 10) * vrng_dir(R[1])),
			.flags = PFLAG_NOREFLECT,
		);
	}

	stage_clear_hazards(CLEAR_HAZARDS_ALL);

	PARTICLE(
		.sprite = "blast",
		.pos = plr->pos,
		.color = RGBA(0.5, 0.15, 0.15, 0),
		.timeout = 35,
		.draw_rule = pdraw_timeout_scalefade(0, 3.4, 1, 0),
		.angle = rng_angle(),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
	);

	PARTICLE(
		.pos = plr->pos,
		.size = 1+I,
		.timeout = 90,
		.draw_rule = player_death_effect_draw_overlay,
		.blend = BLEND_NONE,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_OVERLAY,
		.shader = "player_death",
	);

	Projectile *p = PARTICLE(
		.sprite_ptr = aniplayer_get_frame(&plr->ani),
		.pos = plr->pos,
		.timeout = 38,
		.draw_rule = player_death_effect_draw_sprite,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.layer = LAYER_PLAYER_FOCUS, // LAYER_OVERLAY | 1,
	);

	INVOKE_TASK_AFTER(&p->events.killed, player_death_blastspam, p->pos);

	plr->deathtime = global.frames + floor(player_property(plr, PLR_PROP_DEATHBOMB_WINDOW));

	if(player_is_powersurge_active(plr)) {
		player_cancel_powersurge(plr);
		// player_bomb(plr);
	}
}

static DamageResult ent_damage_player(EntityInterface *ent, const DamageInfo *dmg) {
	Player *plr = ENT_CAST(ent, Player);

	if(
		!player_is_vulnerable(plr) ||
		(dmg->type != DMG_ENEMY_SHOT && dmg->type != DMG_ENEMY_COLLISION)
	) {
		return DMG_RESULT_IMMUNE;
	}

	player_death(plr);
	return DMG_RESULT_OK;
}

void player_damage_hook(Player *plr, EntityInterface *target, DamageInfo *dmg) {
	if(player_is_powersurge_active(plr) && dmg->type == DMG_PLAYER_SHOT) {
		dmg->amount *= 1.2;
	}
}

static PlrInputFlag key_to_inflag(KeyIndex key) {
	switch(key) {
		case KEY_UP:    return INFLAG_UP;
		case KEY_DOWN:  return INFLAG_DOWN;
		case KEY_LEFT:  return INFLAG_LEFT;
		case KEY_RIGHT: return INFLAG_RIGHT;
		case KEY_FOCUS: return INFLAG_FOCUS;
		case KEY_SHOT:  return INFLAG_SHOT;
		case KEY_SKIP:  return INFLAG_SKIP;
		default:        return 0;
	}
}

static bool player_updateinputflags(Player *plr, PlrInputFlag flags) {
	if(flags == plr->inputflags) {
		return false;
	}

	plr->inputflags = flags;
	return true;
}

static bool player_updateinputflags_moveonly(Player *plr, PlrInputFlag flags) {
	return player_updateinputflags(plr, (flags & INFLAGS_MOVE) | (plr->inputflags & ~INFLAGS_MOVE));
}

static bool player_setinputflag(Player *plr, KeyIndex key, bool mode) {
	PlrInputFlag newflags = plr->inputflags;
	PlrInputFlag keyflag = key_to_inflag(key);

	if(!keyflag) {
		return false;
	}

	if(mode) {
		newflags |= keyflag;
	} else {
		newflags &= ~keyflag;
	}

	return player_updateinputflags(plr, newflags);
}

static bool player_set_axis(int *aptr, uint16_t value) {
	int16_t new = (int16_t)value;

	if(*aptr == new) {
		return false;
	}

	*aptr = new;
	return true;
}

PlayerEventResult player_event(
	Player *plr,
	ReplayState *rpy_in,
	ReplayState *rpy_out,
	ReplayEventCode type,
	uint16_t value
) {
	bool useful = true;
	bool cheat = false;

	switch(type) {
		case EV_PRESS:
			if(dialog_is_active(global.dialog) && (value == KEY_SHOT || value == KEY_BOMB)) {
				useful = dialog_page(global.dialog);
				break;
			}

			switch(value) {
				case KEY_BOMB:
					useful = player_bomb(plr);
					break;

				case KEY_SPECIAL:
					if(dialog_is_active(global.dialog)) {
						useful = false;
						break;
					}

					useful = player_powersurge(plr);

					if(!useful/* && plr->iddqd*/) {
						player_cancel_powersurge(plr);
						useful = true;
					}

					break;

				case KEY_IDDQD:
					plr->iddqd = !plr->iddqd;
					cheat = true;
					break;

				case KEY_POWERUP:
					useful = player_add_power(plr,  100);
					cheat = true;
					break;

				case KEY_POWERDOWN:
					useful = player_add_power(plr, -100);
					cheat = true;
					break;

				default:
					useful = player_setinputflag(plr, value, true);
					break;
			}
			break;

		case EV_RELEASE:
			useful = player_setinputflag(plr, value, false);
			break;

		case EV_AXIS_LR:
			useful = player_set_axis(&plr->axis_lr, value);
			break;

		case EV_AXIS_UD:
			useful = player_set_axis(&plr->axis_ud, value);
			break;

		case EV_INFLAGS:
			useful = player_updateinputflags(plr, value);
			break;

		case EV_CONTINUE:
			// continuing in the same frame will desync the replay,
			// so schedule it for the next one
			plr->continuetime = global.frames + 1;
			useful = true;
			break;

		default:
			log_warn("Can not handle event: [%i:%02x:%04x]", global.frames, type, value);
			useful = false;
			break;
	}

	if(rpy_in && rpy_in->stage) {
		assert(rpy_in->mode == REPLAY_PLAY);

		if(!useful) {
			log_warn("Replay input: useless event: [%i:%02x:%04x]", global.frames, type, value);
		}

		if(cheat) {
			log_warn("Replay input: Cheat event: [%i:%02x:%04x]", global.frames, type, value);

			if( !(rpy_in->replay->flags   & REPLAY_GFLAG_CHEATS) ||
				!(rpy_in->stage->flags    & REPLAY_SFLAG_CHEATS)) {
				log_warn("...but this replay was NOT properly cheat-flagged! Not cool, not cool at all");
			}
		}

		if(type == EV_CONTINUE && (
			!(rpy_in->replay->flags   & REPLAY_GFLAG_CONTINUES) ||
			!(rpy_in->stage->flags    & REPLAY_SFLAG_CONTINUES))) {
			log_warn("Replay input: Continue event [%i:%02x:%04x], but this replay was not properly continue-flagged", global.frames, type, value);
		}
	}

	if(rpy_out && rpy_out->stage) {
		assert(rpy_out->mode == REPLAY_RECORD);

		if(useful) {
			replay_stage_event(rpy_out->stage, global.frames, type, value);

			if(type == EV_CONTINUE) {
				rpy_out->replay->flags |= REPLAY_GFLAG_CONTINUES;
				rpy_out->stage->flags  |= REPLAY_SFLAG_CONTINUES;
			}

			if(cheat) {
				rpy_out->replay->flags |= REPLAY_GFLAG_CHEATS;
				rpy_out->stage->flags  |= REPLAY_SFLAG_CHEATS;
			}
		} else {
			log_debug("Replay output: Useless event discarded: [%i:%02x:%04x]", global.frames, type, value);
		}
	}

	PlayerEventResult res = 0;

	if(useful) {
		res |= PLREVT_USEFUL;
	}

	if(cheat) {
		res |= PLREVT_CHEAT;
	}

	return res;
}

// free-axis movement
static bool player_applymovement_gamepad(Player *plr) {
	if(!plr->axis_lr && !plr->axis_ud) {
		if(plr->gamepadmove) {
			plr->gamepadmove = false;
			plr->inputflags &= ~INFLAGS_MOVE;
		}
		return false;
	}

	cmplx direction = (
		gamepad_normalize_axis_value(plr->axis_lr) +
		gamepad_normalize_axis_value(plr->axis_ud) * I
	);

	if(cabs(direction) > 1) {
		direction /= cabs(direction);
	}

	int sr = sign(re(direction));
	int si = sign(im(direction));

	player_updateinputflags_moveonly(plr,
		(INFLAG_UP    * (si == -1)) |
		(INFLAG_DOWN  * (si ==  1)) |
		(INFLAG_LEFT  * (sr == -1)) |
		(INFLAG_RIGHT * (sr ==  1))
	);

	if(direction) {
		plr->gamepadmove = true;
		player_move(&global.plr, direction);
	}

	return true;
}

static const char *moveseqname(int dir) {
	switch(dir) {
	case -1: return "left";
	case 0: return "main";
	case 1: return "right";
	default: log_fatal("Invalid player animation dir given");
	}
}

static void player_ani_moving(Player *plr, int dir) {
	if(plr->lastmovesequence == dir)
		return;
	const char *seqname = moveseqname(dir);
	const char *lastseqname = moveseqname(plr->lastmovesequence);

	char *transition = strjoin(lastseqname,"2",seqname,NULL);

	aniplayer_hard_switch(&plr->ani,transition,1);
	aniplayer_queue(&plr->ani,seqname,0);
	plr->lastmovesequence = dir;
	mem_free(transition);
}

void player_applymovement(Player *plr) {
	plr->velocity = 0;
	plr->uncapped_velocity = 0;

	if(!player_is_alive(plr))
		return;

	bool gamepad = player_applymovement_gamepad(plr);

	int up      =   plr->inputflags & INFLAG_UP;
	int down    =   plr->inputflags & INFLAG_DOWN;
	int left    =   plr->inputflags & INFLAG_LEFT;
	int right   =   plr->inputflags & INFLAG_RIGHT;

	if(left && !right) {
		player_ani_moving(plr,-1);
	} else if(right && !left) {
		player_ani_moving(plr,1);
	} else {
		player_ani_moving(plr,0);
	}

	if(gamepad) {
		return;
	}

	cmplx direction = 0;

	if(up)      direction -= 1.0*I;
	if(down)    direction += 1.0*I;
	if(left)    direction -= 1.0;
	if(right)   direction += 1.0;

	if(cabs(direction))
		direction /= cabs(direction);

	if(direction)
		player_move(&global.plr, direction);
}

void player_fix_input(Player *plr, ReplayState *rpy_out) {
	// correct input state to account for any events we might have missed,
	// usually because the pause menu ate them up

	PlrInputFlag newflags = plr->inputflags;
	bool invert_shot = config_get_int(CONFIG_SHOT_INVERTED);

	for(KeyIndex key = KEYIDX_FIRST; key <= KEYIDX_LAST; ++key) {
		int flag = key_to_inflag(key);

		if(flag && !(plr->gamepadmove && (flag & INFLAGS_MOVE))) {
			bool flagset = plr->inputflags & flag;
			bool keyheld = gamekeypressed(key);

			if(invert_shot && key == KEY_SHOT) {
				keyheld = !keyheld;
			}

			if(flagset && !keyheld) {
				newflags &= ~flag;
			} else if(!flagset && keyheld) {
				newflags |= flag;
			}
		}
	}

	if(newflags != plr->inputflags) {
		player_event(plr, NULL, rpy_out, EV_INFLAGS, newflags);
	}

	int axis_lr, axis_ud;
	gamepad_get_player_analog_input(&axis_lr, &axis_ud);

	if(plr->axis_lr != axis_lr) {
		player_event(plr, NULL, rpy_out, EV_AXIS_LR, axis_lr);
	}

	if(plr->axis_ud != axis_ud) {
		player_event(plr, NULL, rpy_out, EV_AXIS_UD, axis_ud);
	}

	if(
		config_get_int(CONFIG_AUTO_SURGE) &&
		plr->power_stored == PLR_MAX_POWER_STORED &&
		!player_is_powersurge_active(plr) &&
		!dialog_is_active(global.dialog)
	) {
		player_event(plr, NULL, rpy_out, EV_PRESS, KEY_SPECIAL);
	}
}

void player_graze(Player *plr, cmplx pos, int pts, int effect_intensity, const Color *color) {
	if(++plr->graze >= PLR_MAX_GRAZE) {
		log_debug("Graze counter overflow");
		plr->graze = PLR_MAX_GRAZE;
	}

	pos = (pos + plr->pos) * 0.5;

	player_add_points(plr, pts, pos);
	play_sfx("graze");

	Color *c = COLOR_COPY(color);
	color_add(c, RGBA(1, 1, 1, 1));
	color_mul_scalar(c, 0.5);
	c->a = 0;

	for(int i = 0; i < effect_intensity; ++i) {
		RNG_ARRAY(R, 4);
		PARTICLE(
			.sprite = "graze",
			.color = c,
			.pos = pos,
			.draw_rule = pdraw_timeout_scalefade_exp(1, 0, 1, 0, 2),
			.move = move_asymptotic_simple(0.2 * vrng_range(R[0], 1, 6) * vrng_dir(R[1]), 16 * (1 + 0.5 * vrng_sreal(R[3]))),
			.timeout = vrng_range(R[2], 4, 29),
			.flags = PFLAG_NOREFLECT,
			// .layer = LAYER_PARTICLE_LOW,
		);

		color_mul_scalar(c, 0.4);
	}

	spawn_items(pos, ITEM_POWER_MINI, 1);
}

static void player_add_fragments(Player *plr, int frags, int *pwhole, int *pfrags, int maxfrags, int maxwhole, const char *fragsnd, const char *upsnd, int score_per_excess) {
	int total_frags = *pfrags + maxfrags * *pwhole;
	int excess_frags = total_frags + frags - maxwhole * maxfrags;

	if(excess_frags > 0) {
		player_add_points(plr, excess_frags * score_per_excess, plr->pos);
		frags -= excess_frags;
	}

	*pfrags += frags;
	int up = *pfrags / maxfrags;

	*pwhole += up;
	*pfrags %= maxfrags;

	if(up) {
		play_sfx(upsnd);
	}

	if(frags) {
		// FIXME: when we have the extra life/bomb sounds,
		//        don't play this if upsnd was just played.
		play_sfx(fragsnd);
	}

	if(*pwhole >= maxwhole) {
		*pwhole = maxwhole;
		*pfrags = 0;
	}
}

void player_add_life_fragments(Player *plr, int frags) {
	if(global.stage->type == STAGE_SPELL) {
		return;
	}

	player_add_fragments(
		plr,
		frags,
		&plr->lives,
		&plr->life_fragments,
		PLR_MAX_LIFE_FRAGMENTS,
		PLR_MAX_LIVES,
		"item_generic", // FIXME: replacement needed
		"extra_life",
		(plr->point_item_value * 10) / PLR_MAX_LIFE_FRAGMENTS
	);
}

void player_add_bomb_fragments(Player *plr, int frags) {
	if(global.stage->type == STAGE_SPELL) {
		return;
	}

	player_add_fragments(
		plr,
		frags,
		&plr->bombs,
		&plr->bomb_fragments,
		PLR_MAX_BOMB_FRAGMENTS,
		PLR_MAX_BOMBS,
		"item_generic",  // FIXME: replacement needed
		"extra_bomb",
		(plr->point_item_value * 5) / PLR_MAX_BOMB_FRAGMENTS
	);
}

void player_add_lives(Player *plr, int lives) {
	player_add_life_fragments(plr, PLR_MAX_LIFE_FRAGMENTS * lives);
}

void player_add_bombs(Player *plr, int bombs) {
	player_add_bomb_fragments(plr, PLR_MAX_BOMB_FRAGMENTS * bombs);
}

static void scoretext_update(StageText *txt, int t, float a) {
	double r = bits_to_double((uintptr_t)txt->custom.data1);
	txt->pos -= I * cexp(I*r) * a;
}

#define SCORETEXT_PIV_BIT ((uintptr_t)1 << ((sizeof(uintptr_t) * 8) - 1))

static StageText *find_scoretext_combination_candidate(cmplx pos, bool is_piv) {
	for(StageText *stxt = stagetext_list_head(); stxt; stxt = stxt->next) {
		if(
			stxt->custom.update == scoretext_update &&
			stxt->time.spawn > global.frames &&
			(bool)((uintptr_t)(stxt->custom.data2) & SCORETEXT_PIV_BIT) == is_piv &&
			cabs(pos - stxt->pos) < 32
		) {
			return stxt;
		}
	}

	return NULL;
}

static void add_score_text(Player *plr, cmplx location, uint points, bool is_piv) {
	double rnd = rng_f64s();

	StageText *stxt = find_scoretext_combination_candidate(location, is_piv);

	if(stxt) {
		points += ((uintptr_t)stxt->custom.data2 & ~SCORETEXT_PIV_BIT);
	}

	float importance;
	float a;
	Color c;

	struct {
		int delay, lifetime, fadeintime, fadeouttime;
	} timings;

	timings.delay = 6;
	timings.fadeintime = 10;
	timings.fadeouttime = 20;

	if(is_piv) {
		importance = sqrt(min(points/500.0, 1));
		a = lerp(0.4, 1.0, importance);
		c = *color_lerp(RGB(0.5, 0.8, 1.0), RGB(1.0, 0.3, 1.0), importance);
		timings.lifetime = 35 + 10 * importance;
	} else {
		importance = clamp(0.25 * (double)points / (double)plr->point_item_value, 0, 1);
		a = clamp(0.5 + 0.5 * cbrtf(importance), 0, 1);
		c = *color_lerp(RGB(1.0, 0.8, 0.4), RGB(0.4, 1.0, 0.3), importance);
		timings.lifetime = 25 + 20 * importance;
	}

	a *= config_get_float(CONFIG_SCORETEXT_ALPHA);
	color_mul_scalar(&c, a);

	if(!stxt) {
		if(c.a < 1e-4) {
			return;
		}

		stxt = stagetext_add(
			NULL, location, ALIGN_CENTER, res_font("small"), &c,
			timings.delay, timings.lifetime, timings.fadeintime, timings.fadeouttime
		);

		stxt->custom.data1 = (void*)(uintptr_t)double_to_bits(rnd);
		stxt->custom.update = scoretext_update;
	} else {
		stxt->color = c;
	}

	format_huge_num(0, points, sizeof(stxt->text), stxt->text);
	stxt->custom.data2 = (void*)(uintptr_t)(points | (SCORETEXT_PIV_BIT * is_piv));

	if(is_piv) {
		strlcat(stxt->text, _(" PIV"), sizeof(stxt->text));
	}
}

void player_add_points(Player *plr, uint points, cmplx location) {
	plr->points += points;

	while(plr->points >= plr->extralife_threshold) {
		plr->extralife_threshold = player_next_extralife_threshold(++plr->extralives_given);
		player_add_lives(plr, 1);
	}

	add_score_text(plr, location, points, false);
}

void player_add_piv(Player *plr, uint piv, cmplx location) {
	uint v = plr->point_item_value + piv;

	if(v > PLR_MAX_PIV || v < plr->point_item_value) {
		plr->point_item_value = PLR_MAX_PIV;
	} else {
		plr->point_item_value = v;
	}

	add_score_text(plr, location, piv, true);
}

void player_add_voltage(Player *plr, uint voltage) {
	uint v = plr->voltage + voltage;

	if(v > PLR_MAX_VOLTAGE || v < plr->voltage) {
		plr->voltage = PLR_MAX_VOLTAGE;
	} else {
		plr->voltage = v;
	}

	player_add_bomb_fragments(plr, voltage);
}

bool player_drain_voltage(Player *plr, uint voltage) {
	// TODO: animate (or maybe handle that at stagedraw level)

	if(plr->voltage >= voltage) {
		plr->voltage -= voltage;
		return true;
	}

	plr->voltage = 0;
	return false;
}

void player_register_damage(Player *plr, EntityInterface *target, const DamageInfo *damage) {
	if(!DAMAGETYPE_IS_PLAYER(damage->type)) {
		return;
	}

	cmplx pos = NAN;

	if(target != NULL) {
		switch(target->type) {
			case ENT_TYPE_ID(Enemy): {
				pos = ENT_CAST(target, Enemy)->pos;
				player_add_points(&global.plr, damage->amount * 0.5, pos);
				break;
			}

			case ENT_TYPE_ID(Boss): {
				pos = ENT_CAST(target, Boss)->pos;
				player_add_points(&global.plr, damage->amount * 0.2, pos);
				break;
			}

			default: break;
		}
	}

	if(!isnan(re(pos)) && damage->type == DMG_PLAYER_DISCHARGE) {
		double rate = NOT_NULL(target)->type == ENT_TYPE_ID(Boss) ? 110 : 256;
		spawn_and_collect_items(pos, 1, ITEM_VOLTAGE, (int)(damage->amount / rate));
	}

	if(player_is_powersurge_active(plr)) {
		plr->powersurge.damage_done += damage->amount;
		plr->powersurge.damage_accum += damage->amount;

		if(!isnan(re(pos))) {
			double rate = 500;

			while(plr->powersurge.damage_accum > rate) {
				plr->powersurge.damage_accum -= rate;
				spawn_item(pos, ITEM_SURGE);
			}
		}
	}

#ifdef PLR_DPS_STATS
	while(global.frames > plr->dmglogframe) {
		memmove(plr->dmglog + 1, plr->dmglog, sizeof(plr->dmglog) - sizeof(*plr->dmglog));
		plr->dmglog[0] = 0;
		plr->dmglogframe++;
	}

	plr->dmglog[0] += damage->amount;
#endif
}

uint64_t player_next_extralife_threshold(uint64_t step) {
	static uint64_t base = 5000000;
	return base * (step * step + step + 2) / 2;
}

void player_preload(ResourceGroup *rg) {
	const int flags = RESF_DEFAULT;

	res_group_preload(rg, RES_SHADER_PROGRAM, flags,
		"circle_distort",
		"player_death",
	NULL);

	res_group_preload(rg, RES_TEXTURE, flags,
		"static",
	NULL);

	res_group_preload(rg, RES_SPRITE, flags,
		"fairy_circle",
		"focus",
		"part/blast_huge_halo",
		"part/powersurge_field",
	NULL);

	res_group_preload(rg, RES_SFX, flags | RESF_OPTIONAL,
		"death",
		"extra_bomb",
		"extra_life",
		"generic_shot",
		"graze",
		"hit0",
		"hit1",
		"powerup",
		"powersurge_start",
		"powersurge_end",
	NULL);
}

// FIXME: where should this be?

cmplx plrutil_homing_target(cmplx org, cmplx fallback) {
	double mindst = INFINITY;
	cmplx target = fallback;

	if(global.boss && boss_is_vulnerable(global.boss)) {
		target = global.boss->pos;
		mindst = cabs(target - org);
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(!enemy_is_targetable(e)) {
			continue;
		}

		double dst = cabs(e->pos - org);

		if(dst < mindst) {
			mindst = dst;
			target = e->pos;
		}
	}

	return target;
}

void plrutil_slave_retract(BoxedPlayer bplr, cmplx *pos, real retract_time) {
	cmplx pos0 = *pos;
	Player *plr;

	for(int i = 1; i <= retract_time; ++i) {
		YIELD;
		plr = NOT_NULL(ENT_UNBOX(bplr));
		*pos = clerp(pos0, plr->pos, i / retract_time);
	}
}
