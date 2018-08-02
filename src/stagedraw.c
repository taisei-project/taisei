/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "stagedraw.h"
#include "stagetext.h"
#include "video.h"
#include "resource/postprocess.h"
#include "entity.h"

#ifdef DEBUG
	#define GRAPHS_DEFAULT 1
	#define OBJPOOLSTATS_DEFAULT 1
#else
	#define GRAPHS_DEFAULT 0
	#define OBJPOOLSTATS_DEFAULT 0
#endif

typedef struct CustomFramebuffer {
	LIST_INTERFACE(struct CustomFramebuffer);
	Framebuffer fb;
	float scale;
	StageFBPair scaling_base;
} CustomFramebuffer;

static struct {
	struct {
		ShaderProgram *shader;
		Font *font;

		struct {
			Color active;
			Color inactive;
			Color label;
		} color;
	} hud_text;

	PostprocessShader *viewport_pp;
	FBPair fb_pairs[NUM_FBPAIRS];
	CustomFramebuffer *custom_fbs;

	bool framerate_graphs;
	bool objpool_stats;

	#ifdef DEBUG
		Sprite dummy;
	#endif
} stagedraw = {
	.hud_text.color = {
		// NOTE: premultiplied alpha
		.active   = { 1.00, 1.00, 1.00, 1.00 },
		.inactive = { 0.49, 0.49, 0.49, 0.70 },
		.label    = { 0.49, 0.49, 0.49, 0.70 },
	}
};

static double fb_scale(void) {
	int vp_width, vp_height;
	video_get_viewport_size(&vp_width, &vp_height);
	return (double)vp_height / SCREEN_H;
}

static void set_fb_size(StageFBPair fb_id, int *w, int *h) {
	double scale = fb_scale();

	switch(fb_id) {
		case FBPAIR_BG:
			scale *= config_get_float(CONFIG_BG_QUALITY);
			break;

		default:
			scale *= config_get_float(CONFIG_FG_QUALITY);
			break;
	}

	scale = sanitize_scale(scale);
	*w = round(VIEWPORT_W * scale);
	*h = round(VIEWPORT_H * scale);
}

static void update_fb_size(StageFBPair fb_id) {
	int w, h;
	set_fb_size(fb_id, &w, &h);
	fbpair_resize_all(stagedraw.fb_pairs + fb_id, w, h);
	fbpair_viewport(stagedraw.fb_pairs + fb_id, 0, 0, w, h);

	if(fb_id != FBPAIR_FG_AUX) {
		for(CustomFramebuffer *cfb = stagedraw.custom_fbs; cfb; cfb = cfb->next) {
			if(cfb->scaling_base == fb_id) {
				int sw = w * cfb->scale;
				int sh = h * cfb->scale;

				for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
					fbutil_resize_attachment(&cfb->fb, i, sw, sh);
				}

				r_framebuffer_viewport(&cfb->fb, 0, 0, sw, sh);
			}
		}
	}
}

static bool stage_draw_event(SDL_Event *e, void *arg) {
	if(!IS_TAISEI_EVENT(e->type)) {
		return false;
	}

	switch(TAISEI_EVENT(e->type)) {
		case TE_VIDEO_MODE_CHANGED: {
			for(uint i = 0; i < NUM_FBPAIRS; ++i) {
				update_fb_size(i);
			}

			break;
		}

		case TE_CONFIG_UPDATED: {
			switch(e->user.code) {
				case CONFIG_FG_QUALITY: {
					update_fb_size(FBPAIR_FG);
					update_fb_size(FBPAIR_FG_AUX);
					break;
				}

				case CONFIG_BG_QUALITY: {
					update_fb_size(FBPAIR_BG);
					break;
				}
			}

			break;
		}
	}

	return false;
}

static void stage_draw_setup_framebuffers(void) {
	int fg_width, fg_height, bg_width, bg_height;
	FBAttachmentConfig a[2], *a_color, *a_depth;
	memset(a, 0, sizeof(a));

	a_color = &a[0];
	a_depth = &a[1];

	a_color->attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a_depth->attachment = FRAMEBUFFER_ATTACH_DEPTH;

	set_fb_size(FBPAIR_FG, &fg_width, &fg_height);
	set_fb_size(FBPAIR_BG, &bg_width, &bg_height);

	// Set up some parameters shared by all attachments
	TextureParams tex_common = {
		.filter.min = TEX_FILTER_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_MIRROR,
		.wrap.t = TEX_WRAP_MIRROR,
	};

	memcpy(&a_color->tex_params, &tex_common, sizeof(tex_common));
	memcpy(&a_depth->tex_params, &tex_common, sizeof(tex_common));

	// Foreground: 1 RGB texture per FB
	a_color->tex_params.type = TEX_TYPE_RGB;
	a_color->tex_params.width = fg_width;
	a_color->tex_params.height = fg_height;
	fbpair_create(stagedraw.fb_pairs + FBPAIR_FG, 1, a);
	fbpair_viewport(stagedraw.fb_pairs + FBPAIR_FG, 0, 0, fg_width, fg_height);

	// Foreground auxiliary: 1 RGBA texture per FB
	a_color->tex_params.type = TEX_TYPE_RGBA;
	fbpair_create(stagedraw.fb_pairs + FBPAIR_FG_AUX, 1, a);
	fbpair_viewport(stagedraw.fb_pairs + FBPAIR_FG_AUX, 0, 0, fg_width, fg_height);

	// Background: 1 RGB texture + depth per FB
	a_color->tex_params.type = TEX_TYPE_RGB;
	a_color->tex_params.width = bg_width;
	a_color->tex_params.height = bg_height;
	a_depth->tex_params.type = TEX_TYPE_DEPTH;
	a_depth->tex_params.width = bg_width;
	a_depth->tex_params.height = bg_height;
	fbpair_create(stagedraw.fb_pairs + FBPAIR_BG, 2, a);
	fbpair_viewport(stagedraw.fb_pairs + FBPAIR_BG, 0, 0, bg_width, bg_height);
}

static Framebuffer* add_custom_framebuffer(StageFBPair fbtype, float scale_factor, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	CustomFramebuffer *cfb = calloc(1, sizeof(*cfb));
	list_push(&stagedraw.custom_fbs, cfb);

	FBAttachmentConfig cfg[num_attachments];
	memcpy(cfg, attachments, sizeof(cfg));

	int width, height;
	set_fb_size(fbtype, &width, &height);
	cfb->scale = scale_factor;
	width *= scale_factor;
	height *= scale_factor;

	for(uint i = 0; i < num_attachments; ++i) {
		cfg[i].tex_params.width = width;
		cfg[i].tex_params.height = height;
	}

	r_framebuffer_create(&cfb->fb);
	fbutil_create_attachments(&cfb->fb, num_attachments, cfg);
	r_framebuffer_viewport(&cfb->fb, 0, 0, width, height);

	r_state_push();
	r_framebuffer(&cfb->fb);
	r_clear_color4(0, 0, 0, 0);
	r_clear(CLEAR_ALL);
	r_state_pop();

	return &cfb->fb;
}

Framebuffer* stage_add_foreground_framebuffer(float scale_factor, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	return add_custom_framebuffer(FBPAIR_FG, scale_factor, num_attachments, attachments);
}

Framebuffer* stage_add_background_framebuffer(float scale_factor, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	return add_custom_framebuffer(FBPAIR_BG, scale_factor, num_attachments, attachments);
}

static void stage_draw_destroy_framebuffers(void) {
	for(uint i = 0; i < NUM_FBPAIRS; ++i) {
		fbpair_destroy(stagedraw.fb_pairs + i);
	}

	for(CustomFramebuffer *cfb = stagedraw.custom_fbs, *next; cfb; cfb = next) {
		next = cfb->next;
		fbutil_destroy_attachments(&cfb->fb);
		r_framebuffer_destroy(&cfb->fb);
		free(list_unlink(&stagedraw.custom_fbs, cfb));
	}
}

void stage_draw_init(void) {
	preload_resources(RES_POSTPROCESS, RESF_OPTIONAL,
		"viewport",
	NULL);

	preload_resources(RES_SPRITE, RESF_PERMANENT,
		"star",
		"hud",
	NULL);

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"titletransition",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"text_hud",
		"text_stagetext",
		"ingame_menu",
		"sprite_circleclipped_indicator",
		#ifdef DEBUG
		"sprite_filled_circle",
		#endif
	NULL);

	preload_resources(RES_FONT, RESF_PERMANENT,
		"hud",
		"mono",
		"small",
		"monosmall",
	NULL);

	stagedraw.framerate_graphs = env_get("TAISEI_FRAMERATE_GRAPHS", GRAPHS_DEFAULT);
	stagedraw.objpool_stats = env_get("TAISEI_OBJPOOL_STATS", OBJPOOLSTATS_DEFAULT);

	if(stagedraw.framerate_graphs) {
		preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
			"graph",
		NULL);
	}

	if(stagedraw.objpool_stats) {
		preload_resources(RES_FONT, RESF_PERMANENT,
			"monotiny",
		NULL);
	}

	stagedraw.viewport_pp = get_resource_data(RES_POSTPROCESS, "viewport", RESF_OPTIONAL);
	stagedraw.hud_text.shader = r_shader_get("text_hud");
	stagedraw.hud_text.font = get_font("hud");

	r_shader_standard();

	#ifdef DEBUG
	stagedraw.dummy.tex = get_sprite("star")->tex;
	stagedraw.dummy.w = 1;
	stagedraw.dummy.h = 1;
	#endif

	stage_draw_setup_framebuffers();

	events_register_handler(&(EventHandler) {
		stage_draw_event, NULL, EPRIO_SYSTEM,
	});
}

void stage_draw_shutdown(void) {
	events_unregister_handler(stage_draw_event);
	stage_draw_destroy_framebuffers();
}

FBPair* stage_get_fbpair(StageFBPair id) {
	assert(id >= 0 && id < NUM_FBPAIRS);
	return stagedraw.fb_pairs + id;
}

static void stage_draw_collision_areas(void) {
#ifdef DEBUG
	static bool enabled, keystate_saved;
	bool keystate = gamekeypressed(KEY_HITAREAS);

	if(keystate ^ keystate_saved) {
		if(keystate) {
			enabled = !enabled;
		}

		keystate_saved = keystate;
	}

	if(!enabled) {
		return;
	}

	r_shader("sprite_filled_circle");
	r_uniform_vec4("color_inner", 0, 0, 0, 1);
	r_uniform_vec4("color_outer", 1, 1, 1, 0.1);

	for(Projectile *p = global.projs.first; p; p = p->next) {
		complex gsize = projectile_graze_size(p);

		if(creal(gsize)) {
			r_draw_sprite(&(SpriteParams) {
				.color = RGB(0, 0.5, 0.5),
				.sprite_ptr = &stagedraw.dummy,
				.pos = { creal(p->pos), cimag(p->pos) },
				.rotation.angle = p->angle + M_PI/2,
				.scale = { .x = creal(gsize), .y = cimag(gsize) },
				.blend = BLEND_SUB,
			});
		}
	}

	r_flush_sprites();
	r_uniform_vec4("color_inner", 0.0, 1.0, 0.0, 0.75);
	r_uniform_vec4("color_outer", 0.0, 0.5, 0.5, 0.75);

	for(Projectile *p = global.projs.first; p; p = p->next) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = &stagedraw.dummy,
			.pos = { creal(p->pos), cimag(p->pos) },
			.rotation.angle = p->angle + M_PI/2,
			.scale = { .x = creal(p->collision_size), .y = cimag(p->collision_size) },
			.blend = BLEND_ALPHA,
		});
	}

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = &stagedraw.dummy,
		.pos = { creal(global.plr.pos), cimag(global.plr.pos) },
		.scale.both = 2, // NOTE: actual player is a singular point
	});

	// TODO: handle other objects the player may collide with (enemies, bosses...)

	r_flush_sprites();
#endif
}

static void apply_shader_rules(ShaderRule *shaderrules, FBPair *fbos) {
	if(!shaderrules) {
		return;
	}

	for(ShaderRule *rule = shaderrules; *rule; ++rule) {
		r_framebuffer(fbos->back);
		(*rule)(fbos->front);
		fbpair_swap(fbos);
	}

	return;
}

static void draw_wall_of_text(float f, const char *txt) {
	Sprite spr;
	BBox bbox;

	text_render(txt, get_font("standard"), &spr, &bbox);

	// FIXME: The shader currently assumes that the sprite takes up the entire texture.
	// If it could handle any arbitrary sprite, then text_render wouldn't have to resize // the texture per every new string of text.

	float w = VIEWPORT_W;
	float h = VIEWPORT_H;

	r_mat_push();
	r_mat_translate(w/2, h/2, 0);
	r_mat_scale(w, h, 1.0);

	ShaderProgram *shader = r_shader_get("spellcard_walloftext");
	r_shader_ptr(shader);
	r_uniform_float("w", spr.tex_area.w/spr.tex->w);
	r_uniform_float("h", spr.tex_area.h/spr.tex->h);
	r_uniform_float("ratio", h/w);
	r_uniform_vec2("origin", creal(global.boss->pos)/h, cimag(global.boss->pos)/w);
	r_uniform_float("t", f);
	r_texture_ptr(0, spr.tex);
	r_draw_quad();
	r_shader_standard();

	r_mat_pop();
}

static void draw_spellbg(int t) {
	r_mat_push();

	Boss *b = global.boss;
	b->current->draw_rule(b, t);

	if(b->current->type == AT_ExtraSpell)
		draw_extraspell_bg(b, t);

	r_mat_push();
	r_mat_translate(creal(b->pos), cimag(b->pos), 0);
	r_mat_rotate_deg(global.frames*7.0, 0, 0, -1);

	if(t < 0) {
		float f = 1.0 - t/(float)ATTACK_START_DELAY;
		r_mat_scale(f,f,f);
	}

	draw_sprite(0, 0, "boss_spellcircle0");
	r_mat_pop();

	float delay = ATTACK_START_DELAY;
	if(b->current->type == AT_ExtraSpell)
		delay = ATTACK_START_DELAY_EXTRA;
	float f = (-t+ATTACK_START_DELAY)/(delay+ATTACK_START_DELAY);
	if(f > 0)
		draw_wall_of_text(f, b->current->name);

	if(t < ATTACK_START_DELAY && b->dialog) {
		r_mat_push();
		float f = -0.5*t/(float)ATTACK_START_DELAY+0.5;
		r_color(RGBA_MUL_ALPHA(1,1,1,-f*f+2*f));
		draw_sprite_p(VIEWPORT_W*3/4-10*f*f,VIEWPORT_H*2/3-10*f*f,b->dialog);
		r_color4(1,1,1,1);
		r_mat_pop();
	}

	r_mat_pop();
}

static inline bool should_draw_stage_bg(void) {
	return (
		!global.boss
		|| !global.boss->current
		|| !global.boss->current->draw_rule
		|| global.boss->current->endtime
		|| (global.frames - global.boss->current->starttime) < 1.25*ATTACK_START_DELAY
	);
}

static void apply_bg_shaders(ShaderRule *shaderrules, FBPair *fbos) {
	Boss *b = global.boss;
	if(b && b->current && b->current->draw_rule) {
		int t = global.frames - b->current->starttime;

		set_ortho(VIEWPORT_W, VIEWPORT_H);

		if(should_draw_stage_bg()) {
			apply_shader_rules(shaderrules, fbos);
		}

		r_framebuffer(fbos->back);
		draw_framebuffer_tex(fbos->front, VIEWPORT_W, VIEWPORT_H);
		draw_spellbg(t);
		fbpair_swap(fbos);
		r_framebuffer(fbos->back);

		complex pos = b->pos;
		float ratio = (float)VIEWPORT_H/VIEWPORT_W;

		if(t<ATTACK_START_DELAY) {
			r_shader("spellcard_intro");

			r_uniform_float("ratio", ratio);
			r_uniform_vec2("origin", creal(pos)/VIEWPORT_W, 1-cimag(pos)/VIEWPORT_H);

			float delay = ATTACK_START_DELAY;
			if(b->current->type == AT_ExtraSpell)
				delay = ATTACK_START_DELAY_EXTRA;
			float duration = ATTACK_START_DELAY_EXTRA;

			r_uniform_float("t", (t+delay)/duration);
		} else if(b->current->endtime) {
			int tn = global.frames - b->current->endtime;
			ShaderProgram *shader = r_shader_get("spellcard_outro");
			r_shader_ptr(shader);

			float delay = ATTACK_END_DELAY;

			if(boss_is_dying(b)) {
				delay = BOSS_DEATH_DELAY;
			} else if(b->current->type == AT_ExtraSpell) {
				delay = ATTACK_END_DELAY_EXTRA;
			}

			r_uniform_float("ratio", ratio);
			r_uniform_vec2("origin", creal(pos)/VIEWPORT_W, 1-cimag(pos)/VIEWPORT_H);
			r_uniform_float("t", max(0,tn/delay+1));
		} else {
			r_shader_standard();
		}

		draw_framebuffer_tex(fbos->front, VIEWPORT_W, VIEWPORT_H);
		fbpair_swap(fbos);
		r_framebuffer(NULL);
		r_shader_standard();
	} else if(should_draw_stage_bg()) {
		set_ortho(VIEWPORT_W, VIEWPORT_H);
		apply_shader_rules(shaderrules, fbos);
	}
}

static void apply_zoom_shader(void) {
	r_shader("boss_zoom");

	complex fpos = global.boss->pos;
	complex pos = fpos + 15*cexp(I*global.frames/4.5);

	r_uniform_vec2("blur_orig", creal(pos)  / VIEWPORT_W,  1-cimag(pos)  / VIEWPORT_H);
	r_uniform_vec2("fix_orig",  creal(fpos) / VIEWPORT_W,  1-cimag(fpos) / VIEWPORT_H);

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

	r_uniform_float("blur_rad", 1.5*spellcard_sup*(0.2+0.025*sin(global.frames/15.0)));
	r_uniform_float("rad", 0.24);
	r_uniform_float("ratio", (float)VIEWPORT_H/VIEWPORT_W);
	r_uniform_rgba("color", &global.boss->zoomcolor);
}

static void stage_render_bg(StageInfo *stage) {
	FBPair *background = stage_get_fbpair(FBPAIR_BG);

	r_framebuffer(background->back);
	r_clear(CLEAR_ALL);

	if(should_draw_stage_bg()) {
		r_mat_push();
		r_mat_translate(-(VIEWPORT_X+VIEWPORT_W/2), -(VIEWPORT_Y+VIEWPORT_H/2),0);
		r_enable(RCAP_DEPTH_TEST);
		stage->procs->draw();
		r_mat_pop();
		fbpair_swap(background);
	}

	apply_bg_shaders(stage->procs->shader_rules, background);
	return;
}

bool stage_should_draw_particle(Projectile *p) {
	return (p->flags & PFLAG_REQUIREDPARTICLE) || config_get_int(CONFIG_PARTICLES);
}

static bool stage_draw_predicate(EntityInterface *ent) {
	if(ent->type == ENT_PROJECTILE) {
		Projectile *p = ENT_CAST(ent, Projectile);

		if(p->type == Particle) {
			return stage_should_draw_particle(p);
		}
	}

	return true;
}

static void stage_draw_objects(void) {
	r_shader("sprite_default");

	if(global.boss) {
		draw_boss_background(global.boss);
	}

	ent_draw(
		config_get_int(CONFIG_PARTICLES)
			? NULL
			: stage_draw_predicate
	);

	if(global.dialog) {
		draw_dialog(global.dialog);
	}

	stage_draw_collision_areas();
	r_shader_standard();
	stagetext_draw();
}

static void postprocess_prepare(Framebuffer *fb, ShaderProgram *s) {
	r_uniform_int("frames", global.frames);
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_vec2("player", creal(global.plr.pos), VIEWPORT_H - cimag(global.plr.pos));
}

void stage_draw_foreground(void) {
	int vw, vh;
	video_get_viewport_size(&vw, &vh);

	// CAUTION: Very intricate pixel perfect scaling that will ruin your day.
	float facw = (float)vw/SCREEN_W;
	float fach = (float)vh/SCREEN_H;
	// confer video_update_quality to understand why this is fach. fach is equal to facw up to roundoff error.
	float scale = fach;

	// draw the foreground Framebuffer
	r_mat_push();
		r_mat_scale(1/facw,1/fach,1);
		r_mat_translate(floorf(facw*VIEWPORT_X), floorf(fach*VIEWPORT_Y), 0);
		r_mat_scale(floorf(scale*VIEWPORT_W)/VIEWPORT_W,floorf(scale*VIEWPORT_H)/VIEWPORT_H,1);

		// apply the screenshake effect
		if(global.shake_view) {
			r_mat_translate(global.shake_view*sin(global.frames),global.shake_view*sin(global.frames*1.1+3),0);
			r_mat_scale(1+2*global.shake_view/VIEWPORT_W,1+2*global.shake_view/VIEWPORT_H,1);
			r_mat_translate(-global.shake_view,-global.shake_view,0);

			if(global.shake_view_fade) {
				global.shake_view -= global.shake_view_fade;
				if(global.shake_view <= 0)
					global.shake_view = global.shake_view_fade = 0;
			}
		}
		draw_framebuffer_tex(stage_get_fbpair(FBPAIR_FG)->front, VIEWPORT_W, VIEWPORT_H);
	r_mat_pop();
}

void stage_draw_scene(StageInfo *stage) {
#ifdef DEBUG
	bool key_nobg = gamekeypressed(KEY_NOBACKGROUND);
#else
	bool key_nobg = false;
#endif

	FBPair *background = stage_get_fbpair(FBPAIR_BG);
	FBPair *foreground = stage_get_fbpair(FBPAIR_FG);

	bool draw_bg = !config_get_int(CONFIG_NO_STAGEBG) && !key_nobg;

	if(draw_bg) {
		// render the 3D background
		stage_render_bg(stage);
	}

	// prepare for 2D rendering into the game viewport framebuffer
	r_framebuffer(foreground->back);
	set_ortho(VIEWPORT_W, VIEWPORT_H);
	r_disable(RCAP_DEPTH_TEST);

	if(draw_bg) {
		// enable boss background distortion
		if(global.boss) {
			apply_zoom_shader();
		}

		// draw the 3D background
		draw_framebuffer_tex(background->front, VIEWPORT_W, VIEWPORT_H);

		// disable boss background distortion
		r_shader_standard();

		// draw bomb background
		// FIXME: we need a more flexible and consistent way for entities to hook
		// into the various stages of scene drawing code.
		if(global.plr.mode->procs.bombbg /*&& player_is_bomb_active(&global.plr)*/) {
			global.plr.mode->procs.bombbg(&global.plr);
		}
	} else if(!key_nobg) {
		r_clear(CLEAR_COLOR);
	}

	// draw the 2D objects
	stage_draw_objects();

	// everything drawn, now apply postprocessing
	fbpair_swap(foreground);

	// stage postprocessing
	apply_shader_rules(global.stage->procs->postprocess_rules, foreground);

	// bomb effects shader if present and player bombing
	if(global.plr.mode->procs.bomb_shader && player_is_bomb_active(&global.plr)) {
		ShaderRule rules[] = { global.plr.mode->procs.bomb_shader, NULL };
		apply_shader_rules(rules, foreground);
	}

	// custom postprocessing
	postprocess(
		stagedraw.viewport_pp,
		foreground,
		postprocess_prepare,
		draw_framebuffer_tex,
		VIEWPORT_W,
		VIEWPORT_H
	);

	// prepare for 2D rendering into the main framebuffer (actual screen)
	r_framebuffer(NULL);
	set_ortho(SCREEN_W, SCREEN_H);

	// draw the game viewport and HUD
	stage_draw_foreground();
	stage_draw_hud();
}

struct glyphcb_state {
	Color *color;
};

static void draw_powerval_callback(Font *font, charcode_t charcode, SpriteParams *spr_params, void *userdata) {
	struct glyphcb_state *st = userdata;

	if(charcode == '.') {
		st->color = &stagedraw.hud_text.color.inactive;
	}

	spr_params->color = st->color;
}

static void draw_numeric_callback(Font *font, charcode_t charcode, SpriteParams *spr_params, void *userdata) {
	struct glyphcb_state *st = userdata;

	if(charcode != '0') {
		st->color = &stagedraw.hud_text.color.active;
	}

	spr_params->color = st->color;
}

static inline void stage_draw_hud_power_value(float ypos, char *buf, size_t bufsize) {
	snprintf(buf, bufsize, "%i.%02i", global.plr.power / 100, global.plr.power % 100);
	text_draw(buf, &(TextParams) {
		.pos = { 170, ypos },
		.font = "mono",
		.align = ALIGN_RIGHT,
		.glyph_callback = {
			draw_powerval_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.active },
		}
	});
}

static void stage_draw_hud_score(Alignment a, float xpos, float ypos, char *buf, size_t bufsize, uint32_t score) {
	snprintf(buf, bufsize, "%010u", score);
	text_draw(buf, &(TextParams) {
		.pos = { xpos, ypos },
		.font = "mono",
		.align = ALIGN_RIGHT,
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive },
		}
	});
}

static void stage_draw_hud_scores(float ypos_hiscore, float ypos_score, char *buf, size_t bufsize) {
	stage_draw_hud_score(ALIGN_RIGHT, 170, (int)ypos_hiscore, buf, bufsize, progress.hiscore);
	stage_draw_hud_score(ALIGN_RIGHT, 170, (int)ypos_score,   buf, bufsize, global.plr.points);
}

static void stage_draw_hud_objpool_stats(float x, float y, float width) {
	ObjectPool **last = &stage_object_pools.first + (sizeof(StageObjectPools)/sizeof(ObjectPool*) - 1);
	Font *font = get_font("monotiny");

	ShaderProgram *sh_prev = r_shader_current();
	r_shader("text_default");
	for(ObjectPool **pool = &stage_object_pools.first; pool <= last; ++pool) {
		ObjectPoolStats stats;
		char buf[32];
		objpool_get_stats(*pool, &stats);

		snprintf(buf, sizeof(buf), "%zu | %5zu", stats.usage, stats.peak_usage);
		// draw_text(ALIGN_LEFT  | AL_Flag_NoAdjust, (int)x,           (int)y, stats.tag, font);
		// draw_text(ALIGN_RIGHT | AL_Flag_NoAdjust, (int)(x + width), (int)y, buf,       font);
		// y += stringheight(buf, font) * 1.1;

		text_draw(stats.tag, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.align = ALIGN_LEFT,
		});

		text_draw(buf, &(TextParams) {
			.pos = { x + width, y },
			.font_ptr = font,
			.align = ALIGN_RIGHT,
		});

		y += font_get_lineskip(font);
	}
	r_shader_ptr(sh_prev);
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

static void draw_graph(float x, float y, float w, float h) {
	r_mat_push();
	r_mat_translate(x + w/2, y + h/2, 0);
	r_mat_scale(w, h, 1);
	r_draw_quad();
	r_mat_pop();
}

static void draw_label(const char *label_str, double y_ofs, struct labels_s* labels) {
	text_draw(label_str, &(TextParams) {
		.font_ptr = stagedraw.hud_text.font,
		.shader_ptr = stagedraw.hud_text.shader,
		.pos = { labels->x.ofs, y_ofs },
		.color = &stagedraw.hud_text.color.label,
	});
}

void stage_draw_hud_text(struct labels_s* labels) {
	char buf[64];
	Font *font;

	r_shader_ptr(stagedraw.hud_text.shader);

	// Labels
	draw_label("Hi-Score:", labels->y.hiscore, labels);
	draw_label("Score:",    labels->y.score,   labels);
	draw_label("Lives:",    labels->y.lives,   labels);
	draw_label("Spells:",   labels->y.bombs,   labels);
	draw_label("Power:",    labels->y.power,   labels);
	draw_label("Graze:",    labels->y.graze,   labels);

	if(stagedraw.objpool_stats) {
		stage_draw_hud_objpool_stats(labels->x.ofs, labels->y.graze + 32, 250);
	}

	// Score/Hi-Score values
	stage_draw_hud_scores(labels->y.hiscore + labels->y.mono_ofs, labels->y.score + labels->y.mono_ofs, buf, sizeof(buf));

	// Lives and Bombs (N/A)
	if(global.stage->type == STAGE_SPELL) {
		r_color4(0.7, 0.7, 0.7, 0.7);
		text_draw("N/A", &(TextParams) { .pos = { -6, labels->y.lives }, .font_ptr = stagedraw.hud_text.font });
		text_draw("N/A", &(TextParams) { .pos = { -6, labels->y.bombs }, .font_ptr = stagedraw.hud_text.font });
		r_color4(1, 1, 1, 1.0);
	}

	// Power value
	stage_draw_hud_power_value(labels->y.power + labels->y.mono_ofs, buf, sizeof(buf));

	// Graze value
	snprintf(buf, sizeof(buf), "%05i", global.plr.graze);
	text_draw(buf, &(TextParams) {
		.pos = { -6, labels->y.graze },
		.shader_ptr = stagedraw.hud_text.shader,
		.font = "mono",
		.glyph_callback = {
			draw_numeric_callback,
			&(struct glyphcb_state) { &stagedraw.hud_text.color.inactive },
		}
	});

	// Warning: pops outer matrix!
	r_mat_pop();

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

	// draw_text(ALIGN_RIGHT | AL_Flag_NoAdjust, SCREEN_W, rint(SCREEN_H - 0.5 * stringheight(buf, _fonts.monosmall)), buf, _fonts.monosmall);
	font = get_font("monosmall");

	text_draw(buf, &(TextParams) {
		.align = ALIGN_RIGHT,
		.pos = { SCREEN_W, SCREEN_H - 0.5 * text_height(font, buf, 0) },
		.font_ptr = font,
	});

	if(global.replaymode == REPLAY_PLAY) {
		r_shader("text_hud");
		// XXX: does it make sense to use the monospace font here?

		snprintf(buf, sizeof(buf), "Replay: %s (%i fps)", global.replay.playername, global.replay_stage->fps);
		int x = 0, y = SCREEN_H - 0.5 * text_height(font, buf, 0);

		x += text_draw(buf, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.inactive,
		});

		if(global.replay_stage->desynced) {
			strlcpy(buf, " (DESYNCED)", sizeof(buf));

			text_draw(buf, &(TextParams) {
				.pos = { x, y },
				.font_ptr = font,
				.color = RGBA_MUL_ALPHA(1.00, 0.20, 0.20, 0.60),
			});
		}
	}
#ifdef PLR_DPS_STATS
	else if(global.frames) {
		int totaldmg = 0;
		int framespan = sizeof(global.plr.dmglog)/sizeof(*global.plr.dmglog);
		int graphspan = framespan;
		static int max = 0;
		float graph[framespan];

		if(graphspan > 120) {
			// shader limitation
			graphspan = 120;
		}

		// hack to update the graph every frame
		player_register_damage(&global.plr, NULL, &(DamageInfo) { .amount = 0, .type = DMG_PLAYER_SHOT });

		for(int i = 0; i < framespan; ++i) {
			totaldmg += global.plr.dmglog[i];

			if(global.plr.dmglog[i] > max) {
				max = global.plr.dmglog[i];
			}
		}

		for(int i = 0; i < graphspan; ++i) {
			if(max > 0) {
				graph[i] = (float)global.plr.dmglog[i] / max;
			} else {
				graph[i] = 0;
			}
		}

		snprintf(buf, sizeof(buf), "%.02f", totaldmg / (framespan / (double)FPS));
		double text_h = text_height(font, buf, 0);
		double x = 0, y = SCREEN_H - 0.5 * text_h;

		x += text_draw("Avg DPS: ", &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.inactive,
		});

		text_draw(buf, &(TextParams) {
			.pos = { x, y },
			.font_ptr = font,
			.color = &stagedraw.hud_text.color.active,
		});

		r_shader("graph");
		r_uniform_vec3("color_low",  1.0, 0.0, 0.0);
		r_uniform_vec3("color_mid",  1.0, 1.0, 0.0);
		r_uniform_vec3("color_high", 0.0, 1.0, 0.0);
		r_uniform("points[0]", graphspan, graph);
		draw_graph(142, SCREEN_H - text_h, graphspan, text_h);
	}
#endif

	r_shader_standard();
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

	float pad = 15;

	float w = 260 - pad;
	float h = 30;

	float x = SCREEN_W - w - pad;
	float y = 100;

	r_shader("graph");

	fill_graph(NUM_SAMPLES, samples, &global.fps.logic);
	r_uniform_vec3("color_low",  0.0, 1.0, 1.0);
	r_uniform_vec3("color_mid",  1.0, 1.0, 0.0);
	r_uniform_vec3("color_high", 1.0, 0.0, 0.0);
	r_uniform("points[0]", NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	// x -= w * 1.1;
	y += h + 1;

	fill_graph(NUM_SAMPLES, samples, &global.fps.busy);
	r_uniform_vec3("color_low",  0.0, 1.0, 0.0);
	r_uniform_vec3("color_mid",  1.0, 0.0, 0.0);
	r_uniform_vec3("color_high", 1.0, 0.0, 0.5);
	r_uniform("points[0]", NUM_SAMPLES, samples);
	draw_graph(x, y, w, h);

	r_shader_standard();
}

void stage_draw_hud(void) {
	// Background
	draw_sprite(SCREEN_W/2.0, SCREEN_H/2.0, "hud");

	// Set up positions of most HUD elements
	static struct labels_s labels = {
		.x.ofs = -75,

		// XXX: is there a more robust way to level the monospace font with the label one?
		// .y.mono_ofs = 0.5,
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

	r_mat_push();
	r_mat_translate(615, 0, 0);

	// Difficulty indicator
	r_mat_push();
	r_mat_translate((SCREEN_W - 615) * 0.25, SCREEN_H-170, 0);
	r_mat_scale(0.6, 0.6, 0);
	draw_sprite(0, 0, difficulty_sprite_name(global.diff));
	r_mat_pop();

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
		draw_stars(0, labels.y.lives, global.plr.lives, global.plr.life_fragments, PLR_MAX_LIVES, PLR_MAX_LIFE_FRAGMENTS, a, 20);
		draw_stars(0, labels.y.bombs, global.plr.bombs, global.plr.bomb_fragments, PLR_MAX_BOMBS, PLR_MAX_BOMB_FRAGMENTS, a, 20);
	}

	// Power stars
	draw_stars(0, labels.y.power, global.plr.power / 100, global.plr.power % 100, PLR_MAX_POWER / 100, 100, 1, 20);

	ShaderProgram *sh_prev = r_shader_current();
	r_shader("text_default");
	// God Mode indicator
	if(global.plr.iddqd) {
		text_draw("GOD MODE", &(TextParams) { .pos = { -70, 475 }, .font = "big" });
	}

	// Extra Spell indicator
	if(s) {
		float s2 = max(0, swing(s, 3));
		r_mat_push();
		r_mat_translate((SCREEN_W - 615) * 0.25 - 615 * (1 - pow(2*fadein-1, 2)), 340, 0);
		r_color(RGBA_MUL_ALPHA(0.3, 0.6, 0.7, 0.7 * s));
		r_mat_rotate_deg(-25 + 360 * (1-s2), 0, 0, 1);
		r_mat_scale(s2, s2, 0);

		text_draw("Extra Spell!", &(TextParams) { .pos = {  1,  1 }, .font = "big", .align = ALIGN_CENTER });
		text_draw("Extra Spell!", &(TextParams) { .pos = { -1, -1 }, .font = "big", .align = ALIGN_CENTER });
		r_color4(s, s, s, s);
		text_draw("Extra Spell!", &(TextParams) { .pos = {  0,  0 }, .font = "big", .align = ALIGN_CENTER });
		r_color4(1, 1, 1, 1);
		r_mat_pop();
	}

	r_shader_ptr(sh_prev);
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
		
		r_color4(1 - red, 1 - red, 1 - red, 1 - red);
		draw_sprite(VIEWPORT_X+creal(global.boss->pos), 590, "boss_indicator");
		r_color4(1, 1, 1, 1);
	}
}
