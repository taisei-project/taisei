/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "util.h"
#include "stage.h"

#include <time.h>
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "audio.h"
#include "log.h"
#include "stagetext.h"

static size_t numstages = 0;
StageInfo *stages = NULL;

static void add_stage(uint16_t id, StageProcs *procs, StageType type, const char *title, const char *subtitle, AttackInfo *spell, Difficulty diff, Color titleclr, Color bosstitleclr) {
	++numstages;
	stages = realloc(stages, numstages * sizeof(StageInfo));
	StageInfo *stg = stages + (numstages - 1);
	memset(stg, 0, sizeof(StageInfo));

	stg->id = id;
	stg->procs = procs;
	stg->type = type;
	stralloc(&stg->title, title);
	stralloc(&stg->subtitle, subtitle);
	stg->spell = spell;
	stg->difficulty = diff;
	stg->titleclr = titleclr;
	stg->bosstitleclr = titleclr;
}

static void end_stages(void) {
	add_stage(0, NULL, 0, NULL, NULL, NULL, 0, 0, 0);
}

static void add_spellpractice_stages(int *spellnum, bool (*filter)(AttackInfo*), uint16_t spellbits) {
	for(int i = 0 ;; ++i) {
		StageInfo *s = stages + i;

		if(s->type == STAGE_SPELL) {
			break;
		}

		for(AttackInfo *a = s->spell; a->rule; ++a) {
			if(!filter(a)) {
				continue;
			}

			for(Difficulty diff = D_Easy; diff < D_Easy + NUM_SELECTABLE_DIFFICULTIES; ++diff) {
				if(a->idmap[diff - D_Easy] >= 0) {
					uint16_t id = spellbits | a->idmap[diff - D_Easy] | (s->id << 8);

					char *title = strfmt("Spell %d", ++(*spellnum));
					char *subtitle = strjoin(a->name, " ~ ", difficulty_name(diff), NULL);

					add_stage(id, s->procs->spellpractice_procs, STAGE_SPELL, title, subtitle, a, diff, 0, 0);
					s = stages + i; // stages just got realloc'd, so we must update the pointer

					free(title);
					free(subtitle);
				}
			}
		}
	}
}

static bool spellfilter_normal(AttackInfo *spell) {
	return spell->type != AT_ExtraSpell;
}

static bool spellfilter_extra(AttackInfo *spell) {
	return spell->type == AT_ExtraSpell;
}

void stage_init_array(void) {
	int spellnum = 0;

//           id  procs          type         title      subtitle                       spells         diff   titleclr      bosstitleclr
	add_stage(1, &stage1_procs, STAGE_STORY, "Stage 1", "Misty Lake",                  stage1_spells, D_Any, rgb(1, 1, 1), rgb(1, 1, 1));
	add_stage(2, &stage2_procs, STAGE_STORY, "Stage 2", "Walk Along the Border",       stage2_spells, D_Any, rgb(1, 1, 1), rgb(1, 1, 1));
	add_stage(3, &stage3_procs, STAGE_STORY, "Stage 3", "Through the Tunnel of Light", stage3_spells, D_Any, rgb(0, 0, 0), rgb(0, 0, 0));
	add_stage(4, &stage4_procs, STAGE_STORY, "Stage 4", "Forgotten Mansion",           stage4_spells, D_Any, rgb(0, 0, 0), rgb(1, 1, 1));
	add_stage(5, &stage5_procs, STAGE_STORY, "Stage 5", "Climbing the Tower of Babel", stage5_spells, D_Any, rgb(1, 1, 1), rgb(1, 1, 1));
	add_stage(6, &stage6_procs, STAGE_STORY, "Stage 6", "Roof of the World",           stage6_spells, D_Any, rgb(1, 1, 1), rgb(1, 1, 1));

	// generate spellpractice stages
	add_spellpractice_stages(&spellnum, spellfilter_normal, STAGE_SPELL_BIT);
	add_spellpractice_stages(&spellnum, spellfilter_extra, STAGE_SPELL_BIT | STAGE_EXTRASPELL_BIT);

	end_stages();

#ifdef DEBUG
	for(int i = 0; stages[i].procs; ++i) {
		if(stages[i].type == STAGE_SPELL && !(stages[i].id & STAGE_SPELL_BIT)) {
			log_fatal("Spell stage has an ID without the spell bit set: 0x%04x", stages[i].id);
		}

		for(int j = 0; stages[j].procs; ++j) {
			if(i != j && stages[i].id == stages[j].id) {
				log_fatal("Duplicate ID 0x%04x in stages array, indices: %i, %i", stages[i].id, i, j);
			}
		}
	}
#endif
}

void stage_free_array(void) {
	for(StageInfo *stg = stages; stg->procs; ++stg) {
		free(stg->title);
		free(stg->subtitle);
		free(stg->progress);
	}

	free(stages);
}

// NOTE: This returns the stage BY ID, not by the array index!
StageInfo* stage_get(uint16_t n) {
	for(StageInfo *stg = stages; stg->procs; ++stg)
		if(stg->id == n)
			return stg;
	return NULL;
}

StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff) {
	if(!spell)
		return NULL;

	for(StageInfo *stg = stages; stg->procs; ++stg)
		if(stg->spell == spell && stg->difficulty == diff)
			return stg;

	return NULL;
}

StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff, bool allocate) {
	// D_Any stages will have a separate StageProgress for every selectable difficulty.
	// Stages with a fixed difficulty setting (spellpractice, extra stage...) obviously get just one and the diff parameter is ignored.

	// This stuff must stay around until progress_save(), which happens on shutdown.
	// So do NOT try to free any pointers this function returns, that will fuck everything up.

	if(!stage) {
		return NULL;
	}

	bool fixed_diff = (stage->difficulty != D_Any);

	if(!fixed_diff && (diff < D_Easy || diff > D_Lunatic)) {
		return NULL;
	}

	if(!stage->progress) {
		if(!allocate) {
			return NULL;
		}

		size_t allocsize = sizeof(StageProgress) * (fixed_diff ? 1 : NUM_SELECTABLE_DIFFICULTIES);
		stage->progress = malloc(allocsize);
		memset(stage->progress, 0, allocsize);

		log_debug("Allocated %lu bytes for stage %u (%s: %s)", (unsigned long int)allocsize, stage->id, stage->title, stage->subtitle);
	}

	return stage->progress + (fixed_diff ? 0 : diff - D_Easy);
}

StageProgress* stage_get_progress(uint16_t id, Difficulty diff, bool allocate) {
	return stage_get_progress_from_info(stage_get(id), diff, allocate);
}

static void stage_start(StageInfo *stage) {
	global.timer = 0;
	global.frames = 0;
	global.game_over = 0;
	global.shake_view = 0;

	global.fps.stagebg_fps = global.fps.show_fps = FPS;
	global.fps.fpstime = SDL_GetTicks();

	prepare_player_for_next_stage(&global.plr);

	if(stage->type == STAGE_SPELL) {
		global.plr.lives = 0;
		global.plr.bombs = 0;
	}

	reset_sounds();
}

void stage_pause(void) {
	MenuData menu;
	stop_bgm(false);
	create_ingame_menu(&menu);
	menu_loop(&menu);
	resume_bgm();
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.game_over = GAMEOVER_RESTART;
		return;
	}

	MenuData menu;
	create_gameover_menu(&menu);

	bool interrupt_bgm = (global.stage->type != STAGE_SPELL);

	if(interrupt_bgm) {
		save_bgm();
		start_bgm("bgm_gameover");
	}

	menu_loop(&menu);

	if(interrupt_bgm) {
		restore_bgm();
	}
}

void stage_input_event(EventType type, int key, void *arg) {
	switch(type) {
		case E_PlrKeyDown:
			if(key == KEY_NOBACKGROUND) {
				break;
			}

			if(key == KEY_HAHAIWIN) {
#ifdef DEBUG
				stage_finish(GAMEOVER_WIN);
#endif
				break;
			}

			if(global.dialog && (key == KEY_SHOT || key == KEY_BOMB)) {
				page_dialog(&global.dialog);
				replay_stage_event(global.replay_stage, global.frames, EV_PRESS, key);
			} else {
#ifndef DEBUG // no cheating for peasants
				if(global.replaymode == REPLAY_RECORD && key == KEY_IDDQD)
					break;
#endif

				player_event(&global.plr, EV_PRESS, key);
				replay_stage_event(global.replay_stage, global.frames, EV_PRESS, key);

				if(key == KEY_SKIP && global.dialog) {
					global.dialog->skip = true;
				}
			}
			break;

		case E_PlrKeyUp:
			player_event(&global.plr, EV_RELEASE, key);
			replay_stage_event(global.replay_stage, global.frames, EV_RELEASE, key);

			if(key == KEY_SKIP && global.dialog)
				global.dialog->skip = false;
			break;

		case E_Pause:
			stage_pause();
			break;

		case E_PlrAxisLR:
			player_event(&global.plr, EV_AXIS_LR, key);
			replay_stage_event(global.replay_stage, global.frames, EV_AXIS_LR, key);
			break;

		case E_PlrAxisUD:
			player_event(&global.plr, EV_AXIS_UD, key);
			replay_stage_event(global.replay_stage, global.frames, EV_AXIS_UD, key);
			break;

		default: break;
	}
}

void stage_replay_event(EventType type, int state, void *arg) {
	if(type == E_Pause)
		stage_pause();
}

void replay_input(void) {
	ReplayStage *s = global.replay_stage;
	int i;

	handle_events(stage_replay_event, EF_Game, NULL);

	for(i = s->playpos; i < s->numevents; ++i) {
		ReplayEvent *e = s->events + i;

		if(e->frame != global.frames)
			break;

		switch(e->type) {
			case EV_OVER:
				global.game_over = GAMEOVER_DEFEAT;
				break;

			case EV_CHECK_DESYNC:
				s->desync_check = e->value;
				break;

			case EV_FPS:
				s->fps = e->value;
				break;

			default:
				if(global.dialog && e->type == EV_PRESS && (e->value == KEY_SHOT || e->value == KEY_BOMB))
					page_dialog(&global.dialog);
				else if(global.dialog && (e->type == EV_PRESS || e->type == EV_RELEASE) && e->value == KEY_SKIP)
					global.dialog->skip = (e->type == EV_PRESS);
				else
					player_event(&global.plr, e->type, (int16_t)e->value);
				break;
		}
	}

	s->playpos = i;
	player_applymovement(&global.plr);
}

void stage_input(void) {
	handle_events(stage_input_event, EF_Game, NULL);

	// workaround
	if(global.dialog && global.dialog->skip && !gamekeypressed(KEY_SKIP)) {
		global.dialog->skip = false;
		replay_stage_event(global.replay_stage, global.frames, EV_RELEASE, KEY_SKIP);
	}

	player_input_workaround(&global.plr);
	player_applymovement(&global.plr);
}

static void draw_star(int x, int y, float fill, float alpha) {
	Texture *star = get_tex("star");
	Shader *shader = get_shader("circleclipped_indicator");

	float clr[4];

	Color amul = rgba(alpha, alpha, alpha, alpha);
	Color fill_clr = multiply_colors(rgba(1.0f, 1.0f, 1.0f, 1.0f), amul);
	Color back_clr = multiply_colors(rgba(0.2f, 0.6f, 1.0f, 0.2f), amul);

	if(fill < 1) {
		fill_clr = mix_colors(derive_color(back_clr, CLRMASK_A, alpha), fill_clr, 0.35f);
	}

	if(fill >= 1 || fill <= 0) {
		parse_color_call(fill > 0 ? fill_clr : back_clr, glColor4f);
		draw_texture_p(x, y, star);
		glColor4f(1, 1, 1, 1);
		return;
	}

	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "fill"), fill);
	glUniform1f(uniloc(shader, "tcfactor"), star->truew / (float)star->w);
	parse_color_array(fill_clr, clr);
	glUniform4fv(uniloc(shader, "fill_color"), 1, clr);
	parse_color_array(back_clr, clr);
	glUniform4fv(uniloc(shader, "back_color"), 1, clr);
	draw_texture_p(x, y, star);
	glUseProgram(0);
}

static void draw_stars(int x, int y, int numstars, int numfrags, int maxstars, int maxfrags, float alpha) {
	static const int star_width = 20;
	int i = 0;

	while(i < numstars) {
		draw_star(x + star_width * i++, y, 1, alpha);
	}

	if(numfrags) {
		draw_star(x + star_width * i++, y, numfrags / (float)maxfrags, alpha);
	}

	while(i < maxstars) {
		draw_star(x + star_width * i++, y, 0, alpha);
	}
}

void draw_hud(void) {
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");

	char buf[64];

	glPushMatrix();
	glTranslatef(615,0,0);

	glPushMatrix();
	glTranslatef((SCREEN_W - 615) * 0.25, SCREEN_H-170, 0);
	glScalef(0.6, 0.6, 0);

	draw_texture(0,0,difficulty_tex(global.diff));
	glPopMatrix();

	if(global.stage->type == STAGE_SPELL) {
		glColor4f(1, 1, 1, 0.7);
		draw_text(AL_Left, -6, 167, "N/A", _fonts.standard);
		draw_text(AL_Left, -6, 200, "N/A", _fonts.standard);
		glColor4f(1, 1, 1, 1.0);
	} else {
		float a = 1, s = 0, fadein = 1, fadeout = 1, fade = 1;

		if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
			fadein  = min(1, -min(0, global.frames - global.boss->current->starttime) / (float)ATTACK_START_DELAY);
			fadeout = (!!global.boss->current->finished) * (1 - (global.boss->current->endtime - global.frames) / (float)ATTACK_END_DELAY_EXTRA) / 0.74;
			fade = max(fadein, fadeout);

			s = 1 - fade;
			a = 0.5 + 0.5 * fade;
		}

		draw_stars(0, 167, global.plr.lives, global.plr.life_fragments, PLR_MAX_LIVES, PLR_MAX_LIFE_FRAGMENTS, a);
		draw_stars(0, 200, global.plr.bombs, global.plr.bomb_fragments, PLR_MAX_BOMBS, PLR_MAX_BOMB_FRAGMENTS, a);

		if(s) {
			float s2 = max(0, swing(s, 3));
			glPushMatrix();
			glTranslatef((SCREEN_W - 615) * 0.25 - 615 * (1 - pow(2*fadein-1, 2)), 400, 0);
			//glColor4f(1, 0.5, 0.3, 0.7 * s);
			glColor4f(0.3, 0.6, 0.7, 0.7 * s);
			glRotatef(-25 + 360 * (1-s2), 0, 0, 1);
			glScalef(s2, s2, 0);
			draw_text(AL_Center,  1,  1, "Extra Spell!", _fonts.mainmenu);
			draw_text(AL_Center, -1, -1, "Extra Spell!", _fonts.mainmenu);
			glColor4f(1, 1, 1, s);
			draw_text(AL_Center, 0, 0, "Extra Spell!", _fonts.mainmenu);
		glColor4f(1, 1, 1, 1);
			glPopMatrix();
		}
	}

	// snprintf(buf, sizeof(buf), "%.2f", global.plr.power / 100.0);
	// draw_text(AL_Left, -6, 236, buf, _fonts.standard);

	draw_stars(0, 236, global.plr.power / 100, global.plr.power % 100, PLR_MAX_POWER / 100, 100, 1);

	snprintf(buf, sizeof(buf), "%i", global.plr.graze);
	draw_text(AL_Left, -6, 270, buf, _fonts.standard);

	snprintf(buf, sizeof(buf), "%i", global.plr.points);
	draw_text(AL_Left, 8, 49, buf, _fonts.standard);

	snprintf(buf, sizeof(buf), "%i", progress.hiscore);
	draw_text(AL_Left, 8, 83, buf, _fonts.standard);

	if(global.plr.iddqd) {
		draw_text(AL_Left, -70, 475, "GOD MODE", _fonts.mainmenu);
	}

	glPopMatrix();

	snprintf(buf, sizeof(buf), "%i fps", global.fps.show_fps);
	draw_text(AL_Right, SCREEN_W, SCREEN_H - 0.5 * stringheight(buf, _fonts.standard), buf, _fonts.standard);

	if(global.boss)
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");

	if(global.replaymode == REPLAY_PLAY) {
		snprintf(buf, sizeof(buf), "Replay: %s (%i fps)", global.replay.playername, global.replay_stage->fps);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		draw_text(AL_Left, 0, SCREEN_H - 0.5 * stringheight(buf, _fonts.standard), buf, _fonts.standard);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

static void apply_bg_shaders(ShaderRule *shaderrules);
static void display_stage_title(StageInfo *info);

static void postprocess_prepare(FBO *fbo, Shader *s) {
	float w = 1.0f / fbo->nw;
	float h = 1.0f / fbo->nh;

	glUniform1i(uniloc(s, "frames"), global.frames);
	glUniform2f(uniloc(s, "view_ofs"), VIEWPORT_X * w, VIEWPORT_Y * h);
	glUniform2f(uniloc(s, "view_scale"), VIEWPORT_W * w, VIEWPORT_H * h);
}

static void stage_draw(StageInfo *stage) {
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[0].fbo);
	glViewport(0,0,SCREEN_W,SCREEN_H);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2.0), -(VIEWPORT_Y+VIEWPORT_H/2.0),0);
	glEnable(GL_DEPTH_TEST);

	if(!config_get_int(CONFIG_NO_STAGEBG))
		stage->procs->draw();

	glPopMatrix();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	set_ortho();

	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);

	apply_bg_shaders(stage->procs->shader_rules);

	if(global.boss) {
		glPushMatrix();
		glTranslatef(creal(global.boss->pos), cimag(global.boss->pos), 0);

		if(!(global.frames % 5)) {
			complex offset = (frand()-0.5)*50 + (frand()-0.5)*20.0*I;
			create_particle3c("boss_shadow", -20.0*I, rgba(0.2,0.35,0.5,0.5), EnemyFlareShrink, enemy_flare, 50, (-100.0*I-offset)/(50.0+frand()*10), add_ref(global.boss));
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glRotatef(global.frames*4.0, 0, 0, -1);
		float f = 0.8+0.1*sin(global.frames/8.0);
		if(boss_is_dying(global.boss)) {
			float t = (global.frames - global.boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
			f -= t*(t-0.7)/(1-t);
		}

		glScalef(f,f,f);

		draw_texture(0,0,"boss_circle");

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPopMatrix();
	}

	player_draw(&global.plr);

	draw_items();
	draw_projectiles(global.projs);
	draw_projectiles(global.particles);
	draw_lasers(true);
	draw_enemies(global.enemies);
	draw_lasers(false);

	if(global.boss)
		draw_boss(global.boss);

	if(global.dialog)
		draw_dialog(global.dialog);

	stagetext_draw();

	FBO *ppfbo = postprocess(resources.stage_postprocess, &resources.fsec, resources.fbg, postprocess_prepare, draw_fbo_viewport);

	if(ppfbo != &resources.fsec) {
		// ensure that fsec is the most up to date fbo, because the ingame menu derives the background from it.
		// it would be more efficient to somehow pass ppfbo to it and avoid the copy, but this is simpler.
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fsec.fbo);
		draw_fbo_viewport(ppfbo);
		ppfbo = &resources.fsec;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	video_set_viewport();

	glPushMatrix();

	if(global.shake_view) {
		glTranslatef(global.shake_view*sin(global.frames),global.shake_view*sin(global.frames+3),0);
		glScalef(1+2*global.shake_view/VIEWPORT_W,1+2*global.shake_view/VIEWPORT_H,1);
		glTranslatef(-global.shake_view,-global.shake_view,0);

		if(global.shake_view_fade) {
			global.shake_view -= global.shake_view_fade;
			if(global.shake_view <= 0)
				global.shake_view = global.shake_view_fade = 0;
		}
	}

	draw_fbo_viewport(ppfbo);
	glPopMatrix();

	glPopMatrix();
	draw_hud();
}

static int apply_shaderrules(ShaderRule *shaderrules, int fbonum) {
	if(!config_get_int(CONFIG_NO_STAGEBG)) {
		for(ShaderRule *rule = shaderrules; *rule; ++rule) {
			glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[!fbonum].fbo);
			(*rule)(fbonum);
			fbonum = !fbonum;
		}
	}

	return fbonum;
}

static void draw_wall_of_text(float f, const char *txt) {
	fontrenderer_draw(&resources.fontren, txt,_fonts.standard);
	Texture *tex = &resources.fontren.tex;
	int strw = tex->w;
	int strh = tex->h;
	glPushMatrix();
	glTranslatef(VIEWPORT_W/2,VIEWPORT_H/2,0);
	glScalef(VIEWPORT_W,VIEWPORT_H,1.);

	Shader *shader = get_shader("spellcard_walloftext");
	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "w"), strw/(float)tex->truew);
	glUniform1f(uniloc(shader, "h"), strh/(float)tex->trueh);
	glUniform1f(uniloc(shader, "ratio"), (float)VIEWPORT_H/VIEWPORT_W);
	glUniform2f(uniloc(shader, "origin"), creal(global.boss->pos)/VIEWPORT_H,cimag(global.boss->pos)/VIEWPORT_W);
	glUniform1f(uniloc(shader, "t"), f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	draw_quad();
	glUseProgram(0);

	glPopMatrix();


}

static void draw_spellbg(int t) {
	Boss *b = global.boss;
	b->current->draw_rule(b, t);

	if(b->current->type == AT_ExtraSpell)
		draw_extraspell_bg(b, t);

	glPushMatrix();
	glTranslatef(creal(b->pos), cimag(b->pos), 0);
	glRotatef(global.frames*7.0, 0, 0, -1);

	if(t < 0) {
		float f = 1.0 - t/(float)ATTACK_START_DELAY;
		glScalef(f,f,f);
	}

	draw_texture(0,0,"boss_spellcircle0");
	glPopMatrix();

	float delay = 0;
	if(b->current->type == AT_ExtraSpell)
		delay = ATTACK_START_DELAY_EXTRA-ATTACK_START_DELAY;
	float f = -(t+delay)/ATTACK_START_DELAY;
	draw_wall_of_text(f, b->current->name);

	if(t < ATTACK_START_DELAY && b->dialog) {
		glPushMatrix();
		float f = -0.5*t/(float)ATTACK_START_DELAY+0.5;
		glColor4f(1,1,1,-f*f+2*f);
		draw_texture_p(VIEWPORT_W*3/4-10*f*f,VIEWPORT_H*2/3-10*f*f,b->dialog);
		glColor4f(1,1,1,1);
		glPopMatrix();
	}
}

static void apply_zoom_shader() {
	Shader *shader = get_shader("boss_zoom");
	glUseProgram(shader->prog);

	complex fpos = VIEWPORT_H*I + conj(global.boss->pos) + (VIEWPORT_X + VIEWPORT_Y*I);
	complex pos = fpos + 15*cexp(I*global.frames/4.5);

	glUniform2f(uniloc(shader, "blur_orig"),
			creal(pos)/resources.fbg[0].nw, cimag(pos)/resources.fbg[0].nh);
	glUniform2f(uniloc(shader, "fix_orig"),
			creal(fpos)/resources.fbg[0].nw, cimag(fpos)/resources.fbg[0].nh);

	float spellcard_sup = 1;
	// This factor is used to surpress the effect near the start of spell cards.
	// This is necessary so it doesn’t distort the awesome spinning background effect.

	if(global.boss->current && global.boss->current->draw_rule) {
		float t = (global.frames - global.boss->current->starttime + ATTACK_START_DELAY)/(float)ATTACK_START_DELAY;
		spellcard_sup = 1-1/(0.1*t*t+1);
	}

	if(boss_is_dying(global.boss)) {
		float t = (global.frames - global.boss->current->endtime)/(float)BOSS_DEATH_DELAY + 1;
		spellcard_sup = 1-t*t;
	}

	glUniform1f(uniloc(shader, "blur_rad"), spellcard_sup*(0.2+0.025*sin(global.frames/15.0)));
	glUniform1f(uniloc(shader, "rad"), 0.24);
	glUniform1f(uniloc(shader, "ratio"), (float)resources.fbg[0].nh/resources.fbg[0].nw);
	if(global.boss->zoomcolor) {
		static float clr[4];
		parse_color_array(global.boss->zoomcolor, clr);
		glUniform4fv(uniloc(shader, "color"), 1, clr);
	} else {
		glUniform4f(uniloc(shader, "color"), 0.1, 0.2, 0.3, 1);
	}
}

static void apply_bg_shaders(ShaderRule *shaderrules) {
	int fbonum = 0;

	Boss *b = global.boss;
	if(b && b->current && b->current->draw_rule) {
		int t = global.frames - b->current->starttime;
		if(t < 4*ATTACK_START_DELAY || b->current->endtime)
			fbonum = apply_shaderrules(shaderrules, fbonum);

		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[!fbonum].fbo);
		draw_spellbg(t);

		complex pos = VIEWPORT_H*I + conj(b->pos) + (VIEWPORT_X + VIEWPORT_Y*I);
		float ratio = (float)resources.fbg[fbonum].nh/resources.fbg[fbonum].nw;

		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbg[fbonum].fbo);
		if(t<4*ATTACK_START_DELAY) {
			Shader *shader = get_shader("spellcard_intro");
			glUseProgram(shader->prog);
			glUniform1f(uniloc(shader, "ratio"),ratio);
			glUniform2f(uniloc(shader, "origin"),
					creal(pos)/resources.fbg[fbonum].nw, cimag(pos)/resources.fbg[fbonum].nh);
			float delay = ATTACK_START_DELAY;
			if(b->current->type == AT_ExtraSpell)
				delay = ATTACK_START_DELAY_EXTRA;
			float duration = ATTACK_START_DELAY_EXTRA;

			glUniform1f(uniloc(shader, "t"), (t+delay)/duration);
		} else if(b->current->endtime) {
			int tn = global.frames - b->current->endtime;
			Shader *shader = get_shader("spellcard_outro");
			glUseProgram(shader->prog);
			float delay = ATTACK_END_DELAY;
			if(b->current->type == AT_ExtraSpell)
				delay = ATTACK_END_DELAY_EXTRA;

			glUniform1f(uniloc(shader, "ratio"),ratio);
			glUniform2f(uniloc(shader, "origin"),
					creal(pos)/resources.fbg[fbonum].nw, cimag(pos)/resources.fbg[fbonum].nh);

			glUniform1f(uniloc(shader, "t"), max(0,tn/delay+1));

		} else {
			glUseProgram(0);
		}
		draw_fbo_viewport(&resources.fbg[!fbonum]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	} else
		fbonum = apply_shaderrules(shaderrules, fbonum);

	glBindFramebuffer(GL_FRAMEBUFFER, resources.fsec.fbo);

	if(global.boss) { // Boss background shader
		apply_zoom_shader();
	}


#ifdef DEBUG
	if(!gamekeypressed(KEY_NOBACKGROUND))
#endif
	{
		draw_fbo_viewport(&resources.fbg[fbonum]);
	}

	glUseProgram(0);

	if(global.frames - global.plr.recovery < 0) {
		float t = BOMB_RECOVERY - global.plr.recovery + global.frames;
		float fade = 1;

		if(t < BOMB_RECOVERY/6)
			fade = t/BOMB_RECOVERY*6;

		if(t > BOMB_RECOVERY/4*3)
			fade = 1-t/BOMB_RECOVERY*4 + 3;

		glPushMatrix();
		glTranslatef(-30,-30,0);
		fade_out(fade*0.6);
		glPopMatrix();
	}
}

static void stage_logic(void) {
	player_logic(&global.plr);

	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);

	if(global.boss && !global.dialog)
		process_boss(&global.boss);

	if(global.dialog && global.dialog->skip && global.frames - global.dialog->page_time > 3)
		page_dialog(&global.dialog);

	global.frames++;

	if(!global.dialog && !global.boss)
		global.timer++;

	if(global.replaymode == REPLAY_PLAY &&
		global.frames == global.replay_stage->events[global.replay_stage->numevents-1].frame - FADE_TIME &&
		global.game_over != GAMEOVER_TRANSITIONING) {
		stage_finish(GAMEOVER_DEFEAT);
	}

	// BGM handling
	if(global.dialog && global.dialog->messages[global.dialog->pos].side == BGM) {
		start_bgm(global.dialog->messages[global.dialog->pos].msg);
		page_dialog(&global.dialog);
	}
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		delete_dialog(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}

	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.game_over = (intptr_t)arg;
}

void stage_finish(int gameover) {
	assert(global.game_over != GAMEOVER_TRANSITIONING);
	global.game_over = GAMEOVER_TRANSITIONING;
	set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
}

static void stage_preload(void) {
	difficulty_preload();
	projectiles_preload();
	player_preload();
	items_preload();
	boss_preload();

	if(global.stage->type != STAGE_SPELL)
		enemies_preload();

	global.stage->procs->preload();

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"hud",
		"star",
		"titletransition",
	NULL);

	preload_resources(RES_SHADER, RESF_PERMANENT,
		"stagetitle",
		"ingame_menu",
		"circleclipped_indicator",
	NULL);
}

void stage_loop(StageInfo *stage) {
	assert(stage);
	assert(stage->procs);
	assert(stage->procs->preload);
	assert(stage->procs->begin);
	assert(stage->procs->end);
	assert(stage->procs->draw);
	assert(stage->procs->event);
	assert(stage->procs->shader_rules);

	if(global.game_over == GAMEOVER_WIN) {
		global.game_over = 0;
	} else if(global.game_over) {
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	stage_preload();

	uint32_t seed = (uint32_t)time(0);
	tsrand_switch(&global.rand_game);
	tsrand_seed_p(&global.rand_game, seed);
	stage_start(stage);

	if(global.replaymode == REPLAY_RECORD) {
		if(config_get_int(CONFIG_SAVE_RPY)) {
			global.replay_stage = replay_create_stage(&global.replay, stage, seed, global.diff, global.plr.points, &global.plr);

			// make sure our player state is consistent with what goes into the replay
			init_player(&global.plr);
			replay_stage_sync_player_state(global.replay_stage, &global.plr);
		} else {
			global.replay_stage = NULL;
		}

		log_debug("Random seed: %u", seed);
	} else {
		if(!global.replay_stage) {
			log_fatal("Attemped to replay a NULL stage");
			return;
		}

		ReplayStage *stg = global.replay_stage;
		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", stg->numevents, stage_get(stg->stage)->title);

		tsrand_seed_p(&global.rand_game, stg->seed);
		log_debug("Random seed: %u", stg->seed);

		global.diff = stg->diff;
		init_player(&global.plr);
		replay_stage_sync_player_state(stg, &global.plr);
		stg->playpos = 0;
	}

	Enemy *e = global.plr.slaves, *tmp;
	short power = global.plr.power;
	global.plr.power = -1;

	while(e != 0) {
		tmp = e;
		e = e->next;
		delete_enemy(&global.plr.slaves, tmp);
	}

	player_set_power(&global.plr, power);

	stage->procs->begin();

	int transition_delay = 0;

	while(global.game_over <= 0) {
		if(global.game_over != GAMEOVER_TRANSITIONING) {
			if(!global.boss && !global.dialog) {
				stage->procs->event();
			}

			if(stage->type == STAGE_SPELL && !global.boss) {
				stage_finish(GAMEOVER_WIN);
				transition_delay = 60;
			}
		}

		if(!global.timer && stage->type != STAGE_SPELL) {
			// must be done here to let the event function start a BGM first
			display_stage_title(stage);
		}

		((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();
		replay_stage_check_desync(global.replay_stage, global.frames, (tsrand() ^ global.plr.points) & 0xFFFF, global.replaymode);

		stage_logic();

		if(global.replaymode == REPLAY_RECORD && global.plr.points > progress.hiscore) {
			progress.hiscore = global.plr.points;
		}

		if(transition_delay) {
			--transition_delay;
		}

		if(global.frameskip && global.frames % global.frameskip) {
			if(!transition_delay) {
				update_transition();
			}
			continue;
		}

		if(calc_fps(&global.fps) && global.replaymode == REPLAY_RECORD) {
			replay_stage_event(global.replay_stage, global.frames, EV_FPS, global.fps.show_fps);
		}

		tsrand_lock(&global.rand_game);
		tsrand_switch(&global.rand_visual);
		stage_draw(stage);
		tsrand_unlock(&global.rand_game);
		tsrand_switch(&global.rand_game);

		draw_transition();
		if(!transition_delay) {
			update_transition();
		}

		SDL_GL_SwapWindow(video.window);

		if(global.replaymode == REPLAY_PLAY && gamekeypressed(KEY_SKIP)) {
			global.lasttime = SDL_GetTicks();
		} else {
			frame_rate(&global.lasttime);
		}
	}

	if(global.game_over == GAMEOVER_RESTART && global.stage->type != STAGE_SPELL) {
		stop_bgm(true);
	}

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);
	}

	stage->procs->end();
	stage_free();
	tsrand_switch(&global.rand_visual);
	free_all_refs();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), AL_Center, _fonts.mainmenu, info->titleclr, 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    AL_Center, _fonts.standard, info->titleclr, 60, 85, 35, 35);

	if(current_bgm.title && current_bgm.started_at >= 0) {
		stagetext_add(current_bgm.title, VIEWPORT_W-15 + I * (VIEWPORT_H-20), AL_Right, _fonts.standard,
			current_bgm.isboss ? info->bosstitleclr : info->titleclr, 70, 85, 35, 35);
	}
}
