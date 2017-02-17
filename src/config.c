/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <string.h>

#include "config.h"
#include "global.h"
#include "paths/native.h"
#include "taisei_err.h"

ConfigEntry configdefs[] = {
	{CFGT_KEYBINDING,			KEY_UP,					"key_up"},
	{CFGT_KEYBINDING,			KEY_DOWN,				"key_down"},
	{CFGT_KEYBINDING,			KEY_LEFT,				"key_left"},
	{CFGT_KEYBINDING,			KEY_RIGHT,				"key_right"},

	{CFGT_KEYBINDING,			KEY_FOCUS,				"key_focus"},
	{CFGT_KEYBINDING,			KEY_SHOT,				"key_shot"},
	{CFGT_KEYBINDING,			KEY_BOMB,				"key_bomb"},

	{CFGT_KEYBINDING,			KEY_FULLSCREEN,			"key_fullscreen"},
	{CFGT_KEYBINDING,			KEY_SCREENSHOT,			"key_screenshot"},
	{CFGT_KEYBINDING,			KEY_SKIP,				"key_skip"},

	{CFGT_KEYBINDING,			KEY_IDDQD,				"key_iddqd"},
	{CFGT_KEYBINDING,			KEY_HAHAIWIN,			"key_skipstage"},

	{CFGT_INT,					FULLSCREEN,				"fullscreen"},
	{CFGT_INT,					NO_SHADER,				"disable_shader"},
	{CFGT_INT,					NO_AUDIO,				"disable_audio"},
	{CFGT_INT,					NO_MUSIC,				"disable_bgm"},
	{CFGT_FLOAT,				SFX_VOLUME,				"sfx_volume"},
	{CFGT_FLOAT,				BGM_VOLUME,				"bgm_volume"},
	{CFGT_INT,					NO_STAGEBG,				"disable_stagebg"},
	{CFGT_INT,					NO_STAGEBG_FPSLIMIT,	"disable_stagebg_auto_fpslimit"},
	{CFGT_INT,					SAVE_RPY,				"save_rpy"},
	{CFGT_INT,					VID_WIDTH,				"vid_width"},
	{CFGT_INT,					VID_HEIGHT,				"vid_height"},
	{CFGT_STRING,				PLAYERNAME,				"playername"},

	{CFGT_INT,					GAMEPAD_ENABLED,		"gamepad_enabled"},
	{CFGT_INT,					GAMEPAD_DEVICE,			"gamepad_device"},
	{CFGT_INT,					GAMEPAD_AXIS_UD,		"gamepad_axis_ud"},
	{CFGT_INT,					GAMEPAD_AXIS_LR,		"gamepad_axis_lr"},
	{CFGT_FLOAT,				GAMEPAD_AXIS_DEADZONE,	"gamepad_axis_deadzone"},
	{CFGT_INT,					GAMEPAD_AXIS_FREE,		"gamepad_axis_free"},
	{CFGT_FLOAT,				GAMEPAD_AXIS_UD_SENS,	"gamepad_axis_ud_free_sensitivity"},
	{CFGT_FLOAT,				GAMEPAD_AXIS_LR_SENS,	"gamepad_axis_lr_free_sensitivity"},

	// gamepad controls
	{CFGT_INT,					GP_UP,					"gamepad_key_up"},
	{CFGT_INT,					GP_DOWN,				"gamepad_key_down"},
	{CFGT_INT,					GP_LEFT,				"gamepad_key_left"},
	{CFGT_INT,					GP_RIGHT,				"gamepad_key_right"},

	{CFGT_INT,					GP_FOCUS,				"gamepad_key_focus"},
	{CFGT_INT,					GP_SHOT,				"gamepad_key_shot"},
	{CFGT_INT,					GP_BOMB,				"gamepad_key_bomb"},

	{CFGT_INT,					GP_SKIP,				"gamepad_key_skip"},
	{CFGT_INT,					GP_PAUSE,				"gamepad_key_pause"},

	{CFGT_INT,					VSYNC,					"vsync"},

	{0, 0, 0}
};

ConfigEntry* config_findentry(char *name) {
	ConfigEntry *e = configdefs;
	do if(!strcmp(e->name, name)) return e; while((++e)->name);
	return NULL;
}

ConfigEntry* config_findentry_byid(int id) {
	ConfigEntry *e = configdefs;
	do if(id == e->key) return e; while((++e)->name);
	return NULL;
}

void config_preset(void) {
	memset(tconfig.strval, 0, sizeof(tconfig.strval));
	memset(tconfig.intval, 0, sizeof(tconfig.intval));
	memset(tconfig.fltval, 0, sizeof(tconfig.fltval));

	tconfig.intval[KEY_UP] = SDL_SCANCODE_UP;
	tconfig.intval[KEY_DOWN] = SDL_SCANCODE_DOWN;
	tconfig.intval[KEY_LEFT] = SDL_SCANCODE_LEFT;
	tconfig.intval[KEY_RIGHT] = SDL_SCANCODE_RIGHT;

	tconfig.intval[KEY_FOCUS] = SDL_SCANCODE_LSHIFT;
	tconfig.intval[KEY_SHOT] = SDL_SCANCODE_Z;
	tconfig.intval[KEY_BOMB] = SDL_SCANCODE_X;

	tconfig.intval[KEY_FULLSCREEN] = SDL_SCANCODE_F11;
	tconfig.intval[KEY_SCREENSHOT] = SDL_SCANCODE_P;
	tconfig.intval[KEY_SKIP] = SDL_SCANCODE_LCTRL;

	tconfig.intval[KEY_IDDQD] = SDL_SCANCODE_Q;
	tconfig.intval[KEY_HAHAIWIN] = SDL_SCANCODE_E;

	tconfig.intval[FULLSCREEN] = 0;

	tconfig.intval[NO_SHADER] = 0;
	tconfig.intval[NO_AUDIO] = 0;
	tconfig.intval[NO_MUSIC] = 0;
	tconfig.fltval[SFX_VOLUME] = 1.0;
	tconfig.fltval[BGM_VOLUME] = 1.0;

	tconfig.intval[NO_STAGEBG] = 0;
	tconfig.intval[NO_STAGEBG_FPSLIMIT] = 40;

	tconfig.intval[SAVE_RPY] = 2;

	tconfig.intval[VID_WIDTH] = RESX;
	tconfig.intval[VID_HEIGHT] = RESY;

	char *name = "Player";
	tconfig.strval[PLAYERNAME] = malloc(strlen(name)+1);
	strcpy(tconfig.strval[PLAYERNAME], name);

	for(int o = CONFIG_GPKEY_FIRST; o <= CONFIG_GPKEY_LAST; ++o)
		tconfig.intval[o] = -1;

	tconfig.intval[GAMEPAD_AXIS_LR] = 0;
	tconfig.intval[GAMEPAD_AXIS_UD] = 1;

	tconfig.fltval[GAMEPAD_AXIS_UD_SENS] = 1.0;
	tconfig.fltval[GAMEPAD_AXIS_LR_SENS] = 1.0;

	tconfig.intval[GP_FOCUS] = 0;
	tconfig.intval[GP_SHOT]  = 1;
	tconfig.intval[GP_BOMB]  = 2;
	tconfig.intval[GP_SKIP]  = 3;
	tconfig.intval[GP_PAUSE] = 4;

	tconfig.intval[VSYNC] = 1;
}

int config_scan2key(int scan) {
	int i;
	for(i = CONFIG_KEY_FIRST; i <= CONFIG_KEY_LAST; ++i)
		if(scan == tconfig.intval[i])
			return i;
	return -1;
}

int config_button2gpkey(int btn) {
	int i;
	for(i = CONFIG_GPKEY_FIRST; i <= CONFIG_GPKEY_LAST; ++i)
		if(btn == tconfig.intval[i])
			return i;
	return -1;
}

/*
 *	The following function is no less ugly than it's name...
 *	The reason for this crap: config_button2gpkey maps the button ID to the first gamepad control option that's bound to the value of the button, similar to config_sym2key.
 *	However, we need that in the KEY_* format to use in stuff like player and replay events.
 *	So we have to transform them, somehow.
 *	Since I don't think relying on the enum's layout is a better idea, here comes a dumb switch.
 */

int config_gpkey2key(int gpkey) {
	switch(gpkey) {
		case GP_UP		:	return KEY_UP		;break;
		case GP_DOWN	:	return KEY_DOWN		;break;
		case GP_LEFT	:	return KEY_LEFT		;break;
		case GP_RIGHT	:	return KEY_RIGHT	;break;
		case GP_SHOT	:	return KEY_SHOT		;break;
		case GP_FOCUS	:	return KEY_FOCUS	;break;
		case GP_BOMB	:	return KEY_BOMB		;break;
		case GP_SKIP	:	return KEY_SKIP		;break;
	}

	return -1;
}

int config_button2key(int btn) {
	return config_gpkey2key(config_button2gpkey(btn));
}

FILE* config_open(char *filename, char *mode) {
	char *buf;
	FILE *out;

	buf = malloc(strlen(filename) + strlen(get_config_path()) + 2);
	strcpy(buf, get_config_path());
	strcat(buf, "/");
	strcat(buf, filename);

	out = fopen(buf, mode);
	free(buf);

	if(!out) {
		warnx("config_open(): couldn't open '%s'", filename);
		return NULL;
	}

	return out;
}

int config_intval_p(ConfigEntry *e) {
	return tconfig.intval[e->key];
}

char* config_strval_p(ConfigEntry *e) {
	return tconfig.strval[e->key];
}

float config_fltval_p(ConfigEntry *e) {
	return tconfig.fltval[e->key];
}

int config_intval(char *key) {
	return config_intval_p(config_findentry(key));
}

char* config_strval(char *key) {
	return config_strval_p(config_findentry(key));
}

float config_fltval(char *key) {
	return config_fltval_p(config_findentry(key));
}

void config_save(char *filename) {
	FILE *out = config_open(filename, "w");
	ConfigEntry *e = configdefs;

	if(!out)
		return;

	fputs("# Generated by taisei\n", out);

	do switch(e->type) {
		case CFGT_INT:
			fprintf(out, "%s = %i\n", e->name, config_intval_p(e));
			break;

		case CFGT_KEYBINDING:
			fprintf(out, "%s = %s\n", e->name, SDL_GetScancodeName(config_intval_p(e)));
			break;

		case CFGT_STRING:
			fprintf(out, "%s = %s\n", e->name, config_strval_p(e));
			break;

		case CFGT_FLOAT:
			fprintf(out, "%s = %f\n", e->name, config_fltval_p(e));
	} while((++e)->name);

	fclose(out);
	printf("Saved config '%s'\n", filename);
}

#define SYNTAXERROR { warnx("config_load(): syntax error on line %i, aborted! [%s:%i]\n", line, __FILE__, __LINE__); goto end; }
#define BUFFERERROR { warnx("config_load(): string exceed the limit of %i, aborted! [%s:%i]", CONFIG_LOAD_BUFSIZE, __FILE__, __LINE__); goto end; }
#define INTOF(s)   ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

void config_set(char *key, char *val) {
	ConfigEntry *e = config_findentry(key);

	if(!e) {
		warnx("config_set(): unknown setting '%s'", key);
		return;
	}

	switch(e->type) {
		case CFGT_INT:
			tconfig.intval[e->key] = INTOF(val);
			break;

		case CFGT_KEYBINDING: {
			SDL_Scancode scan = SDL_GetScancodeFromName(val);

			if(scan == SDL_SCANCODE_UNKNOWN) {
				warnx("config_set(): unknown key '%s'", val);
			} else {
				tconfig.intval[e->key] = scan;
			}

			break;

		}

		case CFGT_STRING:
			stralloc(&(tconfig.strval[e->key]), val);
			break;

		case CFGT_FLOAT:
			tconfig.fltval[e->key] = FLOATOF(val);
			break;
	}
}

#undef INTOF
#undef FLOATOF

void config_load(char *filename) {
	FILE *in = config_open(filename, "r");
	int c, i = 0, found, line = 0;
	char buf[CONFIG_LOAD_BUFSIZE];
	char key[CONFIG_LOAD_BUFSIZE];
	char val[CONFIG_LOAD_BUFSIZE];

	config_preset();
	if(!in)
		return;

	while((c = fgetc(in)) != EOF) {
		if(c == '#' && !i) {
			i = 0;
			while(fgetc(in) != '\n');
		} else if(c == ' ') {
			if(!i) SYNTAXERROR

			buf[i] = 0;
			i = 0;
			strcpy(key, buf);

			found = 0;
			while((c = fgetc(in)) != EOF) {
				if(c == '=') {
					if(++found > 1) SYNTAXERROR
				} else if(c != ' ') {
					if(!found || c == '\n') SYNTAXERROR

					do {
						if(c == '\n') {
							if(!i) SYNTAXERROR

							buf[i] = 0;
							i = 0;
							strcpy(val, buf);
							found = 0;
							++line;

							config_set(key, val);
							break;
						} else {
							buf[i++] = c;
							if(i == CONFIG_LOAD_BUFSIZE)
								BUFFERERROR
						}
					} while((c = fgetc(in)) != EOF);

					break;
				}
			} if(found) SYNTAXERROR
		} else {
			buf[i++] = c;
			if(i == CONFIG_LOAD_BUFSIZE)
				BUFFERERROR
		}
	}

end:
	fclose(in);
}

#undef SYNTAXERROR
#undef BUFFERERROR
