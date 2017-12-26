/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "stagedraw.h"
#include "stagetext.h"
#include "video.h"

#ifdef DEBUG
	#define GRAPHS_DEFAULT 1
	#define OBJPOOLSTATS_DEFAULT 1
#else
	#define GRAPHS_DEFAULT 0
	#define OBJPOOLSTATS_DEFAULT 0
#endif

static struct {
	struct {
		Shader *shader;
		uint32_t u_colorAtop;
		uint32_t u_colorAbot;
		uint32_t u_colorBtop;
		uint32_t u_colorBbot;
		uint32_t u_colortint;
		uint32_t u_split;
	} hud_text;
	bool framerate_graphs;
	bool objpool_stats;
} stagedraw;

void stage_draw_preload(void) {
	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"hud",
		"star",
		"titletransition",
	NULL);

	preload_resources(RES_SHADER, RESF_PERMANENT,
		"stagetitle",
		"ingame_menu",
		"circleclipped_indicator",
		"hud_text",
	NULL);

	stagedraw.hud_text.shader      = get_shader("hud_text");
	stagedraw.hud_text.u_colorAtop = uniloc(stagedraw.hud_text.shader, "colorAtop");
	stagedraw.hud_text.u_colorAbot = uniloc(stagedraw.hud_text.shader, "colorAbot");
	stagedraw.hud_text.u_colorBtop = uniloc(stagedraw.hud_text.shader, "colorBtop");
	stagedraw.hud_text.u_colorBbot = uniloc(stagedraw.hud_text.shader, "colorBbot");
	stagedraw.hud_text.u_colortint = uniloc(stagedraw.hud_text.shader, "colortint");
	stagedraw.hud_text.u_split     = uniloc(stagedraw.hud_text.shader, "split");

	glUseProgram(stagedraw.hud_text.shader->prog);
	glUniform4f(stagedraw.hud_text.u_colorAtop, 0.70, 0.70, 0.70, 0.70);
	glUniform4f(stagedraw.hud_text.u_colorAbot, 0.50, 0.50, 0.50, 0.50);
	glUniform4f(stagedraw.hud_text.u_colorBtop, 1.00, 1.00, 1.00, 1.00);
	glUniform4f(stagedraw.hud_text.u_colorBbot, 0.80, 0.80, 0.80, 0.80);
	glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 1.00, 1.00, 1.00);
	glUseProgram(0);

	stagedraw.framerate_graphs = getenvint("TAISEI_FRAMERATE_GRAPHS", GRAPHS_DEFAULT);
	stagedraw.objpool_stats = getenvint("TAISEI_OBJPOOL_STATS", OBJPOOLSTATS_DEFAULT);

	if(stagedraw.framerate_graphs) {
		preload_resources(RES_SHADER, RESF_PERMANENT,
			"graph",
		NULL);
	}
}

static void apply_shader_rules(ShaderRule *shaderrules, FBO **fbo0, FBO **fbo1) {
	if(!shaderrules) {
		return;
	}

	for(ShaderRule *rule = shaderrules; *rule; ++rule) {
		glBindFramebuffer(GL_FRAMEBUFFER, (*fbo1)->fbo);
		(*rule)(*fbo0);
		swap_fbos(fbo0, fbo1);
	}

	return;
}

static void draw_wall_of_text(float f, const char *txt) {
	fontrenderer_draw(&resources.fontren, txt,_fonts.standard);
	Texture *tex = &resources.fontren.tex;
	int strw = tex->w;
	int strh = tex->h;

	float w = VIEWPORT_W;
	float h = VIEWPORT_H;

	glPushMatrix();
	glTranslatef(w/2, h/2, 0);
	glScalef(w, h, 1.0);

	Shader *shader = get_shader("spellcard_walloftext");
	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "w"), strw/(float)tex->truew);
	glUniform1f(uniloc(shader, "h"), strh/(float)tex->trueh);
	glUniform1f(uniloc(shader, "ratio"), h/w);
	glUniform2f(uniloc(shader, "origin"), creal(global.boss->pos)/h, cimag(global.boss->pos)/w);
	glUniform1f(uniloc(shader, "t"), f);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	draw_quad();
	glUseProgram(0);

	glPopMatrix();
}

static void draw_spellbg(int t) {
	glPushMatrix();

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

	float delay = ATTACK_START_DELAY;
	if(b->current->type == AT_ExtraSpell)
		delay = ATTACK_START_DELAY_EXTRA;
	float f = (-t+ATTACK_START_DELAY)/(delay+ATTACK_START_DELAY);
	if(f > 0)
		draw_wall_of_text(f, b->current->name);

	if(t < ATTACK_START_DELAY && b->dialog) {
		glPushMatrix();
		float f = -0.5*t/(float)ATTACK_START_DELAY+0.5;
		glColor4f(1,1,1,-f*f+2*f);
		draw_texture_p(VIEWPORT_W*3/4-10*f*f,VIEWPORT_H*2/3-10*f*f,b->dialog);
		glColor4f(1,1,1,1);
		glPopMatrix();
	}

	glPopMatrix();
}

static void apply_bg_shaders(ShaderRule *shaderrules, FBO **fbo0, FBO **fbo1) {
	Boss *b = global.boss;
	if(b && b->current && b->current->draw_rule) {
		int t = global.frames - b->current->starttime;

		FBO *fbo0_orig = *fbo0;
		if(t < 4*ATTACK_START_DELAY || b->current->endtime) {
			apply_shader_rules(shaderrules, fbo0, fbo1);
		}

		if(*fbo0 == fbo0_orig) {
			glBindFramebuffer(GL_FRAMEBUFFER, (*fbo1)->fbo);
			draw_fbo_viewport(*fbo0);
			swap_fbos(fbo0, fbo1);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, (*fbo1)->fbo);
		draw_spellbg(t);

		complex pos = b->pos;
		float ratio = (float)VIEWPORT_H/VIEWPORT_W;

		glBindFramebuffer(GL_FRAMEBUFFER, (*fbo0)->fbo);
		if(t<ATTACK_START_DELAY) {
			Shader *shader = get_shader("spellcard_intro");
			glUseProgram(shader->prog);

			glUniform1f(uniloc(shader, "ratio"), ratio);
			glUniform2f(uniloc(shader, "origin"), creal(pos)/VIEWPORT_W, 1-cimag(pos)/VIEWPORT_H);

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

			if(boss_is_dying(b)) {
				delay = BOSS_DEATH_DELAY;
			} else if(b->current->type == AT_ExtraSpell) {
				delay = ATTACK_END_DELAY_EXTRA;
			}

			glUniform1f(uniloc(shader, "ratio"), ratio);
			glUniform2f(uniloc(shader, "origin"), creal(pos)/VIEWPORT_W, 1-cimag(pos)/VIEWPORT_H);

			glUniform1f(uniloc(shader, "t"), max(0,tn/delay+1));

		} else {
			glUseProgram(0);
		}

		draw_fbo_viewport(*fbo1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	} else {
		apply_shader_rules(shaderrules, fbo0, fbo1);
	}
}

static void apply_zoom_shader(void) {
	Shader *shader = get_shader("boss_zoom");
	glUseProgram(shader->prog);

	complex fpos = global.boss->pos;
	complex pos = fpos + 15*cexp(I*global.frames/4.5);

	glUniform2f(uniloc(shader, "blur_orig"),
			creal(pos)/VIEWPORT_W, 1-cimag(pos)/VIEWPORT_H);
	glUniform2f(uniloc(shader, "fix_orig"),
			creal(fpos)/VIEWPORT_W, 1-cimag(fpos)/VIEWPORT_H);

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

	glUniform1f(uniloc(shader, "blur_rad"), 1.5*spellcard_sup*(0.2+0.025*sin(global.frames/15.0)));
	glUniform1f(uniloc(shader, "rad"), 0.24);
	glUniform1f(uniloc(shader, "ratio"), (float)VIEWPORT_H/VIEWPORT_W);

	if(global.boss->zoomcolor) {
		static float clr[4];
		parse_color_array(global.boss->zoomcolor, clr);
		glUniform4fv(uniloc(shader, "color"), 1, clr);
	} else {
		glUniform4f(uniloc(shader, "color"), 0.1, 0.2, 0.3, 1);
	}
}

static FBO* stage_render_bg(StageInfo *stage) {
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.bg[0].fbo);
	float scale = resources.fbo.bg[0].scale;
	glViewport(0, 0, scale*VIEWPORT_W, scale*VIEWPORT_H);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
		glTranslatef(-(VIEWPORT_X+VIEWPORT_W/2), -(VIEWPORT_Y+VIEWPORT_H/2),0);
		glEnable(GL_DEPTH_TEST);
		stage->procs->draw();
	glPopMatrix();

	FBO *fbo0 = resources.fbo.bg;
	FBO *fbo1 = resources.fbo.bg + 1;

	apply_bg_shaders(stage->procs->shader_rules, &fbo0, &fbo1);
	return fbo0;
}

static void stage_draw_objects(void) {
	if(global.boss) {
		draw_boss_background(global.boss);
	}

	player_draw(&global.plr);

	draw_items();
	draw_projectiles(global.projs, NULL);
	draw_projectiles(global.particles, NULL);
	draw_lasers(true);
	draw_enemies(global.enemies);
	draw_lasers(false);

	if(global.boss) {
		draw_boss(global.boss);
	}

	if(global.dialog) {
		draw_dialog(global.dialog);
	}

	stagetext_draw();
}

static void postprocess_prepare(FBO *fbo, Shader *s) {
	glUniform1i(uniloc(s, "frames"), global.frames);
}

void stage_draw_foreground(void) {
	int vw, vh;
	video_get_viewport_size(&vw,&vh);

	// CAUTION: Very intricate pixel perfect scaling that will ruin your day.
	float facw = (float)vw/SCREEN_W;
	float fach = (float)vh/SCREEN_H;
	// confer video_update_quality to understand why this is fach. fach is equal to facw up to roundoff error.
	float scale = fach;

	// draw the foreground FBO
	glPushMatrix();
		glScalef(1/facw,1/fach,1);
		glTranslatef(floorf(facw*VIEWPORT_X), floorf(fach*VIEWPORT_Y), 0);
		glScalef(floorf(scale*VIEWPORT_W)/VIEWPORT_W,floorf(scale*VIEWPORT_H)/VIEWPORT_H,1);
		// apply the screenshake effect
		if(global.shake_view) {
			glTranslatef(global.shake_view*sin(global.frames),global.shake_view*sin(global.frames*1.1+3),0);
			glScalef(1+2*global.shake_view/VIEWPORT_W,1+2*global.shake_view/VIEWPORT_H,1);
			glTranslatef(-global.shake_view,-global.shake_view,0);

			if(global.shake_view_fade) {
				global.shake_view -= global.shake_view_fade;
				if(global.shake_view <= 0)
					global.shake_view = global.shake_view_fade = 0;
			}
		}
		draw_fbo(&resources.fbo.fg[0]);
	glPopMatrix();
	set_ortho();
}

void stage_draw_scene(StageInfo *stage) {
#ifdef DEBUG
	bool key_nobg = gamekeypressed(KEY_NOBACKGROUND);
#else
	bool key_nobg = false;
#endif

	bool draw_bg = !config_get_int(CONFIG_NO_STAGEBG) && !key_nobg;
	FBO *fbg = NULL;

	if(draw_bg) {
		// render the 3D background
		fbg = stage_render_bg(stage);
	}

	// switch to foreground FBO
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.fg[0].fbo);
	float scale = resources.fbo.fg[0].scale;
	glViewport(0, 0, scale*VIEWPORT_W, scale*VIEWPORT_H);
	set_ortho_ex(VIEWPORT_W,VIEWPORT_H);

	if(draw_bg) {
		// enable boss background distortion
		if(global.boss) {
			apply_zoom_shader();
		}

		// draw the 3D background
		draw_fbo(fbg);

		// disable boss background distortion
		glUseProgram(0);

		// draw bomb background
		if(global.frames - global.plr.recovery < 0) {
			global.plr.mode->procs.bombbg(&global.plr);
		}
	} else if(!key_nobg) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// draw the 2D objects
	set_ortho_ex(VIEWPORT_W,VIEWPORT_H);
	stage_draw_objects();

	// apply postprocessing shaders
	FBO *fbo0 = resources.fbo.fg, *fbo1 = resources.fbo.fg+1;

	// stage postprocessing
	apply_shader_rules(global.stage->procs->postprocess_rules, &fbo0, &fbo1);

	// bomb effects shader if present and player bombing
	if(global.frames - global.plr.recovery < 0 && global.plr.mode->procs.bomb_shader) {
		ShaderRule rules[] = {global.plr.mode->procs.bomb_shader,0};
		apply_shader_rules(rules,&fbo0,&fbo1);
	}
	// custom postprocessing
	postprocess(
		resources.stage_postprocess,
		&fbo0,
		&fbo1,
		postprocess_prepare,
		draw_fbo_viewport
	);

	// update the primary foreground FBO if needed
	if(fbo0 != resources.fbo.fg) {
		glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.fg[0].fbo);
		draw_fbo_viewport(fbo0);
	}

	// switch to main framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	video_set_viewport();
	set_ortho();

	// finally, draw stuff to the actual screen
	stage_draw_foreground();
	stage_draw_hud();
}

static void draw_star(int x, int y, float fill, float alpha) {
	Texture *star = get_tex("star");
	Shader *shader = get_shader("circleclipped_indicator");

	y -= 2;
	float clr[4];

	Color amul = rgba(alpha, alpha, alpha, alpha);
	Color fill_clr = multiply_colors(rgba(1.0f, 1.0f, 1.0f, 1.0f), amul);
	Color back_clr = multiply_colors(rgba(0.2f, 0.6f, 1.0f, 0.2f), amul);

	if(fill < 1) {
		fill_clr = mix_colors(derive_color(back_clr, CLRMASK_A, alpha), fill_clr, 0.35f);
	}

	if(fill >= 1 || fill <= 0) {
		parse_color_call(fill > 0 ? fill_clr : back_clr, glColor4f);
		draw_texture_with_size_p(x, y, 20, 20, star);
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
	draw_texture_with_size_p(x, y, 20, 20, star);
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

static inline void stage_draw_hud_power_value(float ypos, char *buf, size_t bufsize) {
	glUniform1f(stagedraw.hud_text.u_split, -0.25);
	snprintf(buf, bufsize, "%i.%02i", global.plr.power / 100, global.plr.power % 100);
	draw_text(AL_Right, 170, (int)ypos, buf, _fonts.mono);
	glUniform1f(stagedraw.hud_text.u_split, 0.0);
}

static float split_for_digits(uint32_t val, int maxdigits) {
	int digits = val ? log10(val) + 1 : 0;
	int remdigits = maxdigits - digits;
	return max(0, (float)remdigits/maxdigits);
}

static void stage_draw_hud_score(Alignment a, float xpos, float ypos, char *buf, size_t bufsize, uint32_t score) {
	snprintf(buf, bufsize, "%010u", score);
	glUniform1f(stagedraw.hud_text.u_split, split_for_digits(score, 10));
	draw_text(a, (int)xpos, (int)ypos, buf, _fonts.mono);
}

static void stage_draw_hud_scores(float ypos_hiscore, float ypos_score, char *buf, size_t bufsize) {
	stage_draw_hud_score(AL_Right, 170, (int)ypos_hiscore, buf, bufsize, progress.hiscore);
	stage_draw_hud_score(AL_Right, 170, (int)ypos_score,   buf, bufsize, global.plr.points);
	glUniform1f(stagedraw.hud_text.u_split, 0.0);
}

static void stage_draw_hud_objpool_stats(float x, float y, float width, Font *font) {
	ObjectPool **last = &stage_object_pools.first + (sizeof(StageObjectPools)/sizeof(ObjectPool*) - 1);

	for(ObjectPool **pool = &stage_object_pools.first; pool <= last; ++pool) {
		ObjectPoolStats stats;
		char buf[32];
		objpool_get_stats(*pool, &stats);

		snprintf(buf, sizeof(buf), "%zu | %5zu", stats.usage, stats.peak_usage);
		draw_text(AL_Left  | AL_Flag_NoAdjust, (int)x,           (int)y, stats.tag, font);
		draw_text(AL_Right | AL_Flag_NoAdjust, (int)(x + width), (int)y, buf,       font);

		y += stringheight(buf, font) * 1.1;
	}
}

struct labels_s {
	struct {
		float ofs;
	} x;

	struct {
		float mono_ofs;
		float hiscore;
		float score;
		float lives;
		float bombs;
		float power;
		float graze;
	} y;
};

void stage_draw_hud_text(struct labels_s* labels) {
	char buf[64];

	glUseProgram(stagedraw.hud_text.shader->prog);
	glUniform1f(stagedraw.hud_text.u_split, 0.0);
	glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 1.00, 1.00, 1.00);

	// Labels
	glUniform4f(stagedraw.hud_text.u_colortint, 0.70, 0.70, 0.70, 0.70);
	draw_text(AL_Left, labels->x.ofs, labels->y.hiscore, "Hi-Score:", _fonts.hud);
	draw_text(AL_Left, labels->x.ofs, labels->y.score,   "Score:",    _fonts.hud);
	draw_text(AL_Left, labels->x.ofs, labels->y.lives,   "Lives:",    _fonts.hud);
	draw_text(AL_Left, labels->x.ofs, labels->y.bombs,   "Bombs:",    _fonts.hud);
	draw_text(AL_Left, labels->x.ofs, labels->y.power,   "Power:",    _fonts.hud);
	draw_text(AL_Left, labels->x.ofs, labels->y.graze,   "Graze:",    _fonts.hud);
	glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 1.00, 1.00, 1.00);

	if(stagedraw.objpool_stats) {
		stage_draw_hud_objpool_stats(labels->x.ofs, labels->y.graze + 32, 250, _fonts.monotiny);
	}

	// Score/Hi-Score values
	stage_draw_hud_scores(labels->y.hiscore + labels->y.mono_ofs, labels->y.score + labels->y.mono_ofs, buf, sizeof(buf));

	// Lives and Bombs (N/A)
	if(global.stage->type == STAGE_SPELL) {
		glColor4f(1, 1, 1, 0.7);
		draw_text(AL_Left, -6, labels->y.lives, "N/A", _fonts.hud);
		draw_text(AL_Left, -6, labels->y.bombs, "N/A", _fonts.hud);
		glColor4f(1, 1, 1, 1.0);
	}

	// Power value
	stage_draw_hud_power_value(labels->y.power + labels->y.mono_ofs, buf, sizeof(buf));

	// Graze value
	snprintf(buf, sizeof(buf), "%05i", global.plr.graze);
	glUniform1f(stagedraw.hud_text.u_split, split_for_digits(global.plr.graze, 5));
	draw_text(AL_Left, -6, (int)(labels->y.graze + labels->y.mono_ofs), buf, _fonts.mono);
	glUniform1f(stagedraw.hud_text.u_split, 0.0);

	// Warning: pops outer matrix!
	glPopMatrix();

#ifdef DEBUG
	snprintf(buf, sizeof(buf), "%.2f lfps, %.2f rfps, timer: %d, frames: %d",
		global.fps.logic.fps,
		global.fps.render.fps,
		global.timer,
		global.frames
	);
#else
	if(get_effective_frameskip() > 1) {
		snprintf(buf, sizeof(buf), "%.2f lfps, %.2f rfps",
			global.fps.logic.fps,
			global.fps.render.fps
		);
	} else {
		snprintf(buf, sizeof(buf), "%.2f fps",
			global.fps.logic.fps
		);
	}
#endif

	draw_text(AL_Right | AL_Flag_NoAdjust, SCREEN_W, rint(SCREEN_H - 0.5 * stringheight(buf, _fonts.monosmall)), buf, _fonts.monosmall);

	if(global.replaymode == REPLAY_PLAY) {
		// XXX: does it make sense to use the monospace font here?

		snprintf(buf, sizeof(buf), "Replay: %s (%i fps)", global.replay.playername, global.replay_stage->fps);
		int x = 0, y = SCREEN_H - 0.5 * stringheight(buf, _fonts.monosmall);

		glUniform4f(stagedraw.hud_text.u_colortint, 0.50, 0.50, 0.50, 0.50);
		draw_text(AL_Left, x, y, buf, _fonts.monosmall);
		glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 1.00, 1.00, 1.00);

		if(global.replay_stage->desynced) {
			x += stringwidth(buf, _fonts.monosmall);
			strlcpy(buf, " (DESYNCED)", sizeof(buf));

			glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 0.20, 0.20, 0.60);
			draw_text(AL_Left, x, y, buf, _fonts.monosmall);
			glUniform4f(stagedraw.hud_text.u_colortint, 1.00, 1.00, 1.00, 1.00);
		}
	}
#ifdef PLR_DPS_STATS
	else if(global.frames) {
		snprintf(buf, sizeof(buf), "Avg DPS: %.02f", global.plr.total_dmg / (global.frames / (double)FPS));
		glUniform1f(stagedraw.hud_text.u_split, 8.0 / strlen(buf));
		draw_text(AL_Left, 0, rint(SCREEN_H - 0.5 * stringheight(buf, _fonts.monosmall)), buf, _fonts.monosmall);
	}
#endif

	glUseProgram(0);
}

static void draw_graph(float x, float y, float w, float h) {
	glPushMatrix();
	glTranslatef(x + w/2, y + h/2, 0);
	glScalef(w, h, 1);
	glDisable(GL_TEXTURE_2D);
	draw_quad();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

static void fill_graph(int num_samples, float *samples, FPSCounter *fps) {
	for(int i = 0; i < num_samples; ++i) {
		samples[i] = fps->frametimes[i] / (((hrtime_t)2.0)/FPS);

		if(samples[i] > 1.0) {
			samples[i] = 1.0;
		}
	}
}

static void stage_draw_framerate_graphs(void) {
	#define NUM_SAMPLES (sizeof(((FPSCounter){{0}}).frametimes) / sizeof(((FPSCounter){{0}}).frametimes[0]))
	static float samples[NUM_SAMPLES];

	Shader *s = get_shader("graph");
	uint32_t u_points = uniloc(s, "points[0]");
	uint32_t u_colors[3] = {
		uniloc(s, "color_low"),
		uniloc(s, "color_mid"),
		uniloc(s, "color_high"),
	};

	float pad = 15;

	float w = 260 - pad;
	float h = 30;

	float x = SCREEN_W - w - pad;
	float y = 100;

	glUseProgram(s->prog);

	fill_graph(NUM_SAMPLES, samples, &global.fps.logic);
	glUniform3f(u_colors[0], 0.0, 1.0, 1.0);
	glUniform3f(u_colors[1], 1.0, 1.0, 0.0);
	glUniform3f(u_colors[2], 1.0, 0.0, 0.0);
	glUniform1fv(u_points, NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	// x -= w * 1.1;
	y += h + 1;

	fill_graph(NUM_SAMPLES, samples, &global.fps.busy);
	glUniform3f(u_colors[0], 0.0, 1.0, 0.0);
	glUniform3f(u_colors[1], 1.0, 0.0, 0.0);
	glUniform3f(u_colors[2], 1.0, 0.0, 0.5);
	glUniform1fv(u_points, NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	glUseProgram(0);
}

void stage_draw_hud(void) {
	// Background
	draw_texture(SCREEN_W/2.0, SCREEN_H/2.0, "hud");

	// Set up positions of most HUD elements
	static struct labels_s labels = {
		.x.ofs = -75,

		// XXX: is there a more robust way to level the monospace font with the label one?
		.y.mono_ofs = -0.5,
	};

	const float label_height = 33;
	float label_cur_height = 0;
	int i;

	label_cur_height = 49;  i = 0;
	labels.y.hiscore = label_cur_height+label_height*(i++);
	labels.y.score   = label_cur_height+label_height*(i++);

	label_cur_height = 180; i = 0;
	labels.y.lives   = label_cur_height+label_height*(i++);
	labels.y.bombs   = label_cur_height+label_height*(i++);
	labels.y.power   = label_cur_height+label_height*(i++);
	labels.y.graze   = label_cur_height+label_height*(i++);

	glPushMatrix();
	glTranslatef(615, 0, 0);

	// Difficulty indicator
	glPushMatrix();
	glTranslatef((SCREEN_W - 615) * 0.25, SCREEN_H-170, 0);
	glScalef(0.6, 0.6, 0);
	draw_texture(0, 0, difficulty_tex(global.diff));
	glPopMatrix();

	// Set up variables for Extra Spell indicator
	float a = 1, s = 0, fadein = 1, fadeout = 1, fade = 1;

	if(global.boss && global.boss->current && global.boss->current->type == AT_ExtraSpell) {
		fadein  = min(1, -min(0, global.frames - global.boss->current->starttime) / (float)ATTACK_START_DELAY);
		fadeout = global.boss->current->finished * (1 - (global.boss->current->endtime - global.frames) / (float)ATTACK_END_DELAY_EXTRA) / 0.74;
		fade = max(fadein, fadeout);

		s = 1 - fade;
		a = 0.5 + 0.5 * fade;
	}

	// Lives and Bombs
	if(global.stage->type != STAGE_SPELL) {
		draw_stars(0, labels.y.lives, global.plr.lives, global.plr.life_fragments, PLR_MAX_LIVES, PLR_MAX_LIFE_FRAGMENTS, a);
		draw_stars(0, labels.y.bombs, global.plr.bombs, global.plr.bomb_fragments, PLR_MAX_BOMBS, PLR_MAX_BOMB_FRAGMENTS, a);
	}

	// Power stars
	draw_stars(0, labels.y.power, global.plr.power / 100, global.plr.power % 100, PLR_MAX_POWER / 100, 100, 1);

	// God Mode indicator
	if(global.plr.iddqd) {
		draw_text(AL_Left, -70, 475, "GOD MODE", _fonts.mainmenu);
	}

	// Extra Spell indicator
	if(s) {
		float s2 = max(0, swing(s, 3));
		glPushMatrix();
		glTranslatef((SCREEN_W - 615) * 0.25 - 615 * (1 - pow(2*fadein-1, 2)), 340, 0);
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

	// Warning: pops matrix!
	stage_draw_hud_text(&labels);

	if(stagedraw.framerate_graphs) {
		stage_draw_framerate_graphs();
	}

	// Boss indicator ("Enemy")
	if(global.boss) {
		float red = 0.5*exp(-0.5*(global.frames-global.boss->lastdamageframe)); // hit indicator
		if(red > 1)
			red = 0;
		glColor4f(1,1,1,1-red);
		draw_texture(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");
		glColor4f(1,1,1,1);
	}
}
