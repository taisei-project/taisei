/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "credits.h"
#include "global.h"
#include "stages/stage6.h"
#include "video.h"
#include "resource/model.h"
#include "renderer/api.h"

typedef struct CreditsEntry {
	char **data;
	int lines;
	int time;
} CreditsEntry;

static struct {
	CreditsEntry *entries;
	int ecount;
	float panelalpha;
	int end;
} credits;

#define CREDITS_ENTRY_FADEIN 200.0
#define CREDITS_ENTRY_FADEOUT 100.0
#define CREDITS_YUKKURI_SCALE 0.5

#define CREDITS_FADEOUT 240

#define ENTRY_TIME 350

static void credits_add(char *data, int time);

static void credits_fill(void) {
	// In case the shortened URLs break,
	// Tuck V's YouTube: https://www.youtube.com/channel/UCaw73cuHLnFCSpjOtt_9pyg
	// Lalasa's YouTube: https://www.youtube.com/channel/UCc6ePuGLYnKTkdDqxP3OB4Q
	// InsideI's bandcamp: https://vnutriya.bandcamp.com/

	credits_add("Taisei Project\nbrought to you by…", 200);

	credits_add((
		"laochailan\n"
		"Lukas Weber\n"
		"laochailan@web.de\n\n"
		"Programming, game design,"
		"\ngraphics"
	), ENTRY_TIME);

	credits_add((
		"Akari\n"
		"Andrei Alexeyev\n"
		"akari@alienslab.net\n\n"
		"Programming, game design"
	), ENTRY_TIME);

	credits_add((
		"Tuck V\n"
		"Discord: @Tuck#1679\n"
		"YouTube: https://is.gd/exafez\n\n"
		"Original soundtrack"
	), ENTRY_TIME);

	credits_add((
		"InsideI\n"
		"Mikhail Novik\n"
		"Bandcamp: https://is.gd/owojix\n\n"
		"Sound effects"
	), ENTRY_TIME);

	credits_add((
		"Lalasa\n"
		"Ola Kruzel\n"
		"okruzel@comcast.net\n"
		"YouTube: https://is.gd/ohihef\n\n"
		"Writing, playtesting"
	), ENTRY_TIME);

	credits_add((
		"lachs0r\n"
		"Martin Herkt\n"
		"lachs0r@hong-mailing.de\n\n"
		"Hosting, packaging, editing,\n"
		"spiritual guidance"
	), ENTRY_TIME);

	credits_add((
		"makise-homura\n"
		"Igor Molchanov\n"
		"akemi_homura@kurisa.ch\n\n"
		"Code contributions\n"
		"Elbrus compatible™"
	), ENTRY_TIME);

	credits_add((
		"aiju\n"
		"Julius Schmidt\n"
		"http://aiju.de/\n\n"
		"I don't remember\n"
		"what this guy did"
	), ENTRY_TIME);

	credits_add("Special Thanks", ENTRY_TIME);

	credits_add((
		"ZUN\n"
		"for Tōhō Project\n"
		"http://www16.big.or.jp/~zun/"
	), ENTRY_TIME);

	credits_add((
		"Free Software\n"
		"Simple DirectMedia Layer\n"
		"https://libsdl.org/\n\n"
		"zlib\n"
		"https://zlib.net/\n\n"
		"libpng\n"
		"http://www.libpng.org/\n\n"
		"Ogg Vorbis\n"
		"https://xiph.org/vorbis/"
	), 350);

	credits_add((
		"\n"
		"libzip\n"
		"https://libzip.org/\n\n"
		"Meson build system\n"
		"http://mesonbuild.com/\n\n"
		"M cross environment\n"
		"http://mxe.cc/\n\n"
		"and many other projects"
	), 350);

	credits_add((
		"Mochizuki Ado\n"
		"for a nice yukkuri image"
	), ENTRY_TIME);

	credits_add((
		"…and You!\n"
		"for playing"
	), ENTRY_TIME);

	credits_add((
		"Visit Us\n"
		"https://taisei-project.org/\n\n"
		"And join our IRC channel\n"
		"#taisei-project at irc.freenode.net\n\n"
		"Or our Discord server\n"
		"https://discord.gg/JEHCMzW"
	), ENTRY_TIME);

	// yukkureimu
	credits_add("*\nAnd don't forget to take it easy!", 200);
}

static void credits_add(char *data, int time) {
	CreditsEntry *e;
	char *c, buf[256];
	int l = 0, i = 0;

	credits.entries = realloc(credits.entries, (++credits.ecount) * sizeof(CreditsEntry));
	e = &(credits.entries[credits.ecount-1]);
	e->time = time;
	e->lines = 1;

	for(c = data; *c; ++c)
		if(*c == '\n') e->lines++;
	e->data = malloc(e->lines * sizeof(char*));

	for(c = data; *c; ++c) {
		if(*c == '\n') {
			buf[i] = 0;
			e->data[l] = malloc(strlen(buf) + 1);
			strcpy(e->data[l], buf);
			i = 0;
			++l;
		} else {
			buf[i++] = *c;
		}
	}

	buf[i] = 0;
	e->data[l] = malloc(strlen(buf) + 1);
	strcpy(e->data[l], buf);
	credits.end += time + CREDITS_ENTRY_FADEOUT;
}

static void credits_towerwall_draw(vec3 pos) {
	r_texture(0, "stage6/towerwall");

	r_shader("tower_wall");
	r_uniform_float("lendiv", 2800.0 + 300.0 * sin(global.frames / 77.7));

	r_mat_push();
	r_mat_translate(pos[0], pos[1], pos[2]);
	r_mat_scale(30,30,30);
	r_draw_model("towerwall");
	r_mat_pop();

	r_shader_standard();
}

static void credits_init(void) {
	memset(&credits, 0, sizeof(credits));
	init_stage3d(&stage_3d_context);

	add_model(&stage_3d_context, credits_towerwall_draw, stage6_towerwall_pos);

	stage_3d_context.cx[0] = 0;
	stage_3d_context.cx[1] = 600;
	stage_3d_context.crot[0] = 0;

	global.frames = 0;
	credits_fill();
	credits.end += 500 + CREDITS_ENTRY_FADEOUT;

	start_bgm("credits");
}

static double entry_height(CreditsEntry *e, double *head, double *body) {
	double total = *head = *body = 0;

	if(!e->lines) {
		return total;
	}

	if(e->lines > 0) {
		if(*(e->data[0]) == '*') {
			total += *head = get_tex("yukkureimu")->h * CREDITS_YUKKURI_SCALE;
		} else {
			total += *head = font_get_lineskip(get_font("big"));
		}

		if(e->lines > 1) {
			total += *body += (e->lines - 0.5) * font_get_lineskip(get_font("standard"));
		}
	}

	return total;
}

static void credits_draw_entry(CreditsEntry *e) {
	int time = global.frames - 400;
	float fadein = 1, fadeout = 1;

	for(CreditsEntry *o = credits.entries; o != e; ++o) {
		time -= o->time + CREDITS_ENTRY_FADEOUT;
	}

	double h_total, h_head, h_body;
	h_total = entry_height(e, &h_head, &h_body);

	// random asspull approximation to make stuff not overlap too much
	int ofs = (1 - pow(1 - h_total / SCREEN_H, 2)) * SCREEN_H * 0.095;
	time -= ofs;

	if(time < 0) {
		return;
	}

	ofs *= 2;

	if(time <= CREDITS_ENTRY_FADEIN) {
		fadein = time / CREDITS_ENTRY_FADEIN;
	}

	if(time - e->time - CREDITS_ENTRY_FADEIN + ofs > 0) {
		fadeout = max(0, 1 - (time - e->time - CREDITS_ENTRY_FADEIN + ofs) / CREDITS_ENTRY_FADEOUT);
	}

	if(!fadein || !fadeout) {
		return;
	}

	bool yukkuri = false;
	Sprite *yukkuri_spr = NULL;

	if(*e->data[0] == '*') {
		yukkuri = true;
		yukkuri_spr = get_sprite("yukkureimu");
	}

	r_mat_push();

	if(fadein < 1) {
		r_mat_translate(0, SCREEN_W * pow(1 - fadein,  2) *  0.5, 0);
	} else if(fadeout < 1) {
		r_mat_translate(0, SCREEN_W * pow(1 - fadeout, 2) * -0.5, 0);
	}

	// for debugging: draw a quad as tall as the entry is expected to be
	/*
	render_push();
	render_color4(1, 0, 0, fadein * fadeout);
	render_shader_standard_notex();
	render_scale(300, h_total, 1);
	render_draw_quad();
	render_shader_standard();
	render_pop();
	*/

	r_color(RGBA_MUL_ALPHA(1, 1, 1, fadein * fadeout));

	if(yukkuri) {
		r_mat_translate(0, (-h_body) * 0.5, 0);
	} else {
		r_mat_translate(0, (-h_body) * 0.5, 0);
	}

	for(int i = 0; i < e->lines; ++i) {
		if(yukkuri && !i) {
			r_mat_push();
			r_mat_scale(CREDITS_YUKKURI_SCALE, CREDITS_YUKKURI_SCALE, 1.0);
			draw_sprite_p(0, 10 * sin(global.frames / 10.0) * fadeout * fadein, yukkuri_spr);
			r_mat_pop();
			r_mat_translate(0, yukkuri_spr->h * CREDITS_YUKKURI_SCALE * 0.5, 0);
		} else {
			Font *font = get_font(i ? "standard" : "big");
			r_shader("text_default");
			text_draw(e->data[i], &(TextParams) {
				.align = ALIGN_CENTER,
				.font_ptr = font,
			});
			r_shader_standard();
			r_mat_translate(0, font_get_lineskip(font), 0);
		}
	}

	r_mat_pop();
	r_color4(1, 1, 1, 1);
}

static void credits_draw(void) {
	r_clear(CLEAR_ALL);
	colorfill(1, 1, 1, 1); // don't use r_clear_color4 for this, it screws up letterboxing

	r_mat_push();
	r_mat_translate(-SCREEN_W/2, 0, 0);
	r_enable(RCAP_DEPTH_TEST);

	set_perspective_viewport(&stage_3d_context, 100, 9000, 0, 0, SCREEN_W, SCREEN_H);
	draw_stage3d(&stage_3d_context, 10000);

	r_mat_pop();
	set_ortho(SCREEN_W, SCREEN_H);

	r_mat_push();
	r_color4(0, 0, 0, credits.panelalpha * 0.7);
	r_mat_translate(SCREEN_W/4*3, SCREEN_H/2, 0);
	r_mat_scale(300, SCREEN_H, 1);
	r_draw_quad();
	r_color4(1, 1, 1, 1);
	r_mat_pop();

	r_mat_push();
	r_mat_translate(SCREEN_W/4*3, SCREEN_H/2, 0);

	for(int i = 0; i < credits.ecount; ++i) {
		credits_draw_entry(&(credits.entries[i]));
	}

	r_mat_pop();

	draw_transition();
}

static void credits_finish(void *arg) {
	credits.end = 0;
	set_transition(TransLoader, 0, FADE_TIME*10);
}

static void credits_process(void) {
	TIMER(&global.frames);

	stage_3d_context.cx[2] = 200 - global.frames * 50;
	stage_3d_context.cx[1] = 500 + 100 * psin(global.frames / 100.0) * psin(global.frames / 200.0 + M_PI);
	stage_3d_context.cx[0] = 25 * sin(global.frames / 75.7) * cos(global.frames / 99.3);

	FROM_TO(200, 300, 1)
		credits.panelalpha += 0.01;

	if(global.frames >= credits.end - CREDITS_ENTRY_FADEOUT) {
		credits.panelalpha -= 1 / 120.0;
	}

	if(global.frames == credits.end) {
		set_transition_callback(TransFadeWhite, CREDITS_FADEOUT, CREDITS_FADEOUT, credits_finish, NULL);
	}
}

static void credits_free(void) {
	int i, j;
	for(i = 0; i < credits.ecount; ++i) {
		CreditsEntry *e = &(credits.entries[i]);
		for(j = 0; j < e->lines; ++j)
			free(e->data[j]);
		free(e->data);
	}

	free(credits.entries);
}

void credits_preload(void) {
	preload_resource(RES_BGM, "credits", RESF_OPTIONAL);
	preload_resource(RES_SHADER_PROGRAM, "tower_wall", RESF_DEFAULT);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage6/towerwall",
		"yukkureimu",
	NULL);
}

static FrameAction credits_logic_frame(void *arg) {
	update_transition();
	events_poll(NULL, 0);
	credits_process();
	global.frames++;
	return credits.end == 0 ? LFRAME_STOP : LFRAME_WAIT;
}

static FrameAction credits_render_frame(void *arg) {
	credits_draw();
	return RFRAME_SWAP;
}

void credits_loop(void) {
	credits_preload();
	credits_init();
	loop_at_fps(credits_logic_frame, credits_render_frame, NULL, FPS);
	credits_free();
}
