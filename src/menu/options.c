/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdio.h>
#include "menu.h"
#include "common.h"
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

// BT_IntValue: integer and boolean options
// Values are defined with bind_addvalue or bind_setrange
OptionBinding* bind_option(int cfgentry, BindingGetter getter, BindingSetter setter) {
	OptionBinding *bind = bind_new();
	bind->type = BT_IntValue;
	bind->configentry = cfgentry;

	bind->getter = getter;
	bind->setter = setter;

	return bind;
}

// BT_KeyBinding: keyboard action mapping options
OptionBinding* bind_keybinding(int cfgentry) {
	OptionBinding *bind = bind_new();

	bind->configentry = cfgentry;
	bind->type = BT_KeyBinding;

	return bind;
}

// BT_GamepadKeyBinding: gamepad action mapping options
OptionBinding* bind_gpbinding(int cfgentry) {
	OptionBinding *bind = bind_new();

	bind->configentry = cfgentry;
	bind->type = BT_GamepadKeyBinding;

	return bind;
}

// BT_GamepadAxisBinding: gamepad axis mapping options
OptionBinding* bind_gpaxisbinding(int cfgentry) {
	OptionBinding *bind = bind_new();

	bind->configentry = cfgentry;
	bind->type = BT_GamepadAxisBinding;

	return bind;
}

// BT_StrValue: with a half-assed "textbox"
OptionBinding* bind_stroption(ConfigIndex cfgentry) {
	OptionBinding *bind = bind_new();
	bind->type = BT_StrValue;
	bind->configentry = cfgentry;

	bind->valcount = 1;
	bind->values = malloc(sizeof(char*));
	*bind->values = malloc(OPTIONS_TEXT_INPUT_BUFSIZE);
	strlcpy(*bind->values, config_get_str(cfgentry), OPTIONS_TEXT_INPUT_BUFSIZE);

	return bind;
}

// BT_Resolution: super-special binding type for the resolution setting
OptionBinding* bind_resolution(void) {
	OptionBinding *bind = bind_new();
	bind->type = BT_Resolution;

	bind->valcount = video.mcount;
	bind->selected = -1;

	for(int i = 0; i < video.mcount; ++i) {
		VideoMode *m = video.modes + i;
		if(m->width == video.current.width && m->height == video.current.height) {
			bind->selected = i;
		}
	}

	return bind;
}

// BT_Scale: float values clamped to a range
OptionBinding* bind_scale(int cfgentry, float smin, float smax, float step) {
	OptionBinding *bind = bind_new();
	bind->type = BT_Scale;
	bind->configentry = cfgentry;

	bind->scale_min  = smin;
	bind->scale_max  = smax;
	bind->scale_step = step;

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
	if(b->getter) {
		if(b->selected >= b->valcount && b->valcount) {
			b->selected = 0;
		} else {
			b->selected = b->getter(b);

			if(b->selected >= b->valcount && b->valcount)
				b->selected = 0;
		}
	}

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

bool bind_isactive(OptionBinding *b) {
	if(!b->dependence)
		return true;
	return b->dependence();
}

// --- Shared binding callbacks --- //

int bind_common_onoffget(void *b) {
	return !config_get_int(((OptionBinding*)b)->configentry);
}

int bind_common_onoffset(void *b, int v) {
	return !config_set_int(((OptionBinding*)b)->configentry, !v);
}

int bind_common_onoffget_inverted(void *b) {
	return config_get_int(((OptionBinding*)b)->configentry);
}

int bind_common_onoffset_inverted(void *b, int v) {
	return config_set_int(((OptionBinding*)b)->configentry, v);
}

#define bind_common_intget bind_common_onoffget_inverted
#define bind_common_intset bind_common_onoffset_inverted

// --- Binding callbacks for individual options --- //

bool bind_stagebg_fpslimit_dependence(void) {
	return config_get_int(CONFIG_NO_STAGEBG) == 2;
}

bool bind_sfxvol_dependence(void) {
	return !config_get_int(CONFIG_NO_AUDIO);
}

bool bind_bgmvol_dependence(void) {
	return !config_get_int(CONFIG_NO_MUSIC);
}

bool bind_resizable_dependence(void) {
	return !config_get_int(CONFIG_FULLSCREEN);
}

int bind_saverpy_get(void *b) {
	int v = config_get_int(((OptionBinding*)b)->configentry);

	if(v > 1)
		return v;
	return !v;
}

int bind_saverpy_set(void *b, int v) {
	if(v > 1)
		return config_set_int(((OptionBinding*)b)->configentry, v);
	return !config_set_int(((OptionBinding*)b)->configentry, !v);
}

int bind_resolution_set(void *b, int v) {
	if(v >= 0) {
		VideoMode *m = video.modes + v;
		config_set_int(CONFIG_VID_WIDTH, m->width);
		config_set_int(CONFIG_VID_HEIGHT, m->height);
	}

	return v;
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

				video_setmode(m->width, m->height,
					config_get_int(CONFIG_FULLSCREEN),
					config_get_int(CONFIG_VID_RESIZABLE)
				);

				config_set_int(CONFIG_VID_WIDTH, video.intended.width);
				config_set_int(CONFIG_VID_HEIGHT, video.intended.height);
			}
			break;
		}

		bind_free(bind);
		free(bind);
	}

	//we call this in taisei_shutdown instead
	//config_save(CONFIG_FILE);
}

static void do_nothing(MenuData *menu, void *arg) { }

void options_menu_input(MenuData*);

void create_options_menu_basic(MenuData *m, char *s) {
	create_menu(m);
	m->transition = TransMenuDark;
	m->flags = MF_Abortable;
	m->context = s;
	m->input = options_menu_input;
	m->draw = draw_options_menu;
	m->end = destroy_options_menu;
}

#define bind_onoff(b) bind_addvalue(b, "on"); bind_addvalue(b, "off")

void options_sub_video(MenuData *parent, void *arg) {
	MenuData menu, *m;
	OptionBinding *b;
	m = &menu;

	create_options_menu_basic(m, "Video Options");

	add_menu_entry(m, "Resolution", do_nothing,
		b = bind_resolution()
	);	b->setter = bind_resolution_set;

	add_menu_entry(m, "Fullscreen", do_nothing,
		b = bind_option(CONFIG_FULLSCREEN, bind_common_onoffget, bind_common_onoffset)
	);	bind_onoff(b);

	add_menu_entry(m, "Resizable window", do_nothing,
		b = bind_option(CONFIG_VID_RESIZABLE, bind_common_onoffget, bind_common_onoffset)
	);	bind_onoff(b);
		bind_setdependence(b, bind_resizable_dependence);

	add_menu_entry(m, "Vertical synchronization", do_nothing,
		b = bind_option(CONFIG_VSYNC, bind_common_intget, bind_common_intset)
	);	bind_addvalue(b, "on");
		bind_addvalue(b, "off");
		bind_addvalue(b, "adaptive");

	add_menu_separator(m);

#ifdef DEBUG
	add_menu_entry(m, "Shaders", do_nothing,
		b = bind_option(CONFIG_NO_SHADER, bind_common_onoffget_inverted, bind_common_onoffset_inverted)
	);	bind_onoff(b);
#endif

	add_menu_entry(m, "Stage background", do_nothing,
		b = bind_option(CONFIG_NO_STAGEBG, bind_common_intget, bind_common_intset)
	);	bind_addvalue(b, "on");
		bind_addvalue(b, "off");
		bind_addvalue(b, "auto");

	add_menu_entry(m, "Minimum FPS", do_nothing,
		b = bind_option(CONFIG_NO_STAGEBG_FPSLIMIT, bind_common_intget, bind_common_intset)
	);	bind_setvaluerange(b, 20, 60);
		bind_setdependence(b, bind_stagebg_fpslimit_dependence);
		b->pad++;

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	menu_loop(m);
	parent->frames = 0;
}

void bind_setvaluerange_fancy(OptionBinding *b, int ma) {
	int i = 0; for(i = 0; i <= ma; ++i) {
		char tmp[16];
		snprintf(tmp, 16, "%i", i);
		bind_addvalue(b, tmp);
	}
}

bool gamepad_sens_depencence(void) {
	return config_get_int(CONFIG_GAMEPAD_AXIS_FREE);
}

void options_sub_gamepad_controls(MenuData *parent, void *arg) {
	MenuData menu, *m;
	m = &menu;

	create_options_menu_basic(m, "Gamepad Controls");

	add_menu_entry(m, "Fire / Accept", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_SHOT)
	);

	add_menu_entry(m, "Focus / Abort", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_FOCUS)
	);

	add_menu_entry(m, "Bomb", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_BOMB)
	);

	add_menu_separator(m);

	add_menu_entry(m, "Pause", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_PAUSE)
	);

	add_menu_entry(m, "Skip dialog", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_SKIP)
	);

	add_menu_separator(m);

	add_menu_entry(m, "Move up", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_UP)
	);

	add_menu_entry(m, "Move down", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_DOWN)
	);

	add_menu_entry(m, "Move left", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_LEFT)
	);

	add_menu_entry(m, "Move right", do_nothing,
		bind_gpbinding(CONFIG_GAMEPAD_KEY_RIGHT)
	);

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	menu_loop(m);
	parent->frames = 0;
	gamepad_restart();
}

void options_sub_gamepad(MenuData *parent, void *arg) {
	MenuData menu, *m;
	OptionBinding *b;
	m = &menu;

	create_options_menu_basic(m, "Gamepad Options");

	add_menu_entry(m, "Enable Gamepad/Joystick support", do_nothing,
		b = bind_option(CONFIG_GAMEPAD_ENABLED, bind_common_onoffget, bind_common_onoffset)
	);	bind_onoff(b);

	add_menu_entry(m, "Device", do_nothing,
		b = bind_option(CONFIG_GAMEPAD_DEVICE, bind_common_intget, bind_common_intset)
	); b->displaysingle = true;

	add_menu_separator(m);
	add_menu_entry(m, "Customize controls…", options_sub_gamepad_controls, NULL);

	gamepad_init_bare();
	int cnt = gamepad_devicecount();
	int i; for(i = 0; i < cnt; ++i) {
		char buf[50];
		snprintf(buf, sizeof(buf), "#%i: %s", i+1, gamepad_devicename(i));
		bind_addvalue(b, buf);
	}

	if(!i) {
		bind_addvalue(b, "No devices available");
		b->selected = 0;
	}
	gamepad_shutdown_bare();

	add_menu_separator(m);

	add_menu_entry(m, "X axis", do_nothing,
		b = bind_gpaxisbinding(CONFIG_GAMEPAD_AXIS_LR)
	);

	add_menu_entry(m, "Y axis", do_nothing,
		b = bind_gpaxisbinding(CONFIG_GAMEPAD_AXIS_UD)
	);

	add_menu_entry(m, "Axes mode", do_nothing,
		b = bind_option(CONFIG_GAMEPAD_AXIS_FREE, bind_common_onoffget, bind_common_onoffset)
	);	bind_addvalue(b, "free");
		bind_addvalue(b, "restricted");

	add_menu_entry(m, "X axis sensitivity", do_nothing,
		b = bind_scale(CONFIG_GAMEPAD_AXIS_LR_SENS, -2, 2, 0.05)
	);	bind_setdependence(b, gamepad_sens_depencence);
		b->pad++;

	add_menu_entry(m, "Y axis sensitivity", do_nothing,
		b = bind_scale(CONFIG_GAMEPAD_AXIS_UD_SENS, -2, 2, 0.05)
	); 	bind_setdependence(b, gamepad_sens_depencence);
		b->pad++;

	add_menu_entry(m, "Dead zone", do_nothing,
		b = bind_scale(CONFIG_GAMEPAD_AXIS_DEADZONE, 0, 1, 0.01)
	);

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	menu_loop(m);
	parent->frames = 0;
	gamepad_restart();
}

void options_sub_controls(MenuData *parent, void *arg) {
	MenuData menu, *m;
	m = &menu;

	create_options_menu_basic(m, "Controls");

	add_menu_entry(m, "Move up", do_nothing,
		bind_keybinding(CONFIG_KEY_UP)
	);

	add_menu_entry(m, "Move down", do_nothing,
		bind_keybinding(CONFIG_KEY_DOWN)
	);

	add_menu_entry(m, "Move left", do_nothing,
		bind_keybinding(CONFIG_KEY_LEFT)
	);

	add_menu_entry(m, "Move right", do_nothing,
		bind_keybinding(CONFIG_KEY_RIGHT)
	);

	add_menu_separator(m);

	add_menu_entry(m, "Fire", do_nothing,
		bind_keybinding(CONFIG_KEY_SHOT)
	);

	add_menu_entry(m, "Focus", do_nothing,
		bind_keybinding(CONFIG_KEY_FOCUS)
	);

	add_menu_entry(m, "Bomb", do_nothing,
		bind_keybinding(CONFIG_KEY_BOMB)
	);

	add_menu_separator(m);

	add_menu_entry(m, "Toggle fullscreen", do_nothing,
		bind_keybinding(CONFIG_KEY_FULLSCREEN)
	);

	add_menu_entry(m, "Take a screenshot", do_nothing,
		bind_keybinding(CONFIG_KEY_SCREENSHOT)
	);

	add_menu_entry(m, "Skip dialog", do_nothing,
		bind_keybinding(CONFIG_KEY_SKIP)
	);

#ifdef DEBUG
	add_menu_separator(m);

	add_menu_entry(m, "Toggle God mode", do_nothing,
		bind_keybinding(CONFIG_KEY_IDDQD)
	);

	add_menu_entry(m, "Skip stage", do_nothing,
		bind_keybinding(CONFIG_KEY_HAHAIWIN)
	);

	add_menu_entry(m, "Disable background rendering (HoM effect)", do_nothing,
		bind_keybinding(CONFIG_KEY_NOBACKGROUND)
	);
#endif

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_commonaction_close, NULL);

	menu_loop(m);
	parent->frames = 0;
}

void create_options_menu(MenuData *m) {
	OptionBinding *b;

	create_options_menu_basic(m, "Options");

	add_menu_entry(m, "Player name", do_nothing,
		b = bind_stroption(CONFIG_PLAYERNAME)
	);

	add_menu_separator(m);

	add_menu_entry(m, "Save replays", do_nothing,
		b = bind_option(CONFIG_SAVE_RPY, bind_saverpy_get, bind_saverpy_set)
	);	bind_addvalue(b, "on");
		bind_addvalue(b, "off");
		bind_addvalue(b, "ask");

	add_menu_entry(m, "Auto-restart in spell practice mode", do_nothing,
		b = bind_option(CONFIG_SPELLSTAGE_AUTORESTART, 	bind_common_onoffget,
														bind_common_onoffset)
	);	bind_onoff(b);

	add_menu_separator(m);

	add_menu_entry(m, "Sound effects", do_nothing,
		b = bind_option(CONFIG_NO_AUDIO, 	bind_common_onoffget_inverted,
											bind_common_onoffset_inverted)
	);	bind_onoff(b);

	add_menu_entry(m, "Volume", do_nothing,
		b = bind_scale(CONFIG_SFX_VOLUME, 0, 1, 0.1)
	);	bind_setdependence(b, bind_sfxvol_dependence);
		b->pad++;

	add_menu_separator(m);

	add_menu_entry(m, "Background music", do_nothing,
		b = bind_option(CONFIG_NO_MUSIC,	bind_common_onoffget_inverted,
											bind_common_onoffset_inverted)
	);	bind_onoff(b);

	add_menu_entry(m, "Volume", do_nothing,
		b = bind_scale(CONFIG_BGM_VOLUME, 0, 1, 0.1)
	);	bind_setdependence(b, bind_bgmvol_dependence);
		b->pad++;

	add_menu_separator(m);
	add_menu_entry(m, "Video options…", options_sub_video, NULL);
	add_menu_entry(m, "Customize controls…", options_sub_controls, NULL);
	add_menu_entry(m, "Gamepad & Joystick options…", options_sub_gamepad, NULL);
	add_menu_separator(m);

	add_menu_entry(m, "Back", menu_commonaction_close, NULL);
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
	draw_menu_title(menu, menu->context);

	glPushMatrix();
	glTranslatef(100, 100, 0);

	draw_menu_selector(menu->drawdata[0], menu->drawdata[2], menu->drawdata[1], 34, menu->frames);

	menu->drawdata[0] += ((SCREEN_W/2 - 100) - menu->drawdata[0])/10.0;
	menu->drawdata[1] += ((SCREEN_W - 200) * 1.75 - menu->drawdata[1])/10.0;
	menu->drawdata[2] += (20*menu->cursor - menu->drawdata[2])/10.0;

	int i, caption_drawn = 2;

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

		draw_text(AL_Left, (1 + (bind ? bind->pad : 0)) * 20 - e->drawdata, 20*i, e->name, _fonts.standard);

		if(bind) {
			int j, origin = SCREEN_W - 220;

			if(caption_drawn == 2 && bind->type != BT_KeyBinding)
				caption_drawn = 0;

			switch(bind->type) {
				case BT_IntValue: {
					int val = bind_getvalue(bind);

					if(bind->valrange_max) {
						char tmp[16];	// who'd use a 16-digit number here anyway?
						snprintf(tmp, 16, "%d", bind_getvalue(bind));
						draw_text(AL_Right, origin, 20*i, tmp, _fonts.standard);
					} else for(j = bind->displaysingle? val : bind->valcount-1; (j+1)*(!bind->displaysingle || j == val); --j) {
						if(j != bind->valcount-1 && !bind->displaysingle)
							origin -= stringwidth(bind->values[j+1], _fonts.standard) + 5;

						if(val == j) {
							glColor4f(0.9, 0.6, 0.2, alpha);
						} else {
							glColor4f(0.5,0.5,0.5,0.7 * alpha);
						}

						draw_text(AL_Right, origin, 20*i, bind->values[j], _fonts.standard);
					}
					break;
				}

				case BT_KeyBinding: {
					if(bind->blockinput) {
						glColor4f(0.5, 1, 0.5, 1);
						draw_text(AL_Right, origin, 20*i, "Press a key to assign, ESC to cancel", _fonts.standard);
					} else {
						const char *txt = SDL_GetScancodeName(config_get_int(bind->configentry));

						if(!txt || !*txt) {
							txt = "Unknown";
						}

						draw_text(AL_Right, origin, 20*i, txt, _fonts.standard);
					}

					if(!caption_drawn) {
						glColor4f(1,1,1,0.7);
						draw_text(AL_Center, (SCREEN_W - 200)/2, 20*(i-1), "Controls", _fonts.standard);
						caption_drawn = 1;
					}
					break;
				}

				case BT_GamepadKeyBinding:
				case BT_GamepadAxisBinding: {
					bool is_axis = (bind->type == BT_GamepadAxisBinding);

					if(bind->blockinput) {
						glColor4f(0.5, 1, 0.5, 1);
						draw_text(AL_Right, origin, 20*i,
							is_axis ? "Move an axis to assign, press any button to cancel"
									: "Press a button to assign, move an axis to cancel",
							_fonts.standard);
					} else if(config_get_int(bind->configentry) >= 0) {
						char tmp[32];
						snprintf(tmp, 32, is_axis ? "Axis %i" : "Button %i", config_get_int(bind->configentry) + 1);
						draw_text(AL_Right, origin, 20*i, tmp, _fonts.standard);
					} else {
						draw_text(AL_Right, origin, 20*i, "Unbound", _fonts.standard);
					}
					break;
				}

				case BT_StrValue: {
					if(bind->blockinput) {
						glColor4f(0.5, 1, 0.5, 1.0);
						if(strlen(*bind->values))
							draw_text(AL_Right, origin, 20*i, *bind->values, _fonts.standard);
					} else
						draw_text(AL_Right, origin, 20*i, config_get_str(bind->configentry), _fonts.standard);
					break;
				}

				case BT_Resolution: {
					char tmp[16];
					int w, h;

					if(bind->selected == -1) {
						w = video.intended.width;
						h = video.intended.height;
					} else {
						VideoMode *m = video.modes + bind->selected;
						w = m->width;
						h = m->height;
					}

					snprintf(tmp, 16, "%dx%d", w, h);
					draw_text(AL_Right, origin, 20*i, tmp, _fonts.standard);
					break;
				}

				case BT_Scale: {
					int w  = 200;
					int h  = 5;
					int cw = 5;

					float val = config_get_float(bind->configentry);

					float ma = bind->scale_max;
					float mi = bind->scale_min;
					float pos = (val - mi) / (ma - mi);

					char tmp[8];
					snprintf(tmp, 8, "%.0f%%", 100 * val);
					if(!strcmp(tmp, "-0%"))
						strcpy(tmp, "0%");

					glPushMatrix();
					glTranslatef(origin - (w+cw) * 0.5, 20 * i, 0);
					draw_text(AL_Right, -((w+cw) * 0.5 + 10), 0, tmp, _fonts.standard);
					glPushMatrix();
					glScalef(w+cw, h, 1);
					glColor4f(1, 1, 1, (0.1 + 0.2 * a) * alpha);
					draw_quad();
					glPopMatrix();
					glTranslatef(w * (pos - 0.5), 0, 0);
					glScalef(cw, h, 0);
					glColor4f(0.9, 0.6, 0.2, alpha);
					draw_quad();
					glPopMatrix();

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

	int scan = state;
	char c = (char)(((Uint16)state) & 0x7F);
	char *dest = b->type == BT_StrValue? *b->values : NULL;

	switch(type) {
		case E_KeyDown: {
			int esc = scan == SDL_SCANCODE_ESCAPE;
			if(b->type == BT_GamepadKeyBinding || b->type == BT_GamepadAxisBinding) {
				if(esc)
					b->blockinput = false;
				break;
			}

			if(!esc) {
				for(int i = CONFIG_KEY_FIRST; i <= CONFIG_KEY_LAST; ++i) {
					if(config_get_int(i) == scan) {
						config_set_int(i, config_get_int(b->configentry));
					}
				}

				config_set_int(b->configentry, scan);
			}

			b->blockinput = false;
			break;
		}

		case E_GamepadKeyDown: {
			if(b->type != BT_GamepadKeyBinding) {
				if(b->type == BT_GamepadAxisBinding)
					b->blockinput = false;
				break;
			}

			for(int i = CONFIG_GAMEPAD_KEY_FIRST; i <= CONFIG_GAMEPAD_KEY_LAST; ++i) {
				if(config_get_int(i) == scan) {
					config_set_int(i, config_get_int(b->configentry));
				}
			}

			config_set_int(b->configentry, scan);
			b->blockinput = false;
			break;
		}

		case E_GamepadAxis: {
			if(b->type == BT_GamepadKeyBinding) {
				b->blockinput = false;
				break;
			}

			if(b->type == BT_GamepadAxisBinding) {
				if(b->configentry == CONFIG_GAMEPAD_AXIS_UD) {
					if(config_get_int(CONFIG_GAMEPAD_AXIS_LR) == state) {
						config_set_int(CONFIG_GAMEPAD_AXIS_LR, config_get_int(CONFIG_GAMEPAD_AXIS_UD));
					}
				} else if(b->configentry == CONFIG_GAMEPAD_AXIS_LR) {
					if(config_get_int(CONFIG_GAMEPAD_AXIS_UD) == state) {
						config_set_int(CONFIG_GAMEPAD_AXIS_UD, config_get_int(CONFIG_GAMEPAD_AXIS_LR));
					}
				}

				config_set_int(b->configentry, state);
				b->blockinput = false;
			}

			break;
		}

		case E_CharTyped: {
			if(c != ':') {
				char s[] = {c, 0};
				strlcat(dest, s, OPTIONS_TEXT_INPUT_BUFSIZE);
			}
			break;
		}

		case E_CharErased: {
			if(dest != NULL && strlen(dest))
				dest[strlen(dest)-1] = 0;
			break;
		}

		case E_SubmitText: {
			if(dest != NULL && strlen(dest))
				config_set_str(b->configentry, dest);
			else
				strlcpy(dest, config_get_str(b->configentry), OPTIONS_TEXT_INPUT_BUFSIZE);
			b->blockinput = false;
			break;
		}

		case E_CancelText: {
			strlcpy(dest, config_get_str(b->configentry), OPTIONS_TEXT_INPUT_BUFSIZE);
			b->blockinput = false;
			break;
		}

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
			play_ui_sound("generic_shot");
			menu->drawdata[3] = 10;
			do {
				menu->cursor++;
				if(menu->cursor >= menu->ecount)
					menu->cursor = 0;
			} while SHOULD_SKIP;
		break;

		case E_CursorUp:
			play_ui_sound("generic_shot");
			menu->drawdata[3] = 10;
			do {
				menu->cursor--;
				if(menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
			} while SHOULD_SKIP;
		break;

		case E_CursorLeft:
			play_ui_sound("generic_shot");
			if(bind) {
				if(bind->type == BT_IntValue || bind->type == BT_Resolution)
					bind_setprev(bind);
				else if(bind->type == BT_Scale) {
					config_set_float(bind->configentry, clamp(config_get_float(bind->configentry) - bind->scale_step, bind->scale_min, bind->scale_max));
				}
			}
		break;

		case E_CursorRight:
			play_ui_sound("generic_shot");
			if(bind) {
				if(bind->type == BT_IntValue || bind->type == BT_Resolution)
					bind_setnext(bind);
				else if(bind->type == BT_Scale) {
					config_set_float(bind->configentry, clamp(config_get_float(bind->configentry) + bind->scale_step, bind->scale_min, bind->scale_max));
				}
			}
		break;

		case E_MenuAccept:
			play_ui_sound("shot_special1");
			menu->selected = menu->cursor;

			if(bind) switch(bind->type) {
				case BT_KeyBinding:
				case BT_GamepadKeyBinding:
				case BT_GamepadAxisBinding:
					bind->blockinput = true;
					break;

				case BT_StrValue:
					bind->selected = strlen(config_get_str(bind->configentry));
					bind->blockinput = true;
					break;

				default:
					break;
			} else close_menu(menu);
		break;

		case E_MenuAbort:
			play_ui_sound("hit");
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

	if((b = bind_getinputblocking(menu)) != NULL) {
		int flags = 0;

		switch(b->type) {
			case BT_StrValue:			flags = EF_Text;					break;
			case BT_KeyBinding:			flags = EF_Keyboard;				break;
			case BT_GamepadKeyBinding:	flags = EF_Gamepad | EF_Keyboard;	break;
			case BT_GamepadAxisBinding:	flags = EF_Gamepad | EF_Keyboard;	break;
			default: break;
		}

		handle_events(bind_input_event, flags, b);
	} else
		handle_events(options_input_event, EF_Menu, menu);
}
