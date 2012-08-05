/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "credits.h"
#include "global.h"
#include "stageutils.h"
#include "video.h"

static Stage3D bgcontext;

typedef struct CreditsEntry {
	char **data;
	int lines;
	int time;
} CreditsEntry;

static struct {
	CreditsEntry *entries;
	int ecount;
	float panelalpha;
} credits;

void credits_add(char *data, int time) {
	CreditsEntry *e;
	char *c, buf[256];
	int l = 0, i = 0;
	
	credits.entries = realloc(credits.entries, (++credits.ecount) * sizeof(CreditsEntry));
	e = &(credits.entries[credits.ecount-1]);
	e->time = time;
	e->lines = 1;
	
	for(c = data; *c; ++c)
		if(*c == '\n') e->lines++;
	e->data = malloc(e->lines * sizeof(char**));
	
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
}

Vector **stage6_towerwall_pos(Vector pos, float maxrange);
void stage6_towerwall_draw(Vector pos);

void credits_skysphere_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/sky")->gltex);
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]-30);
	float s = 370;
	glScalef(s, s, s);
	draw_model("skysphere");
	glPopMatrix();
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
}

void credits_towerwall_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/towerwall")->gltex);
	if(!tconfig.intval[NO_SHADER]) {
		Shader *s = get_shader("tower_wall");
		glUseProgram(s->prog);
		glUniform1i(uniloc(s, "lendiv"), 2800.0 + 300.0 * sin(global.frames / 77.7));
	}
	
	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
// 	glRotatef(90, 1,0,0);
	glScalef(30,30,30);
	draw_model("towerwall");
	glPopMatrix();
	
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
}

void credits_fill(void) {
	credits_add("Taisei Project", 200);
	credits_add("String 1\nDerp 2\nHuge derp 3", 500);
	credits_add("lol 1\nomg 2\nwtf 3", 500);
}

Vector **credits_skysphere_pos(Vector pos, float maxrange) {
	return single3dpos(pos, maxrange, bgcontext.cx);
}

void credits_init(void) {
	memset(&credits, 0, sizeof(credits));
	init_stage3d(&bgcontext);
	
	add_model(&bgcontext, credits_skysphere_draw, credits_skysphere_pos);
	add_model(&bgcontext, credits_towerwall_draw, stage6_towerwall_pos);
	
	bgcontext.cx[0] = 0;
	bgcontext.cx[1] = 600;
	bgcontext.crot[0] = 0;
	bgcontext.crot[1] = 10;
	
	global.frames = 0;
	credits_fill();
}

void credits_draw_entry(CreditsEntry *e) {
	int time = global.frames - 400, i;
	float first, other = 0, fadein = 1, fadeout = 1;
	CreditsEntry *o;
	
	for(o = credits.entries; o != e; ++o)
		time -= o->time + CREDITS_ENTRY_FADEOUT;
	
	if(time < 0)
		return;
	
	if(time <= CREDITS_ENTRY_FADEIN)
		fadein = time / CREDITS_ENTRY_FADEIN;
	
	if(time - e->time - CREDITS_ENTRY_FADEIN > 0)
		fadeout = max(0, 1 - (time - e->time - CREDITS_ENTRY_FADEIN) / CREDITS_ENTRY_FADEOUT);
	
	if(!fadein || !fadeout)
		return;
	
	first = stringheight(e->data[0], _fonts.mainmenu) * 1.2;
	if(e->lines > 1)
		other = stringheight(e->data[1], _fonts.standard) * 1.3;
	
	glPushMatrix();
	if(fadein < 1)
		glTranslatef(0, (SCREEN_W * pow(1 - fadein,  2)) *  0.5, 0);
	else if(fadeout < 1)
		glTranslatef(0, (SCREEN_W * pow(1 - fadeout, 2)) * -0.5, 0);
	
	glColor4f(1, 1, 1, fadein * fadeout);
	for(i = 0; i < e->lines; ++i)
		draw_text(AL_Center, 0, -0.5 * (i? (other * (e->lines - i) - first) : first), e->data[i], (i? _fonts.standard : _fonts.mainmenu));
	glPopMatrix();
}

void credits_draw(void) {
	//glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glTranslatef(-SCREEN_W/2, 0, 0);
	glEnable(GL_DEPTH_TEST);
	
	set_perspective_viewport(&bgcontext, 100, 9000, 0, 0, SCREEN_W, SCREEN_H);
	draw_stage3d(&bgcontext, 10000);
	
	bgcontext.cx[2] = 200 - global.frames * 50;
	bgcontext.cx[1] = 500 + 100 * psin(global.frames / 100.0) * psin(global.frames / 200.0 + M_PI);
	bgcontext.cx[0] += nfrand();
	
	glPopMatrix();
	set_ortho();
	
	glPushMatrix();
	glColor4f(0, 0, 0, credits.panelalpha * 0.7);
	glTranslatef(SCREEN_W/4*3, SCREEN_H/2, 0);
	glScalef(300, SCREEN_H, 1);
	draw_quad();
	glPopMatrix();
	
	glPushMatrix();
	glColor4f(1, 1, 1, credits.panelalpha * 0.7);
	glTranslatef(SCREEN_W/4*3, SCREEN_H/2, 0);
	
	int i; for(i = 0; i < credits.ecount; ++i)
		credits_draw_entry(&(credits.entries[i]));
	
	glPopMatrix();
	
	fade_out(1 - (global.frames / 300.0));
}

void credits_process(void) {
	TIMER(&global.frames);
	
	FROM_TO(200, 300, 1)
		credits.panelalpha += 0.01;
}

void credits_input(void) {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		//int sym = event.key.keysym.sym;
		global_processevent(&event);
		
		switch(event.type) {
			case SDL_QUIT:
				exit(1);
				break;
		}
	}
}

void credits_free(void) {
	int i, j;
	for(i = 0; i < credits.ecount; ++i) {
		CreditsEntry *e = &(credits.entries[i]);
		for(j = 0; j < e->lines; ++j)
			free(e->data[j]);
		free(e->data);
	}
	
	free(credits.entries);
}

void credits_loop(void) {
	credits_init();
	while(1) {
		credits_input();
		credits_process();
		credits_draw();
		global.frames++;
		SDL_GL_SwapBuffers();
		frame_rate(&global.lasttime);
	}
	credits_free();
}
