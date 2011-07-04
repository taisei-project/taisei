/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <https://github.com/nexAkari>
 */

#include <stdio.h>
#include "menu.h"
#include "options.h"
#include "global.h"
#include "paths/native.h"

// --- Menu entry <-> config option binding stuff --- //

// Initializes an allocated binding
void initialize_binding(OptionBinding* bind)
{
	memset(bind, 0, sizeof(OptionBinding));
	bind->selected 	 	= -1;
	bind->configentry 	= -1;
}

// Allocates a binding (bound to the last menu entry)
OptionBinding* allocate_binding(MenuData *m)
{
	OptionBinding *binds = (OptionBinding*)m->context, *bind;
	binds = realloc(binds, m->ecount * sizeof(OptionBinding));
	
	bind = &(binds[m->ecount - 1]);
	initialize_binding(bind);
	m->context = (void*)binds;

	return bind;
}

// Destroys all bindings
void free_bindings(MenuData *m)
{
	OptionBinding *binds = (OptionBinding*)m->context;
	
	int i, j;
	for(i = 0; i < m->ecount; ++i)
	{
		OptionBinding *bind = &(binds[i]);
		if(!bind->enabled)
			continue;
		
		if(bind->values) for(j = 0; j < bind->valcount; ++j)
			free(bind->values[j]);
		free(bind->optname);
	}
	free(binds);
}

// Binds the last entry to an integer config option having limited values (BT_IntValue type binding).
// Values are defined with bind_addvalue.
OptionBinding* bind_option(MenuData *m, char *optname, int cfgentry, BindingGetter getter, BindingSetter setter)
{
	OptionBinding *binds = (OptionBinding*)m->context, *bind;
	bind = allocate_binding(m);
	
	bind->getter = getter;
	bind->setter = setter;
	bind->configentry = cfgentry;
	bind->optname = malloc(strlen(optname) + 1);
	strcpy(bind->optname, optname);
	bind->enabled = True;
	bind->type = BT_IntValue;
	
	return bind;
}

// Binds the last entry to a keybinding config option (BT_KeyBinding type binding).
OptionBinding* bind_keybinding(MenuData *m, char *optname, int cfgentry)
{
	OptionBinding *binds = (OptionBinding*)m->context, *bind;
	bind = allocate_binding(m);
	
	bind->configentry = cfgentry;
	bind->optname = malloc(strlen(optname) + 1);
	strcpy(bind->optname, optname);
	bind->enabled = True;
	bind->type = BT_KeyBinding;
	
	return bind;
}

// Returns a pointer to the first found binding that blocks input. If none found, returns NULL.
OptionBinding* get_input_blocking_binding(MenuData *m)
{
	OptionBinding *binds = (OptionBinding*)m->context;
	
	int i;
	for(i = 0; i < m->ecount; ++i)
		if(binds[i].blockinput)
			return &(binds[i]);
	return NULL;
}

// Adds a value to a BT_IntValue type binding
int bind_addvalue(OptionBinding *b, char *val)
{
	b->values = realloc(b->values, ++b->valcount);
	b->values[b->valcount-1] = malloc(strlen(val) + 1);
	strcpy(b->values[b->valcount-1], val);
	return b->valcount-1;
}

// Called to select a value of a BT_IntValue type binding by index
int binding_setvalue(OptionBinding *b, int v)
{
	return b->selected = b->setter(b, v);
}

// Called to get the selected value of a BT_IntValue type binding by index
int binding_getvalue(OptionBinding *b)
{
	// query AND update
	return b->selected = b->getter(b);
}

// Selects the next to current value of a BT_IntValue type binding
int binding_setnext(OptionBinding *b)
{
	int s = b->selected + 1;
	if(s >= b->valcount)
		s = 0;
		
	return binding_setvalue(b, s);
}

// Selects the previous to current value of a BT_IntValue type binding
int binding_setprev(OptionBinding *b)
{
	int s = b->selected - 1;
	if(s < 0)
		s = b->valcount - 1;
		
	return binding_setvalue(b, s);
}

// Initializes selection for all BT_IntValue type bindings
void bindings_initvalues(MenuData *m)
{
	OptionBinding *binds = (OptionBinding*)m->context;
	
	int i;
	for(i = 0; i < m->ecount; ++i)
		if(binds[i].enabled && binds[i].type == BT_IntValue)
			binding_getvalue(&(binds[i]));
}

// --- Shared binding callbacks --- //

int bind_common_onoffget(void *b)
{
	return !tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset(void *b, int v)
{
	return !(tconfig.intval[((OptionBinding*)b)->configentry] = !v);
}

int bind_common_onoffget_inverted(void *b)
{
	return tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset_inverted(void *b, int v)
{
	return tconfig.intval[((OptionBinding*)b)->configentry] = v;
}

// --- Binding callbacks for individual options --- //

int bind_fullscreen_set(void *b, int v)
{
	toggle_fullscreen();
	return bind_common_onoffset(b, v);
}

int bind_noaudio_set(void *b, int v)
{
	int i = bind_common_onoffset_inverted(b, v);
	
	if(v)
	{
		alutExit();
		printf("-- Unloaded ALUT\n");
		
		if(resources.state & RS_SfxLoaded)
		{
			delete_sounds();
			resources.state &= ~RS_SfxLoaded;
		}
	}
	else
	{
		if(!alutInit(NULL, NULL))
		{
			warnx("Error initializing audio: %s", alutGetErrorString(alutGetError()));
			tconfig.intval[NO_AUDIO] = 1;
			return 1;	// index of "off"
		}
		tconfig.intval[NO_AUDIO] = 0;
		printf("-- ALUT\n");
		
		load_resources();
	}
	
	return i;
}

int bind_noshader_set(void *b, int v)
{
	int i = bind_common_onoffset_inverted(b, v);
	
	if(!v)
		load_resources();
	
	return i;
}

// --- Config saving --- //

void menu_save_config(MenuData *m, char *filename)
{
	char *buf;
	buf = malloc(strlen(filename) + strlen(get_config_path()) + 2);
	strcpy(buf, get_config_path());
	strcat(buf, "/");
	strcat(buf, filename);
	
	FILE *out = fopen(buf, "w");
	free(buf);
	
	if(!out)
	{
		perror("fopen");
		return;
	}
	
	fputs("# Generated by taisei\n", out);
	
	int i;
	for(i = 0; i < m->ecount; ++i)
	{
		OptionBinding *binds = (OptionBinding*)m->context;
		OptionBinding *bind = &(binds[i]);
		
		if(!bind->enabled)
			continue;
		
		switch(bind->type)
		{
			case BT_IntValue:
				fprintf(out, "%s = %i\n", bind->optname, tconfig.intval[bind->configentry]);
				break;
			
			case BT_KeyBinding:
				fprintf(out, "%s = K%i\n", bind->optname, tconfig.intval[bind->configentry]);
				break;
			
			default:
				printf("FIXME: unhandled BindingType %i, option '%s' will NOT be saved!\n", bind->type, bind->optname);
		}
	}
	
	fclose(out);
	printf("Saved config '%s'\n", filename);
}

// --- Creating, destroying, filling the menu --- //

void destroy_options_menu(void *menu)
{
	MenuData *m = (MenuData*)menu;
	menu_save_config(m, CONFIG_FILE);
	free_bindings(menu);
}

void do_nothing(void *arg) { }
void backtomain(void *arg)
{
	MenuData *m = arg;
	m->quit = 2;
}

void create_options_menu(MenuData *m) {
	OptionBinding* b;
	
	create_menu(m);
	m->type = MT_Persistent;
	m->ondestroy = destroy_options_menu;
	m->context = NULL;
	
	#define bind_onoff(b) bind_addvalue(b, "on"); bind_addvalue(b, "off")
	
	add_menu_entry(m, "Fullscreen", do_nothing, NULL);
		b = bind_option(m, "fullscreen", FULLSCREEN, bind_common_onoffget, bind_fullscreen_set);
			bind_onoff(b);
			
	add_menu_entry(m, "Audio", do_nothing, NULL);
		b = bind_option(m, "disable_audio", NO_AUDIO, bind_common_onoffget_inverted,
													  bind_noaudio_set);
			bind_onoff(b);
		
	add_menu_entry(m, "Shader", do_nothing, NULL);
		b = bind_option(m, "disable_shader", NO_SHADER, bind_common_onoffget_inverted,
														bind_noshader_set);
			bind_onoff(b);
		
	add_menu_entry(m, " ", NULL, NULL);
		allocate_binding(m);
	
	add_menu_entry(m, "Move up", do_nothing, NULL);
		bind_keybinding(m, "key_up", KEY_UP);
	
	add_menu_entry(m, "Move down", do_nothing, NULL);
		bind_keybinding(m, "key_down", KEY_DOWN);
		
	add_menu_entry(m, "Move left", do_nothing, NULL);
		bind_keybinding(m, "key_left", KEY_LEFT);
		
	add_menu_entry(m, "Move right", do_nothing, NULL);
		bind_keybinding(m, "key_right", KEY_RIGHT);
	
	add_menu_entry(m, " ", NULL, NULL);
		allocate_binding(m);
	
	add_menu_entry(m, "Fire", do_nothing, NULL);
		bind_keybinding(m, "key_shot", KEY_SHOT);
		
	add_menu_entry(m, "Focus", do_nothing, NULL);
		bind_keybinding(m, "key_focus", KEY_FOCUS);
	
	add_menu_entry(m, "Bomb", do_nothing, NULL);
		bind_keybinding(m, "key_bomb", KEY_BOMB);
		
	add_menu_entry(m, " ", NULL, NULL);
		allocate_binding(m);
	
	add_menu_entry(m, "Toggle fullscreen", do_nothing, NULL);
		bind_keybinding(m, "key_fullscreen", KEY_FULLSCREEN);
		
	add_menu_entry(m, "Take a screenshot", do_nothing, NULL);
		bind_keybinding(m, "key_screenshot", KEY_SCREENSHOT);
		
	add_menu_entry(m, " ", NULL, NULL);
		allocate_binding(m);
		
	add_menu_entry(m, "Return to the main menu", backtomain, m);
		allocate_binding(m);
}

// --- Drawing the menu --- //

void draw_options_menu_bg(MenuData* menu) {
	glColor4f(0.3, 0.3, 0.3, 1);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(1,0.6,0.5,0.6 + 0.1*sin(menu->frames/100.0));
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
}

void draw_options_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	glColor4f(1,1,1,1);
	draw_text(AL_Right, 140*(1-menu->fade), 30, "Options", _fonts.mainmenu);
	
	//glColor4f(1,1,1,0.7);
	//draw_texture(SCREEN_W/2+40, SCREEN_H/2, "mainmenu/gate");
	
	glPushMatrix();
	glTranslatef(100, 100, 0);
	
	glPushMatrix();
	glTranslatef(0, menu->drawdata[2], 0);
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
		glVertex3f(0, -10, 0);
		glVertex3f(0,  10, 0);
		glVertex3f(SCREEN_W - 200,  10, 0);
		glVertex3f(SCREEN_W - 200, -10, 0);
	glEnd();
	glPopMatrix();
	
	OptionBinding *binds = (OptionBinding*)menu->context;
	OptionBinding *bind;
	int i, caption_drawn = 0;

	for(i = 0; i < menu->ecount; i++) {
		float s = 0;
		
		if(menu->entries[i].action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.7);
		else if(i == menu->cursor)
		{
			glColor4f(1,1,0,0.7);
			s = 5*sin(menu->frames/80.0 + 20*i);
		}
		else
			glColor4f(1, 1, 1, 0.7);

		draw_text(AL_Left, 20 + s, 20*i, menu->entries[i].name, _fonts.standard);
		
		bind = &(binds[i]);
		
		if(bind->enabled)
		{
			int j, origin = SCREEN_W - 220;
			switch(bind->type)
			{
				case BT_IntValue:
					for(j = bind->valcount-1; j+1; --j)
					{
						if(j != bind->valcount-1)
							origin -= strlen(bind->values[j+1])/2.0 * 20;
						
						if(binding_getvalue(bind) == j)
							glColor4f(1,1,0,0.7);
						else
							glColor4f(0.5,0.5,0.5,0.7);
							
						draw_text(AL_Right, origin, 20*i, bind->values[j], _fonts.standard);
					}
					break;
				
				case BT_KeyBinding:
					if(!caption_drawn)
					{
						glColor4f(1,1,1,0.7);
						draw_text(AL_Center, (SCREEN_W - 200)/2, 20*(i-1), "Controls", _fonts.standard);
						caption_drawn = 1;
					}
				
					if(bind->blockinput)
					{
						glColor4f(0,1,0,0.7);
						draw_text(AL_Right, origin, 20*i, "Press a key to assign, ESC to cancel", _fonts.standard);
					}
					else
						draw_text(AL_Right, origin, 20*i, SDL_GetKeyName(tconfig.intval[bind->configentry]), _fonts.standard);
					break;
			}
		}
	}
	
	
	glPopMatrix();
	menu->drawdata[2] += (20*menu->cursor - menu->drawdata[2])/10.0;
	
	fade_out(menu->fade);
	
	glColor4f(1,1,1,1);	
}

// --- Input processing --- //

void binding_input(MenuData *menu, OptionBinding *b)
{
	// yes, no global_input() here.
	
	SDL_Event event;
	
	if(b->type != BT_KeyBinding) // shouldn't happen, but just in case.
	{
		b->blockinput = False;
		return;
	}
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		if(event.type == SDL_KEYDOWN) {
			if(sym != SDLK_ESCAPE)	// escape means cancel
				tconfig.intval[b->configentry] = sym;
			b->blockinput = False;
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
}

void options_menu_input(MenuData *menu) {
	SDL_Event event;
	OptionBinding *b;
	
	if((b = get_input_blocking_binding(menu)) != NULL)
	{
		binding_input(menu, b);
		return;
	}
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		global_processevent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(sym == tconfig.intval[KEY_DOWN]) {
				menu->drawdata[3] = 10;
				while(!menu->entries[++menu->cursor].action);
				
				if(menu->cursor >= menu->ecount)
					menu->cursor = 0;
				
			} else if(sym == tconfig.intval[KEY_UP]) {
				menu->drawdata[3] = 10;
				while(!menu->entries[--menu->cursor].action);
				
				if(menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
				
			} else if((sym == tconfig.intval[KEY_SHOT] || sym == SDLK_RETURN) && menu->entries[menu->cursor].action) {
				menu->selected = menu->cursor;
				
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled) switch(bind->type)
				{
					case BT_IntValue: binding_setnext(bind); break;
					case BT_KeyBinding: bind->blockinput = True; break;
				}
				else
					menu->quit = 1;
			} else if(sym == tconfig.intval[KEY_LEFT]) {
				menu->selected = menu->cursor;
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled && bind->type == BT_IntValue)
					binding_setprev(bind);
			} else if(sym == tconfig.intval[KEY_RIGHT]) {
				menu->selected = menu->cursor;
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled && bind->type == BT_IntValue)
					binding_setnext(bind);
			} else if(sym == SDLK_ESCAPE) {
				menu->quit = 2;
			}
			
			menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
}

int options_menu_loop(MenuData *menu) {
	return menu_loop(menu, options_menu_input, draw_options_menu);
}

