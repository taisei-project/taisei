/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdio.h>
#include "menu.h"
#include "options.h"
#include "global.h"
#include "video.h"
#include "paths/native.h"
#include "taisei_err.h"

// --- Menu entry <-> config option binding stuff --- //

void bind_init(OptionBinding *bind) {
	memset(bind, 0, sizeof(OptionBinding));
	bind->selected 	 	= -1;
	bind->configentry 	= -1;
}

OptionBinding* bind_new(void) {
	OptionBinding *bind = malloc(sizeof(OptionBinding));
	bind_init(bind);
	return bind;
}

void bind_free(OptionBinding *bind) {
	int i;
	
	if(bind->values) {
		for(i = 0; i < bind->valcount; ++i)
			free(*(bind->values+i));
		free(bind->values);
	}
}

OptionBinding* bind_get(MenuData *m, int idx) {
	MenuEntry *e = m->entries + idx;
	return e->arg == m? NULL : e->arg;
}

// Binds the last entry to an integer config option having limited values (BT_IntValue type binding).
// Values are defined with bind_addvalue.
OptionBinding* bind_option(int cfgentry, BindingGetter getter, BindingSetter setter) {
	OptionBinding *bind;
	bind = bind_new();
	
	bind->getter = getter;
	bind->setter = setter;
	bind->configentry = cfgentry;
	bind->type = BT_IntValue;
	
	return bind;
}

// Binds the last entry to a keybinding config option (BT_KeyBinding type binding).
OptionBinding* bind_keybinding(int cfgentry) {
	OptionBinding *bind;
	bind = bind_new();
	
	bind->configentry = cfgentry;
	bind->type = BT_KeyBinding;
	
	return bind;
}

// For string values, with a "textbox" editor
OptionBinding* bind_stroption(int cfgentry) {
	OptionBinding *bind;
	bind = bind_new();
	
	bind->configentry = cfgentry;
	bind->type = BT_StrValue;
	
	bind->valcount = 1;
	bind->values = malloc(sizeof(char*));
	*bind->values = malloc(128);
	strncpy(*bind->values, tconfig.strval[cfgentry], 128);
	
	return bind;
}

// Super-special binding type for the resolution setting
OptionBinding* bind_resolution(void) {
	OptionBinding *bind;
	bind = bind_new();
	
	bind->type = BT_Resolution;
	bind->valcount = video.mcount;
	bind->selected = -1;
	
	int i; for(i = 0; i < video.mcount; ++i) {
		VideoMode *m = &(video.modes[i]);
		
		if(m->width == video.intended.width && m->height == video.intended.height)
			bind->selected = i;
	}
	
	return bind;
}

// Returns a pointer to the first found binding that blocks input. If none found, returns NULL.
OptionBinding* bind_getinputblocking(MenuData *m) {
	int i;
	for(i = 0; i < m->ecount; ++i) {
		OptionBinding *bind = bind_get(m, i);
		if(bind && bind->blockinput)
			return bind;
	}
	return NULL;
}

// Adds a value to a BT_IntValue type binding
int bind_addvalue(OptionBinding *b, char *val) {
	b->values = realloc(b->values, ++b->valcount * sizeof(char*));
	b->values[b->valcount-1] = malloc(strlen(val) + 1);
	strcpy(b->values[b->valcount-1], val);
	return b->valcount-1;
}

void bind_setvaluerange(OptionBinding *b, int vmin, int vmax) {
	b->valrange_min = vmin;
	b->valrange_max = vmax;
}

// Called to select a value of a BT_IntValue type binding by index
int bind_setvalue(OptionBinding *b, int v) {
	if(b->setter)
		return b->selected = b->setter(b, v);
	else
		return b->selected = v;
}

// Called to get the selected value of a BT_IntValue type binding by index
int bind_getvalue(OptionBinding *b) {
	if(b->getter)
		// query AND update
		return b->selected = b->getter(b);
	else
		return b->selected;
}

// Selects the next to current value of a BT_IntValue type binding
int bind_setnext(OptionBinding *b) {
	int s = b->selected +1;
	
	if(b->valrange_max) {
		if(s > b->valrange_max)
			s = b->valrange_min;
	} else if(s >= b->valcount)
		s = 0;
	
	return bind_setvalue(b, s);
}

// Selects the previous to current value of a BT_IntValue type binding
int bind_setprev(OptionBinding *b) {
	int s = b->selected - 1;
	
	if(b->valrange_max) {
		if(s < b->valrange_min)
			s = b->valrange_max;
	} else if(s < 0)
		s = b->valcount - 1;
		
	return bind_setvalue(b, s);
}

void bind_setdependence(OptionBinding *b, BindingDependence dep) {
	b->dependence = dep;
}

int bind_isactive(OptionBinding *b) {
	if(!b->dependence)
		return True;
	return b->dependence();
}

// Initializes selection for all BT_IntValue type bindings
void bindings_initvalues(MenuData *m) {
	int i;
	for(i = 0; i < m->ecount; ++i) {
		OptionBinding *bind = bind_get(m, i);
		if(bind && bind->type == BT_IntValue)
			bind_getvalue(bind);
	}
}

// --- Shared binding callbacks --- //

int bind_common_onoffget(void *b) {
	return !tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset(void *b, int v) {
	return !(tconfig.intval[((OptionBinding*)b)->configentry] = !v);
}

int bind_common_onoffget_inverted(void *b) {
	return tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset_inverted(void *b, int v) {
	return tconfig.intval[((OptionBinding*)b)->configentry] = v;
}

#define bind_common_intget bind_common_onoffget_inverted
#define bind_common_intset bind_common_onoffset_inverted

// --- Binding callbacks for individual options --- //

int bind_fullscreen_set(void *b, int v) {
#ifndef WIN32	// TODO: remove when we're on SDL2
	video_toggle_fullscreen();
#endif
	return bind_common_onoffset(b, v);
}

int bind_noaudio_set(void *b, int v) {
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

int bind_noshader_set(void *b, int v) {
	int i = bind_common_onoffset_inverted(b, v);
	
	if(!v)
		load_resources();
	
	return i;
}

int bind_stagebg_fpslimit_dependence(void) {
	return tconfig.intval[NO_STAGEBG] == 2;
}

int bind_saverpy_get(void *b) {
	int v = tconfig.intval[((OptionBinding*)b)->configentry];
	
	if(v > 1)
		return v;
	return !v;
}

int bind_saverpy_set(void *b, int v) {
	if(v > 1)
		return tconfig.intval[((OptionBinding*)b)->configentry] = v;
	return !(tconfig.intval[((OptionBinding*)b)->configentry] = !v);
}

// --- Creating, destroying, filling the menu --- //

void destroy_options_menu(MenuData *m) {
	int i;
	
	for(i = 0; i < m->ecount; ++i) {
		OptionBinding *bind = bind_get(m, i);
		
		if(!bind)
			continue;
		
		if(bind->type == BT_Resolution) {
			if(bind->selected != -1) {
				VideoMode *m = video.modes + bind->selected;
				
#ifndef WIN32	// TODO: remove when we're on SDL2
				video_setmode(m->width, m->height, tconfig.intval[FULLSCREEN]);
#else
				video.intended.width = m->width;
				video.intended.height = m->height;
#endif
				
				tconfig.intval[VID_WIDTH]  = video.intended.width;
				tconfig.intval[VID_HEIGHT] = video.intended.height;
			}
			break;
		}
		
		bind_free(bind);
		free(bind);
	}
	
	config_save(CONFIG_FILE);
}

void do_nothing(void *arg) { }

void create_options_menu(MenuData *m) {
	OptionBinding *b;
	
	create_menu(m);
	m->flags = MF_Abortable;
	m->context = NULL;
	
	#define bind_onoff(b) bind_addvalue(b, "on"); bind_addvalue(b, "off")
	
	add_menu_entry(m, "Player Name", do_nothing,
		b = bind_stroption(PLAYERNAME)
	);
	
	add_menu_separator(m);
	
	add_menu_entry(m, "Video Mode", do_nothing, 
		b = bind_resolution()
	);
	
	add_menu_entry(m, "Fullscreen", do_nothing, 
		b = bind_option(FULLSCREEN, bind_common_onoffget, bind_fullscreen_set)
	);	bind_onoff(b);
	
	add_menu_entry(m, "Audio", do_nothing,
		b = bind_option(NO_AUDIO, bind_common_onoffget_inverted,
								  bind_noaudio_set)
	);	bind_onoff(b);
		
	add_menu_entry(m, "Shaders", do_nothing, 
		b = bind_option(NO_SHADER, bind_common_onoffget_inverted,
								   bind_noshader_set)
	);	bind_onoff(b);
			
	add_menu_entry(m, "Stage Background", do_nothing, 
		b = bind_option(NO_STAGEBG, bind_common_intget,
									bind_common_intset)
	);	bind_addvalue(b, "on");
		bind_addvalue(b, "off");
		bind_addvalue(b, "auto");
	
	add_menu_entry(m, "Minimum FPS", do_nothing, 
		b = bind_option(NO_STAGEBG_FPSLIMIT, bind_common_intget,
											 bind_common_intset)
	);	bind_setvaluerange(b, 20, 60);
		bind_setdependence(b, bind_stagebg_fpslimit_dependence);
	
	add_menu_entry(m, "Save Replays", do_nothing, 
		b = bind_option(SAVE_RPY, bind_saverpy_get,
								  bind_saverpy_set)
	);	bind_addvalue(b, "on");
		bind_addvalue(b, "off");
		bind_addvalue(b, "ask");
	
	add_menu_separator(m);
	
	add_menu_entry(m, "Move up", do_nothing, 
		bind_keybinding(KEY_UP)
	);
	
	add_menu_entry(m, "Move down", do_nothing, 
		bind_keybinding(KEY_DOWN)
	);
	
	add_menu_entry(m, "Move left", do_nothing, 
		bind_keybinding(KEY_LEFT)
	);
	
	add_menu_entry(m, "Move right", do_nothing, 
		bind_keybinding(KEY_RIGHT)
	);
	
	add_menu_separator(m);
	
	add_menu_entry(m, "Fire", do_nothing, 
		bind_keybinding(KEY_SHOT)
	);
		
	add_menu_entry(m, "Focus", do_nothing, 
		bind_keybinding(KEY_FOCUS)
	);
	
	add_menu_entry(m, "Bomb", do_nothing, 
		bind_keybinding(KEY_BOMB)
	);
		
	add_menu_separator(m);

#ifndef WIN32	// TODO: remove when we're on SDL2
	add_menu_entry(m, "Toggle fullscreen", do_nothing, 
		bind_keybinding(KEY_FULLSCREEN)
	);
#endif

	add_menu_entry(m, "Take a screenshot", do_nothing, 
		bind_keybinding(KEY_SCREENSHOT)
	);
	
	add_menu_entry(m, "Skip dialog", do_nothing, 
		bind_keybinding(KEY_SKIP)
	);
	
	add_menu_separator(m);
	add_menu_entry(m, "Return to the main menu", (MenuAction)kill_menu, m);
}

// --- Drawing the menu --- //

void draw_options_menu_bg(MenuData* menu) {
	//draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(0.3, 0.3, 0.3, 0.9 + 0.1 * sin(menu->frames/100.0));
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1, 1, 1, 1);
}

void draw_options_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Options");
	
	glPushMatrix();
	glTranslatef(100, 100, 0);
	
	draw_menu_selector(menu->drawdata[0], menu->drawdata[2], menu->drawdata[1]/100.0, 0.2, menu->frames);
	
	menu->drawdata[0] += ((SCREEN_W/2 - 100) - menu->drawdata[0])/10.0;
	menu->drawdata[1] += ((SCREEN_W - 200) - menu->drawdata[1])/10.0;
	menu->drawdata[2] += (20*menu->cursor - menu->drawdata[2])/10.0;
	
	int i, caption_drawn = 0;
	
	for(i = 0; i < menu->ecount; i++) {
		MenuEntry *e = menu->entries + i;
		OptionBinding *bind = bind_get(menu, i);
		
		if(!e->name)
			continue;
		
		e->drawdata += 0.2 * (10*(i == menu->cursor) - e->drawdata);
		float a = e->drawdata * 0.1;
		float alpha = (!bind || bind_isactive(bind))? 1 : 0.5;
		
		if(e->action == NULL) {
			glColor4f(0.5, 0.5, 0.5, 0.7 * alpha);
		} else {
			//glColor4f(0.7 + 0.3 * (1-a), 1, 1, (0.7 + 0.3 * a) * alpha);
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a) * alpha);
		}
		
		draw_text(AL_Left,
					((bind && bind->dependence)? 20 : 0)	// hack hack hack
					+ 20 - e->drawdata, 20*i, e->name, _fonts.standard);
		
		if(bind) {
			int j, origin = SCREEN_W - 220;
			
			switch(bind->type) {
				case BT_IntValue:
					if(bind->valrange_max) {
						char tmp[16];	// who'd use a 16-digit number here anyway?
						snprintf(tmp, 16, "%d", bind_getvalue(bind));
						draw_text(AL_Right, origin, 20*i, tmp, _fonts.standard);
					} else for(j = bind->valcount-1; j+1; --j) {
						if(j != bind->valcount-1)
							origin -= strlen(bind->values[j+1])/2.0 * 20;
						
						if(bind_getvalue(bind) == j) {
							glColor4f(0.9, 0.6, 0.2, alpha);
						} else {
							glColor4f(0.5,0.5,0.5,0.7 * alpha);
						}
							
						draw_text(AL_Right, origin, 20*i, bind->values[j], _fonts.standard);
					}
					break;
				
				case BT_KeyBinding:
					if(bind->blockinput) {
						glColor4f(0.5, 1, 0.5, 1);
						draw_text(AL_Right, origin, 20*i, "Press a key to assign, ESC to cancel", _fonts.standard);
					} else
						draw_text(AL_Right, origin, 20*i, SDL_GetKeyName(tconfig.intval[bind->configentry]), _fonts.standard);
					
					if(!caption_drawn) {
						glColor4f(1,1,1,0.7);
						draw_text(AL_Center, (SCREEN_W - 200)/2, 20*(i-1), "Controls", _fonts.standard);
						caption_drawn = 1;
					}
					break;
				
				case BT_StrValue:
					if(bind->blockinput) {
						glColor4f(0.5, 1, 0.5, 1.0);
						if(strlen(*bind->values))
							draw_text(AL_Right, origin, 20*i, *bind->values, _fonts.standard);
					} else
						draw_text(AL_Right, origin, 20*i, tconfig.strval[bind->configentry], _fonts.standard);
					break;
				
				case BT_Resolution: {
					char tmp[16];
					int w, h;
					
					if(bind->selected == -1) {
						w = video.intended.width;
						h = video.intended.height;
					} else {
						VideoMode *m = &(video.modes[bind->selected]);
						w = m->width;
						h = m->height;
					}
					
					snprintf(tmp, 16, "%dx%d", w, h);
					draw_text(AL_Right, origin, 20*i, tmp, _fonts.standard);
					break;
				}
			}
		}
	}
		
	glPopMatrix();
}

// --- Input processing --- //

void bind_input_event(EventType type, int state, void *arg) {
	OptionBinding *b = arg;
	
	int sym = state;
	char c = (char)(((Uint16)state) & 0x7F);
	char *dest = b->type == BT_StrValue? *b->values : NULL;
	
	switch(type) {
		case E_KeyDown:
			if(sym != SDLK_ESCAPE)
				tconfig.intval[b->configentry] = sym;
			b->blockinput = False;
		break;
		
		case E_CharTyped:
			if(c != ':') {
				char s[] = {c, 0};
				strncat(dest, s, 128);
			}
		break;
		
		case E_CharErased:
			if(strlen(dest))
				dest[strlen(dest)-1] = 0;
		break;
		
		case E_SubmitText:
			if(strlen(dest))
				stralloc(&(tconfig.strval[b->configentry]), dest);
			else
				strncpy(dest, tconfig.strval[b->configentry], 128);
			b->blockinput = False;
		break;
		
		case E_CancelText:
			strncpy(dest, tconfig.strval[b->configentry], 128);
			b->blockinput = False;
		break;
		
		default: break;
	}
}

// raw access to arg is safe there after the bind_get check
#define SHOULD_SKIP (!menu->entries[menu->cursor].action || (bind_get(menu, menu->cursor) && !bind_isactive(menu->entries[menu->cursor].arg)))

static void options_input_event(EventType type, int state, void *arg) {
	MenuData *menu = arg;
	OptionBinding *bind = bind_get(menu, menu->cursor);
	
	switch(type) {
		case E_CursorDown:
			menu->drawdata[3] = 10;
			do {
				menu->cursor++;
				if(menu->cursor >= menu->ecount)
					menu->cursor = 0;
			} while SHOULD_SKIP;
		break;
		
		case E_CursorUp:
			menu->drawdata[3] = 10;
			do {
				menu->cursor--;
				if(menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
			} while SHOULD_SKIP;
		break;
		
		case E_CursorLeft:
			if(bind && (bind->type == BT_IntValue || bind->type == BT_Resolution))
				bind_setprev(bind);
		break;
		
		case E_CursorRight:
			if(bind && (bind->type == BT_IntValue || bind->type == BT_Resolution))
				bind_setnext(bind);
		break;
		
		case E_MenuAccept:
			menu->selected = menu->cursor;
			
			if(bind) switch(bind->type) {
				case BT_KeyBinding:
					bind->blockinput = True; 
					break;
					
				case BT_StrValue:
					bind->selected = strlen(tconfig.strval[bind->configentry]);
					bind->blockinput = True;
					break;
					
				default:
					break;
			} else close_menu(menu);
		break;
		
		case E_MenuAbort:
			menu->selected = -1;
			close_menu(menu);
		break;
		
		default: break;
	}
	
	menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
}
#undef SHOULD_SKIP

void options_menu_input(MenuData *menu) {
	OptionBinding *b;
	
	if((b = bind_getinputblocking(menu)) != NULL)
		handle_events(bind_input_event, b->type == BT_StrValue? EF_Text : EF_Keyboard, b);
	else
		handle_events(options_input_event, EF_Menu, menu);
}

int options_menu_loop(MenuData *menu) {
	return menu_loop(menu, options_menu_input, draw_options_menu, destroy_options_menu);
}

